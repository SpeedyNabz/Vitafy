//! Bounded, account-scoped browse snapshots. Disk work never runs on the UI thread.
use std::{collections::HashMap, path::PathBuf, sync::{Arc,Mutex,atomic::{AtomicU64,Ordering}},time::{Instant,Duration}};
use serde::{Serialize,de::DeserializeOwned};
use crate::{internal::{self,PlaylistInfo},saved::Library,shared::Shared,webapi::ApiError,util::sha1_hex};
const RECORD_MAX:usize=16*1024*1024;
const RAM_MAX:usize=8*1024*1024;
#[derive(serde::Serialize,serde::Deserialize)]
struct Envelope { schema:u32,account:String,stored_at:u64,checksum:String,payload:String }
#[derive(Default)]
struct State { active:Option<(String,u64)>, snapshots:HashMap<String,(Arc<PlaylistInfo>,Instant,u64)>,stale:std::collections::HashSet<String>,pins:HashMap<String,Instant>,written:HashMap<String,(String,Instant)>,disk_used:HashMap<String,u64>,recent:Option<(Vec<String>,Instant)>,clock:u64,version:u64 }
pub struct BrowseCache {
    pub budget:Arc<AtomicU64>,
    root:PathBuf,state:Mutex<State>,io:Arc<Mutex<()>>,fetch:tokio::sync::Mutex<()>,flights:Mutex<HashMap<String,Arc<tokio::sync::Mutex<()>>>>,
    pub hits:AtomicU64,pub fetches:AtomicU64,pub disk_hits:AtomicU64,pub io_bytes:AtomicU64,
}
impl BrowseCache {
    pub fn new(root:String)->Arc<Self>{Arc::new(Self{budget:Arc::new(AtomicU64::new(32*1024*1024)),root:root.into(),state:Mutex::new(State::default()),io:Arc::new(Mutex::new(())),fetch:tokio::sync::Mutex::new(()),flights:Mutex::new(HashMap::new()),hits:AtomicU64::new(0),fetches:AtomicU64::new(0),disk_hits:AtomicU64::new(0),io_bytes:AtomicU64::new(0)})}
    pub fn with_budget(root:String,budget:Arc<AtomicU64>)->Arc<Self>{let mut cache=Self::new(root);Arc::get_mut(&mut cache).unwrap().budget=budget;cache}
    pub fn policy_changed(&self){self.state.lock().unwrap().written.clear();}
    pub fn activate(&self,account:String,epoch:u64){let mut s=self.state.lock().unwrap();s.version+=1;s.snapshots.clear();s.stale.clear();s.pins.clear();s.written.clear();s.disk_used.clear();s.recent=None;s.active=Some((account,epoch));}
    pub fn active(&self,account:&str,epoch:u64)->bool{self.state.lock().unwrap().active.as_ref().is_some_and(|(a,e)|a==account&&*e==epoch)}
    pub fn deactivate(&self){let mut s=self.state.lock().unwrap();s.version+=1;s.active=None;s.snapshots.clear();s.stale.clear();s.pins.clear();s.written.clear();s.disk_used.clear();s.recent=None;}
    pub fn invalidate(&self){let mut s=self.state.lock().unwrap();s.version+=1;s.snapshots.clear();s.stale.clear();s.pins.clear();s.written.clear();s.disk_used.clear();s.recent=None;}
    fn version(&self)->u64{self.state.lock().unwrap().version}
    pub async fn read<T:DeserializeOwned+Send+'static>(self:&Arc<Self>,account:&str,epoch:u64,key:&str)->Option<T>{
        let version=self.version();let this=self.clone();let account=account.to_string();let key=key.to_string();
        tokio::task::spawn_blocking(move||{
            let _io=this.io.lock().unwrap();if !this.active(&account,epoch)||this.version()!=version{return None;}
            let path=this.root.join(sha1_hex(account.as_bytes())).join(format!("{}.json",sha1_hex(key.as_bytes())));
            let value=[path.clone(),path.with_extension("bak")].iter().find_map(|p|read_record::<T>(p,&account));
            let Some(value)=value else{this.state.lock().unwrap().written.remove(&format!("{}:{}",sha1_hex(account.as_bytes()),sha1_hex(key.as_bytes())));return None;};
            let bytes=std::fs::metadata(&path).map(|m|m.len()).unwrap_or(0);
            this.state.lock().unwrap().disk_used.insert(format!("{}:{}",sha1_hex(account.as_bytes()),sha1_hex(key.as_bytes())),now_seconds());
            if this.version()!=version||!this.active(&account,epoch){return None;}
            this.disk_hits.fetch_add(1,Ordering::Relaxed);this.io_bytes.fetch_add(bytes as u64,Ordering::Relaxed);Some(value)
        }).await.ok().flatten()
    }
    pub async fn write<T:Serialize+Send+'static>(self:&Arc<Self>,account:String,epoch:u64,key:String,value:T)->bool{
        let version=self.version();let this=self.clone();tokio::task::spawn_blocking(move||{
            let _io=this.io.lock().unwrap();if !this.active(&account,epoch)||this.version()!=version{return false;}
            let Ok(payload)=serde_json::to_string(&value)else{return false;};if payload.len()>RECORD_MAX/2{return false;}
            let record=Envelope{schema:1,account:account.clone(),stored_at:now_seconds(),checksum:sha1_hex(payload.as_bytes()),payload};
            let write_key=format!("{}:{}",sha1_hex(account.as_bytes()),sha1_hex(key.as_bytes()));
            if this.state.lock().unwrap().written.get(&write_key).is_some_and(|(hash,at)|hash==&record.checksum&&at.elapsed()<Duration::from_secs(86400)){return true;}
            let checksum=record.checksum.clone();
            let Ok(bytes)=serde_json::to_vec(&record)else{return false;};if bytes.len()>RECORD_MAX{return false;}
            let dir=this.root.join(sha1_hex(account.as_bytes()));if std::fs::create_dir_all(&dir).is_err(){return false;}
            let path=dir.join(format!("{}.json",sha1_hex(key.as_bytes())));let tmp=path.with_extension("tmp");
            if write_sync(&tmp,&bytes).is_err(){let _=std::fs::remove_file(tmp);return false;}
            // Two valid generations: recover .bak after interrupted replacement on Vita/Windows.
            let backup=path.with_extension("bak");if path.exists(){let _=std::fs::remove_file(&backup);}
            if path.exists()&&std::fs::rename(&path,&backup).is_err(){let _=std::fs::remove_file(tmp);return false;}
            if std::fs::rename(&tmp,&path).is_err(){let _=std::fs::rename(&backup,&path);return false;}
            let _=std::fs::remove_file(backup);this.io_bytes.fetch_add(bytes.len()as u64,Ordering::Relaxed);
            let Ok(entries)=std::fs::read_dir(&dir)else{return false;};
            let mut files:Vec<_>=entries.flatten().take(256).filter_map(|e|{let m=e.metadata().ok()?;if !m.is_file(){return None;}Some((e.path(),m.len(),m.modified().ok()))}).collect();
            let account_hash=sha1_hex(account.as_bytes());
            {let state=this.state.lock().unwrap();files.sort_by_key(|f|{
                let key=format!("{}:{}",account_hash,f.0.file_stem().unwrap_or_default().to_string_lossy());
                state.disk_used.get(&key).copied().unwrap_or_else(||f.2.and_then(|t|t.duration_since(std::time::UNIX_EPOCH).ok()).map(|t|t.as_secs()).unwrap_or(0))
            });}
            let mut total:u64=files.iter().map(|f|f.1).sum();let mut count=files.len();
            let protected=[sha1_hex(b"tracks"),sha1_hex(b"library")];
            for (file,size,_)in files{
                if total<=this.budget.load(Ordering::Relaxed)&&count<=64{break;}
                let stem=file.file_stem().unwrap_or_default().to_string_lossy().to_string();
                if file!=path&&!protected.contains(&stem)&&std::fs::remove_file(&file).is_ok(){
                    total=total.saturating_sub(size);count-=1;
                    let key=format!("{account_hash}:{stem}");let mut state=this.state.lock().unwrap();state.written.remove(&key);state.disk_used.remove(&key);
                }
            }
            if total>this.budget.load(Ordering::Relaxed){let _=std::fs::remove_file(&path);return false;}
            {let mut state=this.state.lock().unwrap();state.disk_used.insert(write_key.clone(),now_seconds());state.written.insert(write_key,(checksum,Instant::now()));}
            true
        }).await.unwrap_or(false)
    }
    pub async fn clear(self:&Arc<Self>,account:String,epoch:u64)->bool{
        self.invalidate();let this=self.clone();tokio::task::spawn_blocking(move||{
            let _io=this.io.lock().unwrap();if !this.active(&account,epoch){return false;}
            let dir=this.root.join(sha1_hex(account.as_bytes()));
            match std::fs::read_dir(dir){Ok(files)=>{let mut ok=true;for f in files{match f{Ok(f)if f.file_type().map(|t|t.is_file()).unwrap_or(false)=>{ok&=std::fs::remove_file(f.path()).is_ok();},Ok(_)=>{},Err(_)=>ok=false}}ok},Err(e)=>e.kind()==std::io::ErrorKind::NotFound}
        }).await.unwrap_or(false)
    }
    pub async fn purge_logout(self:&Arc<Self>,account:String)->bool{
        self.deactivate();let this=self.clone();tokio::task::spawn_blocking(move||{
            let _io=this.io.lock().unwrap();clear_files(&this.root.join(sha1_hex(account.as_bytes())))
        }).await.unwrap_or(false)
    }
    pub async fn playlist(self:&Arc<Self>,shared:&Shared,uri:&str,offset:u32)->Result<(Arc<PlaylistInfo>,bool),ApiError>{
        let version=self.version();let epoch=shared.web.epoch();let account=shared.web.username();let flight={let mut flights=self.flights.lock().unwrap();if flights.len()>=32{flights.retain(|_,v|Arc::strong_count(v)>1);}if flights.len()>=32&&!flights.contains_key(uri){return Err(ApiError::unavailable("Too many simultaneous playlist loads. Retry shortly."));}flights.entry(uri.to_string()).or_insert_with(||Arc::new(tokio::sync::Mutex::new(()))).clone()};let _flight=flight.lock().await;
        if !self.active(&account,epoch){return Err(ApiError::auth("Session changed. Reopen the playlist."));}
        {
            let mut s=self.state.lock().unwrap();s.clock+=1;let clock=s.clock;
            if let Some((info,_,_))=s.snapshots.get(uri){if offset as usize+50>=info.track_uris.len(){s.pins.remove(uri);}else{s.pins.insert(uri.into(),Instant::now());}}
            let stale=s.stale.contains(uri);
            if let Some((info,at,used))=s.snapshots.get_mut(uri){
                // Keep one immutable revision throughout a complete load. Reopening revalidates after 60s.
                if offset>0||at.elapsed()<Duration::from_secs(60){*used=clock;self.hits.fetch_add(1,Ordering::Relaxed);return Ok((info.clone(),stale));}
            }
        }
        if let Some(info)=self.read::<PlaylistInfo>(&account,epoch,uri).await {
            if info.uri==uri&&info.track_uris.len()<=100_000&&snapshot_bytes(&info)<=RAM_MAX {
                let info=Arc::new(info);self.remember(uri,info.clone(),true);
                let web=shared.web.clone();let backend=shared.backend.clone();let cache=self.clone();let uri=uri.to_string();let account=account.clone();
                tokio::spawn(async move{
                    cache.fetches.fetch_add(1,Ordering::Relaxed);
                    if let Ok(Ok(fresh))=tokio::time::timeout(Duration::from_secs(20),internal::playlist(&web,&uri)).await{
                        if cache.active(&account,epoch)&&cache.version()==version&&snapshot_bytes(&fresh)<=RAM_MAX{
                            let fresh=Arc::new(fresh);cache.write(account,epoch,uri.clone(),(*fresh).clone()).await;
                            if cache.version()==version{cache.remember(&uri,fresh,false);backend.events.push_simple(crate::abi::VAM_EV_LIBRARY_CHANGED,crate::abi::VAM_OK,"","");}
                        }
                    }
                });
                return Ok((info,true));
            }
        }
        self.fetches.fetch_add(1,Ordering::Relaxed);
        let fetched=tokio::time::timeout(Duration::from_secs(20),internal::playlist(&shared.web,uri)).await.unwrap_or_else(|_|Err(ApiError::network("Playlist loading timed out. Retry it.")));
        if !self.active(&account,epoch)||self.version()!=version{return Err(ApiError::auth("Session changed."));}
        let (info,stale)=match fetched{
            Ok(info)=>{self.write(account.clone(),epoch,uri.into(),info.clone()).await;(info,false)},
            Err(error)=>{match self.read::<PlaylistInfo>(&account,epoch,uri).await{Some(info)if info.uri==uri=>(info,true),_=>return Err(error)}}
        };
        if info.track_uris.len()>100_000{return Err(ApiError::unavailable("Playlist is too large for available browse memory."));}
        if snapshot_bytes(&info)>RAM_MAX{return Err(ApiError::unavailable("Playlist exceeds the browse memory budget."));}
        let info=Arc::new(info);self.remember(uri,info.clone(),stale);Ok((info,stale))
    }
    fn remember(&self,uri:&str,info:Arc<PlaylistInfo>,stale:bool){
        let mut s=self.state.lock().unwrap();s.clock+=1;let clock=s.clock;
        if stale{s.stale.insert(uri.into());}else{s.stale.remove(uri);}
        s.pins.insert(uri.into(),Instant::now());
        s.snapshots.insert(uri.into(),(info,Instant::now(),clock));
        while s.snapshots.len()>16||s.snapshots.values().map(|(p,_,_)|snapshot_bytes(p)).sum::<usize>()>RAM_MAX{
            let victim=s.snapshots.iter().filter(|(k,_)|k.as_str()!=uri&&!s.pins.get(*k).is_some_and(|t|t.elapsed()<Duration::from_secs(30))).min_by_key(|(_,v)|v.2).map(|(k,_)|k.clone());
            if let Some(key)=victim{s.snapshots.remove(&key);s.stale.remove(&key);s.pins.remove(&key);}else{s.snapshots.remove(uri);s.stale.remove(uri);s.pins.remove(uri);break;}
        }
    }
    pub async fn recent(&self,web:&crate::webapi::WebApi)->Vec<String>{
        let _flight=self.fetch.lock().await;
        {let s=self.state.lock().unwrap();if let Some((uris,at))=&s.recent{if at.elapsed()<Duration::from_secs(60){return uris.clone();}}}
        let epoch=web.epoch();let uris=internal::recently_played_contexts(web).await;
        if web.guard_epoch(epoch).is_some(){self.state.lock().unwrap().recent=Some((uris.clone(),Instant::now()));}uris
    }
    pub async fn warm(self:&Arc<Self>,shared:&Shared){
        let account=shared.web.username();let epoch=shared.web.epoch();
        shared.meta.bind(self.clone(),account.clone(),epoch);
        if let Some(library)=self.read::<Library>(&account,epoch,"library").await{
            if let Some(_guard)=shared.web.guard_epoch(epoch){shared.saved.seed(library);}
        }
        if let Some(items)=self.read::<Vec<crate::model::Item>>(&account,epoch,"tracks").await{
            if let Some(_guard)=shared.web.guard_epoch(epoch){
                for item in items.into_iter().rev().take(16384){if shared.meta.get(&item.uri).is_none(){shared.meta.put(item);}}
            }
        }
    }

    pub async fn flush(self:&Arc<Self>,shared:&Shared){
        let account=shared.web.username();let epoch=shared.web.epoch();if account.is_empty(){return;}
        if let Some(items)=shared.meta.take_dirty_snapshot(){
            let seen:std::collections::HashSet<_>=items.iter().map(|i|i.uri.clone()).collect();
            let mut old=self.read::<Vec<crate::model::Item>>(&account,epoch,"tracks").await.unwrap_or_default();
            old.retain(|i|!seen.contains(&i.uri));old.extend(items);
            let mut bytes=0;let mut bounded=Vec::new();
            for i in old.into_iter().rev(){let n=serde_json::to_vec(&i).map(|v|v.len()+1).unwrap_or(0);if bounded.len()>=16384||bytes+n>6*1024*1024{break;}bytes+=n;bounded.push(i);}
            bounded.reverse();if !self.write(account.clone(),epoch,"tracks".into(),bounded).await{shared.meta.mark_dirty();}
        }
        self.write(account,epoch,"library".into(),shared.saved.snapshot()).await;
        log::info!("browse counters hits={} fetches={} disk_hits={} io_bytes={}",self.hits.load(Ordering::Relaxed),self.fetches.load(Ordering::Relaxed),self.disk_hits.load(Ordering::Relaxed),self.io_bytes.load(Ordering::Relaxed));
    }
}
fn snapshot_bytes(p:&PlaylistInfo)->usize{p.track_uris.iter().map(|s|s.len()+24).sum::<usize>()+p.entry_ids.iter().map(|s|s.len()+24).sum::<usize>()+p.added.len()*8+p.name.len()+p.description.len()+512}

