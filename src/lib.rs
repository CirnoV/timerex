#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]
#![feature(extract_if)]

use std::collections::BTreeMap;
use std::ffi;
use std::sync::Arc;
use std::sync::RwLock;

use once_cell::sync::Lazy;

pub(crate) mod timer;

use timer::{TimerChannel, TimerDetail, TimerInfo};

static TIMER_MAP: Lazy<Arc<RwLock<BTreeMap<i32, TimerChannel>>>> = Lazy::new(|| Default::default());

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
pub extern "C" fn create_timer(
    hook: *mut ffi::c_void,
    context: *mut ffi::c_void,
    identity: *mut ffi::c_void,
    interval: u32,
    user_data: i32,
    flags: i32,
    channel: i32,
) {
    let t = TimerDetail::new(hook, context, identity, interval, user_data, flags, channel);

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

#[no_mangle]
pub extern "C" fn drop_timer_arr(arr: *mut timer_arr) {
    unsafe {
        let arr = arr.as_ref().unwrap();
        Vec::from_raw_parts(arr.arr, arr.n, arr.cap);
    };
}

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
