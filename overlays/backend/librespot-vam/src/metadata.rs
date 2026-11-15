//! Small in-memory metadata cache, plus batched internal metadata lookups.

use std::collections::HashMap;
use std::sync::Mutex;

use librespot_metadata::audio::{AudioItem, UniqueFields};

use crate::abi::*;
use crate::internal;
use crate::model::Item;
use crate::webapi::WebApi;

const MAX_ENTRIES: usize = 16384;
#[derive(Default)]
struct Entries { items:HashMap<String,(Item,u64)>,clock:u64,dirty:bool,bytes:usize }
#[derive(Default)]
pub struct MetaCache { pub hits:std::sync::atomic::AtomicU64,pub misses:std::sync::atomic::AtomicU64,pub batches:std::sync::atomic::AtomicU64,items:Mutex<Entries>,disk:Mutex<Option<(std::sync::Arc<crate::browse::BrowseCache>,String,u64)>> }
impl MetaCache {
    pub fn new()->Self{Self::default()}
    pub fn get(&self,uri:&str)->Option<Item>{
        let mut g=self.items.lock().unwrap();g.clock+=1;let now=g.clock;
        if g.items.contains_key(uri){self.hits.fetch_add(1,std::sync::atomic::Ordering::Relaxed);}else{self.misses.fetch_add(1,std::sync::atomic::Ordering::Relaxed);}
        g.items.get_mut(uri).map(|(item,used)|{
            *used=now;let mut item=item.clone();
            let ttl=if item.album_title.is_empty()||item.release_date==0{3600}else{86400};
            if item.metadata_resolved&&unix_seconds().saturating_sub(item.metadata_at)>ttl{item.metadata_resolved=false;}
            item
        })
    }
    pub fn put(&self,mut item:Item){
        let mut g=self.items.lock().unwrap();
        if let Some((old,_))=g.items.get(&item.uri){
            if !item.metadata_resolved{item.metadata_at=old.metadata_at;}
            item.metadata_resolved|=old.metadata_resolved;
            if item.album_uri.is_empty(){item.album_uri=old.album_uri.clone();}
            if item.album_title.is_empty(){item.album_title=old.album_title.clone();}
            if item.release_date==0{item.release_date=old.release_date;}
        }
        if item.metadata_resolved&&item.metadata_at==0{item.metadata_at=unix_seconds();}
        let changed=g.items.get(&item.uri).map(|(old,_)|!same_core(old,&item)).unwrap_or(true);
        g.clock+=1;let now=g.clock;let bytes=item_bytes(&item);if bytes>8*1024*1024{return;}
        if let Some((old,_))=g.items.remove(&item.uri){g.bytes=g.bytes.saturating_sub(item_bytes(&old));}
        g.bytes+=bytes;g.items.insert(item.uri.clone(),(item,now));g.dirty|=changed;
        while g.items.len()>MAX_ENTRIES||g.bytes>8*1024*1024{
            let key=g.items.iter().min_by_key(|(_,v)|v.1).map(|(k,_)|k.clone());if let Some(k)=key{if let Some((old,_))=g.items.remove(&k){g.bytes=g.bytes.saturating_sub(item_bytes(&old));}}else{break;}
        }
    }
    pub fn clear(&self){*self.items.lock().unwrap()=Entries::default();*self.disk.lock().unwrap()=None;}
    pub fn bind(&self,disk:std::sync::Arc<crate::browse::BrowseCache>,account:String,epoch:u64){*self.disk.lock().unwrap()=Some((disk,account,epoch));}
    pub async fn hydrate(&self,web:&WebApi,uris:&[String]){
        let binding=self.disk.lock().unwrap().clone();let Some((disk,account,epoch))=binding else{return;};
        if !uris.iter().any(|u|self.get(u).is_none()){return;}
        if let Some(items)=disk.read::<Vec<Item>>(&account,epoch,"tracks").await{
            let wanted:std::collections::HashSet<_>=uris.iter().collect();
            let Some(_guard)=web.guard_epoch(epoch)else{return;};
            for item in items{if wanted.contains(&item.uri){self.put(item);}}
        }
    }
    pub fn resident_bytes(&self)->usize{self.items.lock().unwrap().bytes}
    pub fn mark_dirty(&self){self.items.lock().unwrap().dirty=true;}

