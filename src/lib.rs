#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]
#![feature(drain_filter)]
#![feature(shrink_to)]

use std::collections::BTreeMap;
use std::ffi;
use std::sync::{Arc, RwLock};
use std::time::{Duration, Instant};

use bitflags::bitflags;
use once_cell::sync::Lazy;

bitflags! {
    #[derive(Default)]
    struct TimerFlags: i32 {
        const TIMER_REPEAT              = 1 << 0;
        const TIMER_FLAG_NO_MAPCHANGE   = 1 << 1;
        const TIMER_DATA_HNDL_CLOSE     = 1 << 9;
    }
}

const TIMERS_MIN_CAPACITY: usize = 1024;
static TIMER_MAP: Lazy<Arc<RwLock<BTreeMap<i32, TimerChannel>>>> = Lazy::new(|| Default::default());

pub struct TimerChannel {
    stopped: Option<Instant>,
    timers: Vec<TimerDetail>,
}

impl Default for TimerChannel {
    fn default() -> Self {
        Self {
            stopped: None,
            timers: Vec::with_capacity(TIMERS_MIN_CAPACITY),
        }
    }
}

impl TimerChannel {
    fn append(&mut self, timer: TimerDetail) {
        self.timers.push(timer);
    }
    fn update(&mut self) -> Option<Vec<TimerDetail>> {
        if let Some(_) = self.stopped {
            return None;
        }

        let elapsed_timers: Vec<TimerDetail> =
            self.timers.drain_filter(|timer| timer.elapsed()).collect();
        if elapsed_timers.is_empty() {
            None
        } else {
            Some(elapsed_timers)
        }
    }
    fn pause(&mut self) {
        if let Some(_) = self.stopped {
            self.resume();
        }
        self.stopped = Some(Instant::now());
    }
    fn resume(&mut self) {
        if let Some(instant) = self.stopped {
            for timer in self.timers.iter_mut() {
                timer.interval += instant.elapsed();
            }
            self.stopped = None;
        }
    }
    fn clear(&mut self) -> Vec<TimerDetail> {
        self.timers.drain(..).collect()
    }
    fn handle_mapchange(&mut self) -> Vec<TimerDetail> {
        let drop_timers = self
            .timers
            .drain_filter(|timer| timer.flags.contains(TimerFlags::TIMER_FLAG_NO_MAPCHANGE))
            .collect::<Vec<_>>();

        self.timers.shrink_to(TIMERS_MIN_CAPACITY);

        drop_timers
    }
    fn handle_pluginload(&mut self, identity: *mut ffi::c_void) -> Vec<TimerDetail> {
        let drop_timers = self
            .timers
            .drain_filter(|timer| timer.identity == identity)
            .collect::<Vec<_>>();

        self.timers.shrink_to(TIMERS_MIN_CAPACITY);

        drop_timers
    }
}

#[derive(Clone, Copy)]
#[repr(C)]
pub struct TimerDetail {
    hook: *mut ffi::c_void,
    context: *mut ffi::c_void,
    identity: *mut ffi::c_void,
    time: Instant,
    interval: Duration,
    _interval: Duration,
    user_data: i32,
    flags: TimerFlags,
    channel: i32,
}

impl TimerDetail {
    fn elapsed(&self) -> bool {
        self.time.elapsed() >= self.interval
    }
}

impl Ord for TimerDetail {
    fn cmp(&self, other: &Self) -> std::cmp::Ordering {
        (self.time.elapsed() + self.interval).cmp(&(other.time.elapsed() + other.interval))
    }
}

impl PartialOrd for TimerDetail {
    fn partial_cmp(&self, other: &Self) -> Option<std::cmp::Ordering> {
        Some(self.cmp(other))
    }
}

impl PartialEq for TimerDetail {
    fn eq(&self, other: &Self) -> bool {
        self.time.elapsed() + self.interval == other.time.elapsed() + other.interval
    }
}

impl Eq for TimerDetail {}

unsafe impl Send for TimerDetail {}
unsafe impl Sync for TimerDetail {}

