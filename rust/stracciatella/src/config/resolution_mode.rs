use serde::{Deserialize, Serialize};

use crate::config::Resolution;

/// How the game picks its resolution
#[derive(Debug, Default, PartialEq, Eq, Copy, Clone, Serialize, Deserialize)]
#[serde(rename_all = "lowercase")]
#[repr(C)]
#[allow(non_camel_case_types)]
pub enum ResolutionMode {
    /// The game picks the base resolution from the desktop resolution on every start
    /// (see `auto_base_resolution`); `res` in ja2.json is not used
    #[default]
    AUTO,
    /// The game uses `res` from ja2.json (or `--res`), as chosen by the player
    MANUAL,
}

/// Base resolutions the launcher offers, smallest first. The smallest one is
/// also the minimum desktop resolution the launcher supports.
pub const BASE_RESOLUTIONS: [Resolution; 2] = [Resolution(1280, 720), Resolution(1366, 768)];

/// Whether the base resolution fits on the desktop, in both dimensions
pub fn base_resolution_fits_desktop(base: Resolution, desktop: Resolution) -> bool {
    base.0 <= desktop.0 && base.1 <= desktop.1
}

/// Index into `BASE_RESOLUTIONS` of the base resolution the AUTO mode uses for
/// the desktop: the largest one that fits on it, or the smallest one if none
/// does (a desktop below the supported minimum).
pub fn auto_base_resolution_index(desktop: Resolution) -> usize {
    BASE_RESOLUTIONS
        .iter()
        .rposition(|&base| base_resolution_fits_desktop(base, desktop))
        .unwrap_or(0)
}

/// The base resolution the AUTO mode uses for the desktop, see `auto_base_resolution_index`
pub fn auto_base_resolution(desktop: Resolution) -> Resolution {
    BASE_RESOLUTIONS[auto_base_resolution_index(desktop)]
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn default_should_be_auto() {
        assert_eq!(ResolutionMode::default(), ResolutionMode::AUTO);
    }

    #[test]
    fn auto_base_resolution_should_pick_the_largest_base_that_fits() {
        macro_rules! t {
            ($w:expr, $h:expr, $expected:expr) => {
                assert_eq!(auto_base_resolution(Resolution($w, $h)), $expected);
            };
        }
        let small = Resolution(1280, 720);
        let large = Resolution(1366, 768);
        t!(1280, 720, small);
        t!(1280, 800, small);
        t!(1280, 1024, small);
        t!(1360, 768, small);
        t!(1366, 720, small);
        t!(1366, 768, large);
        t!(1600, 900, large);
        t!(1920, 1080, large);
        t!(1920, 1200, large);
        t!(2560, 1440, large);
        t!(3440, 1440, large);
        t!(3840, 2160, large);
    }

    #[test]
    fn auto_base_resolution_should_fall_back_to_the_smallest_base_below_the_minimum() {
        assert_eq!(
            auto_base_resolution(Resolution(1024, 768)),
            Resolution(1280, 720)
        );
        assert_eq!(
            auto_base_resolution(Resolution(640, 480)),
            Resolution(1280, 720)
        );
        assert_eq!(
            auto_base_resolution(Resolution(0, 0)),
            Resolution(1280, 720)
        );
    }

    #[test]
    fn base_resolution_fits_desktop_should_check_both_dimensions() {
        let large = Resolution(1366, 768);
        assert!(base_resolution_fits_desktop(large, Resolution(1366, 768)));
        assert!(!base_resolution_fits_desktop(large, Resolution(1360, 768)));
        assert!(!base_resolution_fits_desktop(large, Resolution(1920, 720)));
        assert!(!base_resolution_fits_desktop(
            Resolution(1280, 720),
            Resolution(1024, 768)
        ));
    }
}
