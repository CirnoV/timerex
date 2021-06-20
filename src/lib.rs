use std::collections::BTreeMap;
use std::ffi::c_void;
use std::sync::{Arc, RwLock};
use std::time::Instant;

use once_cell::sync::Lazy;

static TIMER_MAP: Lazy<Arc<RwLock<BTreeMap<i32, TimerChannel>>>> = Lazy::new(|| Default::default());

#[derive(Default)]
pub struct TimerChannel {
    stopped: bool,
    timers: Vec<TimerDetail>,
}

#[repr(C)]
pub struct TimerDetail {
    hook: *const c_void,
    context: *const c_void,
    time: Instant,
    interval: i32,
    user_data: i32,
    flags: i32,
}

unsafe impl Send for TimerDetail {}
unsafe impl Sync for TimerDetail {}

#[no_mangle]
pub extern "C" fn create_timer(
    hook: *const c_void,
    context: *const c_void,
    interval: i32,
    user_data: i32,
    flags: i32,
    channel: i32,
) {
    let t = TimerDetail {
        hook,
        context,
        time: Instant::now(),
        interval,
        user_data,
        flags,
    };

    {
        let mut timer_map = TIMER_MAP.write().unwrap();
        let timer_list = match timer_map.get_mut(&channel) {
            Some(v) => v,
            None => {
                timer_map.insert(channel, TimerChannel::default());
                timer_map.get_mut(&channel).unwrap()
            }
        };
        timer_list.timers.push(t);
    }
}

#[repr(C)]
pub struct TimerInfo {
    hook: *const c_void,
    context: *const c_void,
}

#[allow(improper_ctypes_definitions)]
#[no_mangle]
pub extern "C" fn update_timer() -> Vec<TimerInfo> {
    let mut timer_map = TIMER_MAP.write().unwrap();
    let timers = timer_map
        .iter_mut()
        .filter_map(|(_key, value): (&i32, &mut TimerChannel)| {
            if value.stopped {
                return None;
            }

            Some(
                value
                    .timers
                    .iter()
                    .map(|t| TimerInfo {
                        hook: t.hook,
                        context: t.context,
                    })
                    .collect::<Vec<_>>(),
            )
        })
        .flatten()
        .collect::<Vec<_>>();

    timers
}
