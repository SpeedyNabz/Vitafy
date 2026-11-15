//! Session holder shared by the metadata code, plus plain HTTP downloads
//! (image CDN) on the HTTP stack librespot already owns. Everything else
//! talks to Spotify's internal endpoints (`internal.rs`, `search.rs`); the
//! public Web API rate-limits librespot's shared client id.

use std::sync::{Arc, Mutex, atomic::{AtomicU64,Ordering}};

use bytes::Bytes;
use http::{header, Method, Request};
use http_body_util::BodyExt;
use librespot_core::Session;

use crate::abi::*;

#[derive(Debug, Clone)]
pub struct ApiError {
    pub result: i32,
    pub message: String,
}

impl ApiError {
    pub fn new(result: i32, message: impl Into<String>) -> Self {
        ApiError {
            result,
            message: message.into(),
        }
    }

    pub fn auth(message: impl Into<String>) -> Self {
        Self::new(VAM_ERR_AUTH, message)
    }

    pub fn network(message: impl Into<String>) -> Self {
        Self::new(VAM_ERR_NETWORK, message)
    }

    pub fn not_found() -> Self {
        Self::new(VAM_ERR_NOT_FOUND, "Spotify has no such item.")
    }

    pub fn unavailable(message: impl Into<String>) -> Self {
        Self::new(VAM_ERR_UNAVAILABLE, message)
    }

    pub fn rate_limited(message: impl Into<String>) -> Self {
        Self::new(VAM_ERR_RATE_LIMITED, message)
    }
}

/// Shared Web API client. Cheap to clone.
#[derive(Clone)]
pub struct WebApi {
    inner: Arc<Inner>,
}

struct Inner {
    epoch: AtomicU64,
    session: Mutex<Option<Session>>,
}

impl WebApi {
    pub fn new() -> Self {
        WebApi {
            inner: Arc::new(Inner {
                epoch: AtomicU64::new(0),
                session: Mutex::new(None),
            }),
        }
    }

    pub fn epoch(&self) -> u64 { self.inner.epoch.load(Ordering::SeqCst) }

    /// Short synchronous commit guard; never hold this across an await.
    pub fn guard_epoch(&self,epoch:u64) -> Option<std::sync::MutexGuard<'_,Option<Session>>> {
        let guard=self.inner.session.lock().unwrap_or_else(|e|e.into_inner());
        if self.epoch()==epoch {Some(guard)}else{None}
    }

    pub fn set_session(&self, session: Session) {
        let mut guard=self.inner.session.lock().unwrap_or_else(|e|e.into_inner());
        self.inner.epoch.fetch_add(1,Ordering::SeqCst);
        *guard = Some(session);
    }

    pub fn clear_session(&self) {
        let mut guard=self.inner.session.lock().unwrap_or_else(|e|e.into_inner());
        self.inner.epoch.fetch_add(1,Ordering::SeqCst);
        *guard = None;
    }

    pub fn is_authenticated(&self) -> bool {
        self.inner
            .session
            .lock()
            .unwrap_or_else(|e| e.into_inner())
            .is_some()
    }

    /// Canonical username of the logged in account (empty when logged out).
    pub fn username(&self) -> String {
        self.inner
            .session
            .lock()
            .unwrap_or_else(|e| e.into_inner())
            .as_ref()
            .map(|s| s.username())
            .unwrap_or_default()
    }

    /// Spotify market/country code reported by the session.
    pub fn country(&self) -> String {
        self.inner
            .session
            .lock()
            .unwrap_or_else(|e| e.into_inner())
            .as_ref()
            .map(|s| s.country())
            .unwrap_or_default()
    }

    pub(crate) fn session(&self) -> Result<Session, ApiError> {
        self.inner
            .session
            .lock()
            .unwrap_or_else(|e| e.into_inner())
            .clone()
            .ok_or_else(|| ApiError::auth("Not logged in."))
    }

    pub async fn playlist_request(&self,method: Method,path: &str,body: serde_json::Value) -> Result<serde_json::Value,ApiError> {
        let session=self.session()?;
        let token=session.token_provider().get_token("playlist-modify-private,playlist-modify-public").await
            .map_err(|_| ApiError::new(VAM_ERR_UNSUPPORTED,"Spotify did not grant playlist editing for this session."))?;
        let request=Request::builder().method(method).uri(format!("https://api.spotify.com/v1/{path}"))
            .header(header::AUTHORIZATION,format!("Bearer {}",token.access_token))
            .header(header::CONTENT_TYPE,"application/json")
            .body(Bytes::from(body.to_string())).map_err(|_| ApiError::network("Couldn't prepare playlist edit."))?;
        let response=session.http_client().request(request).await.map_err(|_| ApiError::network("Playlist edit connection failed."))?;
        let status=response.status();
        if status.as_u16()==403 || status.as_u16()==401 { return Err(ApiError::new(VAM_ERR_UNSUPPORTED,"Spotify does not permit playlist editing for this session.")); }
        if status.as_u16()==429 { return Err(ApiError::rate_limited("Spotify rate limited playlist editing. Try later.")); }
        if !status.is_success() { return Err(ApiError::network(format!("Playlist edit failed (HTTP {}).",status.as_u16()))); }
        let data=response.into_body().collect().await.map_err(|_| ApiError::network("Couldn't read playlist edit result."))?.to_bytes();
        if data.is_empty(){Ok(serde_json::Value::Null)} else {serde_json::from_slice(&data).map_err(|_|ApiError::network("Invalid playlist edit result."))}
    }

    /// Download arbitrary bytes (image CDN). No auth required.
    pub async fn get_bytes(&self, url: &str) -> Result<Vec<u8>, ApiError> {
        let session = self.session()?;
        let request = Request::builder()
            .method(Method::GET)
            .uri(url)
            .header(header::USER_AGENT, "VitaSpot/1.0")
            .body(Bytes::new())
            .map_err(|e| ApiError::new(VAM_ERR_UNKNOWN, e.to_string()))?;
        let response = session
            .http_client()
            .request(request)
            .await
            .map_err(|e| ApiError::network(format!("Image download failed: {e}")))?;
        let status = response.status();
        let bytes = response
            .into_body()
            .collect()
            .await
            .map(|c| c.to_bytes().to_vec())
            .map_err(|e| ApiError::network(format!("Image download failed: {e}")))?;
        if !status.is_success() {
            return Err(ApiError::network(format!("Image download HTTP {status}")));
        }
        Ok(bytes)
    }
}

impl Default for WebApi {
    fn default() -> Self {
        Self::new()
    }
}