#[no_mangle]
pub extern "C" fn create_timer(
    hook: *mut ffi::c_void,
    context: *mut ffi::c_void,
    identity: *mut ffi::c_void,
    interval: u32,
    user_data: i32,
    flags: i32,
    channel: i32,
) {
    let t = TimerDetail {
        hook,
        context,
        identity,
        time: Instant::now(),
        interval: Duration::from_millis(interval.into()),
        _interval: Duration::from_millis(interval.into()),
        user_data,
        flags: unsafe { TimerFlags::from_bits_unchecked(flags) },
        channel,
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
    hook: *mut ffi::c_void,
    context: *mut ffi::c_void,
    identity: *mut ffi::c_void,
    interval: u32,
    user_data: i32,
    flags: i32,
    channel: i32,
}

impl TimerDetail {
    fn to_info(self) -> TimerInfo {
        TimerInfo {
            hook: self.hook,
            context: self.context,
            identity: self.identity,
            interval: self._interval.as_millis() as u32,
            user_data: self.user_data,
            flags: self.flags.bits(),
            channel: self.channel,
        }
    }
}

impl From<&TimerDetail> for TimerInfo {
    fn from(detail: &TimerDetail) -> Self {
        Self {
            hook: detail.hook,
            context: detail.context,
            identity: detail.identity,
            interval: detail._interval.as_millis() as u32,
            user_data: detail.user_data,
            flags: detail.flags.bits(),
            channel: detail.channel,
        }
    }
}

#[repr(C)]
pub struct timer_arr {
    arr: *mut TimerInfo,
    n: usize,
    cap: usize,
}

impl timer_arr {
    fn from_vec(vec: &mut Vec<TimerInfo>) -> Self {
        timer_arr {
            arr: vec.as_mut_ptr(),
            n: vec.len(),
            cap: vec.capacity(),
        }
    }
}

#[no_mangle]
pub extern "C" fn drop_timer_arr(arr: *mut timer_arr) {
    unsafe {
        let arr = arr.as_ref().unwrap();
        Vec::from_raw_parts(arr.arr, arr.n, arr.cap);
    };
}

// #[allow(improper_ctypes_definitions)]
#[no_mangle]
pub extern "C" fn update_timer() -> timer_arr {
    let mut timer_map = TIMER_MAP.write().unwrap();
    let mut timers = timer_map
        .iter_mut()
        .filter_map(|(_key, channel): (&i32, &mut TimerChannel)| channel.update())
        .flatten()
        .map(|detail| detail.to_info())
        .collect::<Vec<_>>();

    let output = timer_arr::from_vec(&mut timers);
    std::mem::forget(timers);
    output
}

#[no_mangle]
pub extern "C" fn pause_timer(channels: *mut i32, len: libc::size_t) {
    let channels = unsafe { std::slice::from_raw_parts(channels, len) };
    channels.iter().for_each(|&c| pause_channel(c))
}

#[no_mangle]
pub extern "C" fn pause_channel(channel: i32) {
    if let Some(channel) = TIMER_MAP.write().unwrap().get_mut(&channel) {
        channel.pause()
    }
}

#[no_mangle]
pub extern "C" fn resume_timer(channels: *mut i32, len: libc::size_t) {
    let channels = unsafe { std::slice::from_raw_parts(channels, len) };
    channels.iter().for_each(|&c| resume_channel(c))
}

#[no_mangle]
pub extern "C" fn resume_timer_all() {
    for (_key, channel) in TIMER_MAP.write().unwrap().iter_mut() {
        channel.resume();
    }
}

#[no_mangle]
pub extern "C" fn resume_channel(channel: i32) {
    if let Some(channel) = TIMER_MAP.write().unwrap().get_mut(&channel) {
        channel.resume()
    }
}

#[no_mangle]
pub extern "C" fn remove_channel(channel: i32) -> timer_arr {
    let mut timers = {
        let mut timer_map = TIMER_MAP.write().unwrap();
        match timer_map.remove(&channel) {
            Some(mut channel) => channel
                .clear()
                .into_iter()
                .map(|detail| detail.to_info())
                .collect(),
            None => Vec::new(),
        }
    };

    let output = timer_arr::from_vec(&mut timers);
    std::mem::forget(timers);
    output
}

#[no_mangle]
pub extern "C" fn clear_timer() -> timer_arr {
    let mut timers = {
        let mut timer_map = TIMER_MAP.write().unwrap();
        let timers = timer_map
            .values_mut()
            .flat_map(|channel| channel.clear())
            .map(|detail| detail.to_info())
            .collect::<Vec<_>>();
        timer_map.clear();
        timers
    };

    let output = timer_arr::from_vec(&mut timers);
    std::mem::forget(timers);
    output
}

#[no_mangle]
pub extern "C" fn timer_mapchange() -> timer_arr {
    let mut timers = {
        let mut timer_map = TIMER_MAP.write().unwrap();
        let timers = timer_map
            .values_mut()
            .flat_map(|channel| channel.handle_mapchange())
            .map(|detail| detail.to_info())
            .collect::<Vec<_>>();
        timers
    };

    let output = timer_arr::from_vec(&mut timers);
    std::mem::forget(timers);
    output
}

#[no_mangle]
pub extern "C" fn timer_pluginload(identity: *mut ffi::c_void) -> timer_arr {
    let mut timers = {
        let mut timer_map = TIMER_MAP.write().unwrap();
        let timers = timer_map
            .values_mut()
            .flat_map(|channel| channel.handle_pluginload(identity))
            .map(|detail| detail.to_info())
            .collect::<Vec<_>>();
        timers
    };

    let output = timer_arr::from_vec(&mut timers);
    std::mem::forget(timers);
    output
}
