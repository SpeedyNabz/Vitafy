//! VitaSpot backend library.
//!
//! A librespot based implementation of `include/vam_backend.h` for the PS Vita
//! (and for desktop testing). The C API lives in [`ffi`]; everything else is
//! private implementation detail.

#![allow(clippy::missing_safety_doc)]

mod abi;
mod browse;
mod ordered;
mod engine;
mod ffi;
mod images;
mod internal;
mod items;
mod lyrics;
mod metadata;
mod model;
mod pages;
mod platform;
mod saved;
mod search;
mod shared;
mod state;
mod util;
mod volume;
mod webapi;

pub use abi::VAM_API_VERSION;

#[cfg(test)]
mod vitafy_regression_tests {
    use super::*;
    #[test]
    fn positional_remove_targets_only_one_duplicate() {
        let info=internal::PlaylistInfo{owner:"account".into(),revision:"0102".into(),
            track_uris:vec!["spotify:track:same".into(),"spotify:track:same".into()],
            entry_ids:vec!["first".into(),"second".into()],..Default::default()};
        let change=internal::positional_change(&info,"account","0102","spotify:track:same","second",1,1).unwrap();
        let removal=change.deltas[0].ops[0].rem.as_ref().unwrap();
        assert_eq!(removal.from_index(),1);assert_eq!(removal.length(),1);assert!(!removal.items_as_key());
        assert_eq!(change.base_revision(),&[1,2]);
    }
    #[test]
    fn positional_edits_reject_stale_revision_identity_and_owner() {
        let mut info=internal::PlaylistInfo{owner:"account".into(),revision:"0102".into(),
            track_uris:vec!["spotify:track:same".into()],entry_ids:vec!["entry".into()],..Default::default()};
        assert!(internal::positional_change(&info,"account","0304","spotify:track:same","entry",0,1).is_err());
        assert!(internal::positional_change(&info,"other","0102","spotify:track:same","entry",0,1).is_err());
        assert!(internal::positional_change(&info,"account","0102","spotify:track:same","wrong",0,1).is_err());
        info.revision="snow☃".into();
        assert!(internal::positional_change(&info,"account","snow☃","spotify:track:same","entry",0,1).is_err());
    }
    #[test]
    fn positional_move_uses_pre_removal_destination() {
        let info=internal::PlaylistInfo{owner:"account".into(),revision:"0102".into(),
            track_uris:vec!["a".into(),"b".into(),"c".into()],..Default::default()};
        let change=internal::positional_change(&info,"account","0102","a","",0,3).unwrap();
        let movement=change.deltas[0].ops[0].mov.as_ref().unwrap();
        assert_eq!(movement.from_index(),0);assert_eq!(movement.to_index(),2);assert_eq!(movement.length(),1);
    }
    #[test]
    fn queue_edits_preserve_context_and_duplicate_uids() {
        use librespot_protocol::player::ProvidedTrack;
        let queued=|uid:&str|ProvidedTrack{uri:"spotify:track:same".into(),uid:uid.into(),provider:"queue".into(),..Default::default()};
        let mut tracks=vec![queued("first"),ProvidedTrack{uri:"spotify:track:context".into(),provider:"context".into(),..Default::default()},queued("second")];
        assert!(librespot_connect::edit_queued_tracks(&mut tracks,1,1).is_err());
        librespot_connect::edit_queued_tracks(&mut tracks,0,3).unwrap();
        assert_eq!(tracks[0].uid,"second");assert_eq!(tracks[1].provider,"context");
        librespot_connect::edit_queued_tracks(&mut tracks,0,1).unwrap();
        assert_eq!(tracks[1].uid,"first");
        librespot_connect::edit_queued_tracks(&mut tracks,0,4).unwrap();
        assert_eq!(tracks.len(),1);assert_eq!(tracks[0].uri,"spotify:track:context");
    }
    #[test]
    fn source_quality_is_lossless_first_without_duplicates() {
        use librespot_metadata::audio::AudioFileFormat as Format;
        let formats=librespot_playback::player::preferred_audio_formats();
        assert_eq!(formats[0],Format::FLAC_FLAC_24BIT);
        assert_eq!(formats[1],Format::FLAC_FLAC);
        assert_eq!(formats[2],Format::OGG_VORBIS_320);
        for(i,f) in formats.iter().enumerate(){assert!(!formats[..i].contains(f));}
    }
    #[test]
    fn bandwidth_choices_never_select_lossless_or_higher_bitrate() {
        use librespot_metadata::audio::AudioFileFormat as F;
        use librespot_playback::player::audio_formats_for_quality;
        let low=audio_formats_for_quality(96);assert_eq!(low.len(),2);
        assert!(low.contains(&F::OGG_VORBIS_96)&&low.contains(&F::MP3_96));
        let medium=audio_formats_for_quality(160);
        assert_eq!(medium[0],F::OGG_VORBIS_160);assert!(medium.ends_with(low));
        assert!(!medium.contains(&F::FLAC_FLAC)&&!medium.contains(&F::OGG_VORBIS_320));
        assert_eq!(audio_formats_for_quality(0)[0],F::FLAC_FLAC_24BIT);
    }
    #[test]
    fn saved_cache_tracks_and_ordered_membership_agree() {
        let cache=saved::SavedCache::new();
        let uri="spotify:track:fixture";
        assert!(!cache.is_saved(uri));
        cache.set_saved(uri,true);assert!(cache.is_saved(uri));assert!(cache.snapshot_tracks().contains(uri));
        cache.set_saved(uri,true);assert_eq!(cache.snapshot_tracks().len(),1);
        cache.set_saved(uri,false);assert!(!cache.is_saved(uri));assert!(cache.snapshot_tracks().is_empty());
        cache.set_saved(uri,true);cache.clear();assert!(!cache.is_saved(uri));
    }
    #[test]
    fn metadata_audio_update_preserves_album_and_release() {
        let cache=metadata::MetaCache::new();
        let mut item=model::Item::default();item.uri="spotify:track:fixture".into();
        item.album_title="Album".into();item.album_uri="spotify:album:fixture".into();item.release_date=20260101;
        cache.put(item);
        let mut audio=model::Item::default();audio.uri="spotify:track:fixture".into();cache.put(audio);
        let item=cache.get("spotify:track:fixture").unwrap();
        assert_eq!(item.album_title,"Album");assert_eq!(item.release_date,20260101);
    }
    #[test]
    fn logout_changes_epoch_and_clears_authentication() {
        let web=webapi::WebApi::new();let before=web.epoch();web.clear_session();
        assert_ne!(web.epoch(),before);assert!(!web.is_authenticated());
    }
    #[test]
    fn abi_v4_carries_entry_identity_and_new_metadata() {
        let item=model::Item {uri:"spotify:track:fixture".into(),album_title:"Album".into(),
            added_at:1700000000123,release_date:20260101,original_index:7,entry_id:"duplicate-2".into(),..Default::default()};
        let c=item.to_c();assert_eq!(c.original_index,7);assert_eq!(c.added_at,1700000000123);
        assert_eq!(&c.entry_id[..11],b"duplicate-2");assert_eq!(VAM_API_VERSION,6);
    }
}