    pub fn take_dirty_snapshot(&self)->Option<Vec<Item>>{
        let mut g=self.items.lock().unwrap();if !g.dirty{return None;}g.dirty=false;
        let mut items:Vec<_>=g.items.values().collect();items.sort_by_key(|(_,used)|*used);
        Some(items.into_iter().map(|(i,_)|{let mut i=i.clone();i.is_saved=false;i.is_owned=0;i.queued=false;i.unverified=false;i.entry_id.clear();i.added_at=0;i.parent_uri.clear();i}).collect())
    }
}
fn item_bytes(i:&Item)->usize{std::mem::size_of::<Item>()+std::mem::size_of::<String>()+48+i.uri.len()*2+i.title.len()+i.subtitle.len()+i.image_url.len()+i.album_uri.len()+i.artist_uri.len()+i.album_title.len()+i.creator.len()+i.entry_id.len()+i.parent_uri.len()}

#[cfg(test)]mod cache_tests{
    use super::*;
    #[test]fn incremental_eviction_preserves_hot_entries_and_unknown_fields(){
        let cache=MetaCache::new();for i in 0..MAX_ENTRIES{cache.put(Item{uri:format!("spotify:track:{i}"),metadata_resolved:true,..Default::default()});}
        assert!(cache.get("spotify:track:0").is_some());cache.put(Item{uri:"spotify:track:new".into(),..Default::default()});
        assert!(cache.get("spotify:track:0").is_some());assert!(cache.get("spotify:track:1").is_none());
        assert_eq!(cache.items.lock().unwrap().items.len(),MAX_ENTRIES);
    }
    #[tokio::test]async fn expired_metadata_retries_but_keeps_last_good_details_on_failure(){
        let cache=MetaCache::new();let uri="spotify:track:old".to_string();
        cache.put(Item{uri:uri.clone(),title:"Last good title".into(),metadata_resolved:true,metadata_at:unix_seconds().saturating_sub(7200),..Default::default()});
        assert!(!cache.get(&uri).unwrap().metadata_resolved);
        let rows=resolve_playables(&WebApi::new(),&cache,&[uri]).await;assert_eq!(rows[0].title,"Last good title");
        assert_eq!(cache.batches.load(std::sync::atomic::Ordering::Relaxed),1);
    }
    #[tokio::test]async fn resolved_unknown_dates_are_hits_and_network_failures_are_not_persisted(){
        let cache=MetaCache::new();let uri="spotify:track:known".to_string();
        cache.put(Item{uri:uri.clone(),title:"Known title".into(),metadata_resolved:true,..Default::default()});
        let web=WebApi::new();let rows=resolve_playables(&web,&cache,&[uri.clone(),uri]).await;
        assert_eq!(rows.len(),2);assert_eq!(cache.batches.load(std::sync::atomic::Ordering::Relaxed),0);
        let missing="spotify:track:missing".to_string();let _=resolve_playables(&web,&cache,&[missing.clone(),missing.clone()]).await;
        assert!(cache.get(&missing).is_none());
    }
}