fn now_seconds()->u64{std::time::SystemTime::now().duration_since(std::time::UNIX_EPOCH).map(|t|t.as_secs()).unwrap_or(0)}
fn write_sync(path:&std::path::Path,bytes:&[u8])->std::io::Result<()>{use std::io::Write;let mut f=std::fs::File::create(path)?;f.write_all(bytes)?;f.sync_all()}

fn clear_files(dir:&std::path::Path)->bool{
    match std::fs::read_dir(dir){Ok(files)=>files.flatten().all(|f|!f.file_type().map(|t|t.is_file()).unwrap_or(false)||std::fs::remove_file(f.path()).is_ok()),Err(e)=>e.kind()==std::io::ErrorKind::NotFound}
}
fn read_record<T:DeserializeOwned>(path:&std::path::Path,account:&str)->Option<T>{
    if std::fs::metadata(path).ok()?.len()>RECORD_MAX as u64{return None;}
    let bytes=std::fs::read(path).ok()?;if bytes.len()>RECORD_MAX{return None;}
    let record:Envelope=serde_json::from_slice(&bytes).ok()?;let now=now_seconds();
    if record.schema!=1||record.account!=account||record.stored_at>now+300||now.saturating_sub(record.stored_at)>7*86400||sha1_hex(record.payload.as_bytes())!=record.checksum{return None;}
    serde_json::from_str(&record.payload).ok()
}