#[cfg(test)]
mod early_preload_tests {
    use librespot_playback::player::next_preload_ready;
    #[test]
    fn early_next_prepares_after_safe_headroom_without_waiting_for_whole_song() {
        assert!(!next_preload_ready(0, 240000, 30000, false));
        assert!(!next_preload_ready(2000, 238000, 2999, false));
        assert!(next_preload_ready(2000, 238000, 3000, false));
        assert!(next_preload_ready(5000, 235000, 9000, false));
        assert!(!next_preload_ready(5000, 235000, 0, false));
        assert!(next_preload_ready(2000, 238000, 0, true));
        assert!(next_preload_ready(0, 10000, 0, true));
    }
}

#[cfg(test)]
mod loader_lifetime_tests {
    use librespot_playback::player::bounded_track_load;
    use std::{sync::{Arc, atomic::{AtomicBool, Ordering}}, time::Duration};
    struct DropFlag(Arc<AtomicBool>);
    impl Drop for DropFlag { fn drop(&mut self) { self.0.store(true, Ordering::SeqCst); } }
    #[test]
    fn superseded_and_timed_out_network_futures_are_dropped() {
        tokio::runtime::Builder::new_current_thread().enable_all().build().unwrap().block_on(async {
            let (mut tx, rx) = tokio::sync::oneshot::channel::<u32>();
            assert_eq!(bounded_track_load(&mut tx, async { Some(7) }, Duration::from_secs(1)).await, Some(7));
            drop(rx);
            let dropped = Arc::new(AtomicBool::new(false));
            let guard = DropFlag(dropped.clone());
            let pending = async move { let _guard = guard; std::future::pending::<Option<u32>>().await };
            assert_eq!(bounded_track_load(&mut tx, pending, Duration::from_secs(1)).await, None);
            assert!(dropped.load(Ordering::SeqCst));
            let (mut tx, _rx) = tokio::sync::oneshot::channel::<u32>();
            let dropped = Arc::new(AtomicBool::new(false));
            let guard = DropFlag(dropped.clone());
            let pending = async move { let _guard = guard; std::future::pending::<Option<u32>>().await };
            assert_eq!(bounded_track_load(&mut tx, pending, Duration::from_millis(5)).await, None);
            assert!(dropped.load(Ordering::SeqCst));
        });
    }
}
