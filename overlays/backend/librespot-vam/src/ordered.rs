//! One bounded, expiring ordered-context upload; identities are positions, not URI keys.
use std::time::{Instant,Duration};
pub const MAX_TRACKS:usize=10_000;
pub const MAX_BYTES:usize=1024*1024;
pub const CHUNK:usize=128;
#[derive(Default)]
pub struct Uploads { generation:u64,next:u32,upload:Option<Upload> }
struct Upload { id:u32,count:usize,index:u32,tracks:Vec<String>,bytes:usize,started:Instant }
impl Uploads {
    pub fn reset(&mut self,generation:u64){self.generation=generation;self.upload=None;}
    pub fn generation(&self)->u64{self.generation}
    pub fn begin(&mut self,count:usize,index:u32)->u32{
        if count==0||count>MAX_TRACKS||index as usize>=count{return 0;}
        if self.upload.as_ref().is_some_and(|u|u.started.elapsed()<Duration::from_secs(30)){return 0;}
        let mut tracks=Vec::new();if tracks.try_reserve_exact(count).is_err(){return 0;}
        self.next=self.next.wrapping_add(1).max(1);let id=self.next;
        self.upload=Some(Upload{id,count,index,tracks,bytes:0,started:Instant::now()});id
    }
    pub fn append(&mut self,id:u32,tracks:Vec<String>)->bool{
        let Some(u)=self.upload.as_mut()else{return false;};
        if u.id!=id||u.started.elapsed()>Duration::from_secs(30)||tracks.is_empty()||tracks.len()>CHUNK||u.tracks.len()+tracks.len()>u.count{return false;}
        if tracks.iter().any(|s|s.len()>=96||(!s.starts_with("spotify:track:")&&!s.starts_with("spotify:episode:"))){return false;}
        let bytes=tracks.iter().map(|s|s.len()+24).sum::<usize>();if bytes+u.bytes>MAX_BYTES{return false;}
        u.bytes+=bytes;u.tracks.extend(tracks);true
    }
    pub fn commit(&mut self,id:u32)->Option<(Vec<String>,u32,u64)>{
        let u=self.upload.as_ref()?;
        if u.id!=id||u.tracks.len()!=u.count||u.started.elapsed()>Duration::from_secs(30){return None;}
        let u=self.upload.take()?;Some((u.tracks,u.index,self.generation))
    }
    pub fn cancel(&mut self,id:u32){if self.upload.as_ref().is_some_and(|u|u.id==id){self.upload=None;}}
}
#[cfg(test)]mod tests{
    use super::*;
    #[test]fn large_context_boundaries_duplicates_and_selection(){
        for count in [2048,2049,5000,10000]{for selected in [0,count/2,count-1]{
            let mut u=Uploads::default();u.reset(7);let id=u.begin(count,selected as u32);assert_ne!(id,0);
            for start in(0..count).step_by(CHUNK){assert!(u.append(id,(start..(start+CHUNK).min(count)).map(|i|format!("spotify:track:{}",i%17)).collect()));}
            let(v,index,epoch)=u.commit(id).unwrap();assert_eq!((v.len(),index,epoch),(count,selected as u32,7));
            assert_eq!(v[17],v[0]);assert!(u.commit(id).is_none());
        }}
    }
    #[test]fn real_connect_context_serialization_preserves_large_duplicate_order(){
        use protobuf::Message;
        for count in [2049,5000,10000]{
            let tracks:Vec<_>=(0..count).map(|i|format!("spotify:track:{:022}",i%31)).collect();
            let context=librespot_connect::ordered_context(tracks.clone());
            let wire=context.write_to_bytes().unwrap();
            assert!(wire.len()<8*1024*1024);
            let decoded=librespot_protocol::context::Context::parse_from_bytes(&wire).unwrap();
            assert_eq!(decoded.pages[0].tracks.len(),count);
            for i in [0,31,count/2,count-1]{assert_eq!(decoded.pages[0].tracks[i].uri(),tracks[i]);}
        }
    }
    #[tokio::test]async fn real_rolling_connect_windows_and_repeat_preserve_order(){
        for(count,start,steps,repeat)in [(5000,2000,170,false),(10000,5000,170,false),(5000,4998,6,true)]{
            let tracks:Vec<_>=(0..count).map(|i|format!("spotify:track:{:022}",i%31)).collect();
            let (snapshots,previous)=librespot_connect::ordered_window_probe(tracks.clone(),start,steps,repeat).unwrap();
            assert_eq!(snapshots.len(),steps+1);
            for(i,s)in snapshots.iter().enumerate(){assert_eq!(s.track_uri,tracks[(start+i)%count]);assert!(s.next_tracks.len()<=80);assert!(s.prev_tracks.len()<=10);}
            assert_eq!(previous.track_uri,tracks[(start+steps-1)%count]);
        }
    }
    #[test]fn incomplete_invalid_cancelled_and_old_account_uploads_rejected(){
        let mut u=Uploads::default();assert_eq!(u.begin(MAX_TRACKS+1,0),0);
        let id=u.begin(2,1);assert!(u.commit(id).is_none());assert!(!u.append(id,vec!["bad".into()]));
        assert!(u.append(id,vec!["spotify:track:a".into()]));u.reset(9);assert!(u.commit(id).is_none());
        let id=u.begin(1,0);u.cancel(id);assert!(!u.append(id,vec!["spotify:track:a".into()]));
        let id=u.begin(1,0);u.upload.as_mut().unwrap().started=Instant::now()-Duration::from_secs(31);assert!(!u.append(id,vec!["spotify:track:a".into()]));assert!(u.commit(id).is_none());
    }
}
