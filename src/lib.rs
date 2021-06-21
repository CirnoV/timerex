use std::cmp::Reverse;
use std::collections::{BTreeMap, BinaryHeap};
use std::ffi::c_void;
use std::sync::{Arc, RwLock};
use std::time::{Duration, Instant};

use once_cell::sync::Lazy;

static TIMER_MAP: Lazy<Arc<RwLock<BTreeMap<i32, TimerChannel>>>> = Lazy::new(|| Default::default());

#[derive(Default)]
pub struct TimerChannel {
    stopped: Option<Instant>,
    timers: BinaryHeap<Reverse<TimerDetail>>,
}

impl TimerChannel {
    fn append(&mut self, timer: TimerDetail) {
        self.timers.push(Reverse(timer));
    }
    fn update(&mut self) -> Option<Vec<TimerDetail>> {
        if let Some(_) = self.stopped {
            return None;
        }
        return None;
    }
    fn stop(&mut self) {
        if let Some(_) = self.stopped {
            self.resume();
        }
        self.stopped = Some(Instant::now());
    }
    fn resume(&mut self) {
        if let None = self.stopped {
            return;
        }
        self.stopped = None;
    }
}

#[repr(C)]
pub struct TimerDetail {
    hook: *const c_void,
    context: *const c_void,
    time: Instant,
    interval: Duration,
    user_data: i32,
    flags: i32,
}

impl Ord for TimerDetail {
    fn cmp(&self, other: &Self) -> std::cmp::Ordering {
        (self.time + self.interval).cmp(&(other.time + other.interval))
    }
}

impl PartialOrd for TimerDetail {
    fn partial_cmp(&self, other: &Self) -> Option<std::cmp::Ordering> {
        Some(self.cmp(other))
    }
}

impl PartialEq for TimerDetail {
    fn eq(&self, other: &Self) -> bool {
        self.time + self.interval == other.time + other.interval
    }
}

impl Eq for TimerDetail {}

unsafe impl Send for TimerDetail {}
unsafe impl Sync for TimerDetail {}

#[no_mangle]
pub extern "C" fn create_timer(
    hook: *const c_void,
    context: *const c_void,
    interval: u32,
    user_data: i32,
    flags: i32,
    channel: i32,
) {
    let t = TimerDetail {
        hook,
        context,
        time: Instant::now(),
        interval: Duration::from_millis(interval.into()),
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
        timer_list.append(t);
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
                    .map(|Reverse(t)| TimerInfo {
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
