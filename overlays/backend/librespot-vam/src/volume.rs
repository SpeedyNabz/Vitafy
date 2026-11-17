//! App/Spotify gain is independent of the Vita's speaker/Bluetooth hardware gain.
//! Never poll AVConfig or write system volume here: its generic level does not
//! identify the output route and can echo the wrong level during Bluetooth changes.
use std::sync::Arc;
use librespot_core::Error;
use librespot_playback::mixer::{softmixer::SoftMixer, Mixer, MixerConfig};

pub fn open(config: MixerConfig) -> Result<Arc<dyn Mixer>, Error> {
    Ok(Arc::new(SoftMixer::open(config)?))
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn app_gain_has_one_route_independent_source_and_supports_mute() {
        let mixer = open(MixerConfig::default()).unwrap();
        let gain = mixer.get_soft_volume();
        mixer.set_volume(u16::MAX);
        assert_eq!(mixer.volume(), u16::MAX);
        assert!((gain.attenuation_factor() - 1.0).abs() < 1e-6);
        mixer.set_volume(20000);
        assert_eq!(mixer.volume(), 20000);
        assert!(gain.attenuation_factor() > 0.0 && gain.attenuation_factor() < 1.0);
        mixer.set_volume(0);
        assert_eq!(mixer.volume(), 0);
        assert_eq!(gain.attenuation_factor(), 0.0);
        // A later remote command is applied once, including after mute.
        mixer.set_volume(45000);
        assert!(mixer.volume().abs_diff(45000) <= 1); // mapped f64 roundtrip
        assert!(gain.attenuation_factor() > 0.0);
    }
}