#[cfg(test)]mod tests{
    use super::*;
    fn fixture()->(PathBuf,Arc<BrowseCache>){
        static NEXT:AtomicU64=AtomicU64::new(0);
        let root=std::env::temp_dir().join(format!("vitafy-browse-{}-{}",std::process::id(),NEXT.fetch_add(1,Ordering::Relaxed)));
        let cache=BrowseCache::new(root.to_string_lossy().into());cache.activate("alice".into(),1);(root,cache)
    }
    fn path(root:&std::path::Path,key:&str)->PathBuf{root.join(sha1_hex(b"alice")).join(format!("{}.json",sha1_hex(key.as_bytes())))}
    #[tokio::test]async fn persistent_roundtrip_corruption_schema_account_and_backup_recovery(){
        let(root,c)=fixture();assert!(c.write("alice".into(),1,"test".into(),vec!["a","a","b"]).await);
        let reopened=BrowseCache::new(root.to_string_lossy().into());reopened.activate("alice".into(),2);
        assert_eq!(reopened.read::<Vec<String>>("alice",2,"test").await.unwrap(),["a","a","b"]);
        assert!(reopened.read::<Vec<String>>("bob",2,"test").await.is_none());
        let p=path(&root,"test");let original=std::fs::read(&p).unwrap();
        std::fs::rename(&p,p.with_extension("bak")).unwrap();std::fs::write(&p,b"interrupted json").unwrap();
        assert!(reopened.read::<Vec<String>>("alice",2,"test").await.is_some());std::fs::remove_file(p.with_extension("bak")).unwrap();
        assert!(reopened.read::<Vec<String>>("alice",2,"test").await.is_none());
        for change in ["schema","account","checksum","stored_at"]{
            let mut value:serde_json::Value=serde_json::from_slice(&original).unwrap();
            value[change]=match change{"schema"=>99.into(),"account"=>"bob".into(),"checksum"=>"bad".into(),_=>0.into()};
            std::fs::write(&p,serde_json::to_vec(&value).unwrap()).unwrap();assert!(reopened.read::<Vec<String>>("alice",2,"test").await.is_none());
        }
        assert!(reopened.write("alice".into(),2,"test".into(),vec!["repaired"]).await);
        assert_eq!(reopened.read::<Vec<String>>("alice",2,"test").await.unwrap(),["repaired"]);
        assert!(reopened.purge_logout("alice".into()).await);assert!(!reopened.active("alice",2));
        std::fs::remove_dir_all(root).unwrap();
    }
    #[tokio::test]async fn failed_write_preserves_old_record_and_old_session_jobs_are_rejected(){
        let(root,c)=fixture();assert!(c.write("alice".into(),1,"test".into(),"old").await);
        let p=path(&root,"test");std::fs::create_dir(p.with_extension("tmp")).unwrap();
        assert!(!c.write("alice".into(),1,"test".into(),"new").await);
        assert_eq!(c.read::<String>("alice",1,"test").await.unwrap(),"old");
        c.activate("bob".into(),2);assert!(!c.write("alice".into(),1,"test".into(),"wrong account").await);
        assert!(c.read::<String>("bob",2,"test").await.is_none());
        let huge="x".repeat(RECORD_MAX);assert!(!c.write("bob".into(),2,"huge".into(),huge).await);
        std::fs::remove_dir_all(root).unwrap();
    }
    #[tokio::test]async fn disk_byte_and_file_budgets_evict_cold_records_and_coalesce_writes(){
        let(root,c)=fixture();c.budget.store(2048,Ordering::Relaxed);
        for n in 0..10{assert!(c.write("alice".into(),1,format!("p{n}"),"x".repeat(400)).await);}
        let dir=root.join(sha1_hex(b"alice"));let size:u64=std::fs::read_dir(&dir).unwrap().flatten().map(|f|f.metadata().unwrap().len()).sum();assert!(size<=2048);
        let before=c.io_bytes.load(Ordering::Relaxed);assert!(c.write("alice".into(),1,"p9".into(),"x".repeat(400)).await);assert_eq!(before,c.io_bytes.load(Ordering::Relaxed));
        assert!(c.clear("alice".into(),1).await);assert!(c.read::<String>("alice",1,"p9").await.is_none());
        c.budget.store(32*1024*1024,Ordering::Relaxed);
        for n in 0..70{assert!(c.write("alice".into(),1,format!("k{n}"),n).await);}
        assert!(std::fs::read_dir(&dir).unwrap().count()<=64);std::fs::remove_dir_all(root).unwrap();
    }
    #[tokio::test]async fn simultaneous_page_slices_share_an_immutable_cached_snapshot(){
        use crate::{state::{Backend,Config},webapi::WebApi};
        let(root,_)=fixture();
        let backend=Backend::new(Config{device_name:"test".into(),data_dir:root.to_string_lossy().into(),bitrate:0,initial_volume:0,normalize_volume:false,audio_cache_bytes:1024});
        let shared=Shared::new(backend,WebApi::new());shared.browse.activate(String::new(),0);
        let info=PlaylistInfo{uri:"spotify:playlist:test".into(),revision:"revision-a".into(),track_uris:vec!["spotify:track:duplicate".into();5000],..Default::default()};
        shared.browse.remember(&info.uri,Arc::new(info.clone()),false);
        let replies=futures_util::future::join_all((0..100).map(|i|shared.browse.playlist(&shared,&info.uri,i*50))).await;
        for reply in replies{let(info,stale)=reply.unwrap();assert_eq!(info.revision,"revision-a");assert_eq!(info.track_uris.len(),5000);assert!(!stale);}
        assert_eq!(shared.browse.fetches.load(Ordering::Relaxed),0);assert_eq!(shared.browse.hits.load(Ordering::Relaxed),100);
        shared.browse.invalidate();assert!(shared.browse.state.lock().unwrap().snapshots.is_empty());
        // An account/session change invalidates old reads even when a record remains on disk.
        shared.browse.deactivate();assert!(shared.browse.read::<PlaylistInfo>("",0,&info.uri).await.is_none());
    }
}