/// Convert the metadata librespot decodes from the audio stream into an item.
pub fn item_from_audio(audio: &AudioItem) -> Item {
    let mut item = Item {
        uri: audio.uri.clone(),
        title: audio.name.clone(),
        duration_ms: audio.duration_ms,
        is_explicit: audio.is_explicit,
        is_playable: true,
        kind: if audio.uri.starts_with("spotify:episode:") {
            VAM_KIND_EPISODE
        } else {
            VAM_KIND_TRACK
        },
        ..Default::default()
    };

    if let Some(cover) = audio
        .covers
        .iter()
        .min_by_key(|c| (c.width as i64 - 300).abs())
    {
        item.image_url = cover.url.clone();
    }

    match &audio.unique_fields {
        UniqueFields::Track {
            artists,
            album,
            album_artists: _,
            ..
        } => {
            item.subtitle = artists
                .0
                .iter()
                .map(|a| a.name.clone())
                .collect::<Vec<_>>()
                .join(", ");
            item.artist_uri = artists
                .0
                .first()
                .and_then(|a| a.id.to_uri().ok())
                .unwrap_or_default();
            // librespot's AudioItem only carries the album name, so the album
            // URI is filled in later from extended metadata when available.
            item.album_title = album.clone();
        }
        UniqueFields::Episode {
            show_name,
            description,
            ..
        } => {
            item.subtitle = show_name.clone();
            item.album_uri.clear();
            let _ = description;
        }
        UniqueFields::Local { .. } => {
            item.is_playable = false;
        }
    }

    item
}

/// Resolve a list of track/episode URIs to items, using the cache first and
/// batching extended-metadata requests for whatever is missing.
pub async fn resolve_playables(
    web: &WebApi,
    cache: &MetaCache,
    uris: &[String],
) -> Vec<Item> {
    cache.hydrate(web,uris).await;
    let mut result: Vec<Option<Item>> = uris.iter().map(|u| cache.get(u).filter(|i|i.metadata_resolved)).collect();

    let mut seen=std::collections::HashSet::new();
    let epoch=web.epoch();
    let missing: Vec<String> = uris
        .iter()
        .zip(result.iter())
        .filter(|(uri, cached)| {
            cached.is_none()
                && (uri.starts_with("spotify:track:") || uri.starts_with("spotify:episode:")||uri.starts_with("spotify:album:")||uri.starts_with("spotify:artist:"))
        })
        .filter(|(uri,_)|seen.insert((*uri).clone()))
        .map(|(uri, _)| uri.clone())
        .collect();
    if !missing.is_empty() {
        cache.batches.fetch_add(1,std::sync::atomic::Ordering::Relaxed);
        for mut item in internal::resolve_items(web, &missing).await {
            item.metadata_resolved=true;
            let Some(_guard)=web.guard_epoch(epoch)else{return Vec::new();};
            cache.put(item.clone());
            for (index, uri) in uris.iter().enumerate() {
                if *uri == item.uri && result[index].is_none() {
                    result[index] = Some(item.clone());
                }
            }
        }
    }

    result
        .into_iter()
        .zip(uris.iter())
        .map(|(item, uri)| {
            item.or_else(||cache.get(uri)).unwrap_or_else(|| Item {
                kind: if uri.starts_with("spotify:episode:") {
                    VAM_KIND_EPISODE
                } else {
                    if uri.starts_with("spotify:album:"){VAM_KIND_ALBUM}else if uri.starts_with("spotify:artist:"){VAM_KIND_ARTIST}else{VAM_KIND_TRACK}
                },
                uri: uri.clone(),
                title: "Unavailable track".to_string(),
                is_playable: false,
                ..Default::default()
            })
        })
        .collect()
}

fn unix_seconds()->u64{std::time::SystemTime::now().duration_since(std::time::UNIX_EPOCH).map(|t|t.as_secs()).unwrap_or(0)}

/// Immediate local details for stale-first browse pages; never starts a network lookup.
pub async fn resolve_cached_only(web:&WebApi,cache:&MetaCache,uris:&[String])->Vec<Item>{
    cache.hydrate(web,uris).await;uris.iter().filter_map(|u|cache.get(u)).collect()
}

fn same_core(a:&Item,b:&Item)->bool{
    a.kind==b.kind&&a.uri==b.uri&&a.title==b.title&&a.subtitle==b.subtitle&&a.image_url==b.image_url&&a.album_uri==b.album_uri&&a.artist_uri==b.artist_uri&&a.duration_ms==b.duration_ms&&a.is_explicit==b.is_explicit&&a.is_playable==b.is_playable&&a.album_title==b.album_title&&a.creator==b.creator&&a.release_date==b.release_date&&a.metadata_resolved==b.metadata_resolved&&a.metadata_at==b.metadata_at
}
