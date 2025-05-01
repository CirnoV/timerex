#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]

use std::collections::BTreeMap;
use std::ffi;
use std::cell::RefCell;

pub(crate) mod timer;

use timer::{TimerChannel, TimerDetail, TimerInfo};

thread_local! {
    static TIMER_MAP: RefCell<BTreeMap<i32, TimerChannel>> = const { RefCell::new(BTreeMap::new()) };
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

/// # Safety
///
/// - The `hook`, `context`, and `identity` pointers must be valid (or null) for the lifetime of the timer. These pointers are stored but not dereferenced by this library.
/// - This function must only be called from the SourceMod main thread.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn create_timer(
    hook: *mut ffi::c_void,
    context: *mut ffi::c_void,
    identity: *mut ffi::c_void,
    interval: u32,
    user_data: i32,
    flags: i32,
    channel: i32,
) {
    let t = TimerDetail::new(hook, context, identity, interval, user_data, flags, channel);

    TIMER_MAP.with(|cell| {
        let mut timer_map = cell.borrow_mut();
        let timer_list = match timer_map.get_mut(&channel) {
            Some(v) => v,
            None => {
                timer_map.insert(channel, TimerChannel::default());
                timer_map.get_mut(&channel).unwrap()
            }
        };
        timer_list.append(t);
    });
}

/// # Safety
///
/// - The `arr` pointer must point to a valid `timer_arr` obtained from another function in this library.
/// - After this call, the `arr` pointer and the `arr` field within it become invalid as the memory is freed.
/// - This function must only be called from the SourceMod main thread.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn drop_timer_arr(arr: *mut timer_arr) {
    unsafe {
        let arr = arr.as_ref().unwrap();
        Vec::from_raw_parts(arr.arr, arr.n, arr.cap);
    };
}

/// # Safety
///
/// - This function must only be called from the SourceMod main thread.
/// - The returned `timer_arr` must be freed using `drop_timer_arr`.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn update_timer() -> timer_arr {
    let mut timers = TIMER_MAP.with(|cell| {
        let mut timer_map = cell.borrow_mut();
        timer_map
            .iter_mut()
            .filter_map(|(_key, channel): (&i32, &mut TimerChannel)| channel.update())
            .flatten()
            .map(|detail| detail.to_info())
            .collect::<Vec<_>>()
    });

    let output = timer_arr::from_vec(&mut timers);
    std::mem::forget(timers);
    output
}

/// # Safety
///
/// - The `channels` pointer must point to a valid array of `i32` values with length `len`.
/// - This function must only be called from the SourceMod main thread.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn pause_timer(channels: *mut i32, len: libc::size_t) {
    let channels = unsafe { std::slice::from_raw_parts(channels, len) };
    channels.iter().for_each(|&c| unsafe { pause_channel(c) })
}

/// # Safety
///
/// - This function must only be called from the SourceMod main thread.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn pause_channel(channel: i32) {
    TIMER_MAP.with(|cell| {
        if let Some(channel) = cell.borrow_mut().get_mut(&channel) {
            channel.pause()
        }
    });
}

/// # Safety
///
/// - The `channels` pointer must point to a valid array of `i32` values with length `len`.
/// - This function must only be called from the SourceMod main thread.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn resume_timer(channels: *mut i32, len: libc::size_t) {
    let channels = unsafe { std::slice::from_raw_parts(channels, len) };
    channels.iter().for_each(|&c| unsafe { resume_channel(c) })
}

/// # Safety
///
/// - This function must only be called from the SourceMod main thread.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn resume_timer_all() {
    TIMER_MAP.with(|cell| {
        for (_key, channel) in cell.borrow_mut().iter_mut() {
            channel.resume();
        }
    });
}

/// # Safety
///
/// - This function must only be called from the SourceMod main thread.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn resume_channel(channel: i32) {
    TIMER_MAP.with(|cell| {
        if let Some(channel) = cell.borrow_mut().get_mut(&channel) {
            channel.resume()
        }
    });
}

/// # Safety
///
/// - This function must only be called from the SourceMod main thread.
/// - The returned `timer_arr` must be freed using `drop_timer_arr`.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn remove_channel(channel: i32) -> timer_arr {
    let mut timers = TIMER_MAP.with(|cell| {
        let mut timer_map = cell.borrow_mut();
        match timer_map.remove(&channel) {
            Some(mut channel) => channel
                .clear()
                .into_iter()
                .map(|detail| detail.to_info())
                .collect(),
            None => Vec::new(),
        }
    });

    let output = timer_arr::from_vec(&mut timers);
    std::mem::forget(timers);
    output
}

/// # Safety
///
/// - This function must only be called from the SourceMod main thread.
/// - The returned `timer_arr` must be freed using `drop_timer_arr`.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn clear_timer() -> timer_arr {
    let mut timers = TIMER_MAP.with(|cell| {
        let mut timer_map = cell.borrow_mut();
        let timers = timer_map
            .values_mut()
            .flat_map(|channel| channel.clear())
            .map(|detail| detail.to_info())
            .collect::<Vec<_>>();
        timer_map.clear();
        timers
    });

    let output = timer_arr::from_vec(&mut timers);
    std::mem::forget(timers);
    output
}

/// # Safety
///
/// - This function must only be called from the SourceMod main thread.
/// - The returned `timer_arr` must be freed using `drop_timer_arr`.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn timer_mapchange() -> timer_arr {
    let mut timers = TIMER_MAP.with(|cell| {
        let mut timer_map = cell.borrow_mut();

        timer_map
            .values_mut()
            .flat_map(|channel| channel.handle_mapchange())
            .map(|detail| detail.to_info())
            .collect::<Vec<_>>()
    });

    let output = timer_arr::from_vec(&mut timers);
    std::mem::forget(timers);
    output
}

/// # Safety
///
/// - The `identity` pointer must be a valid plugin identity pointer (or null).
/// - This function must only be called from the SourceMod main thread.
/// - The returned `timer_arr` must be freed using `drop_timer_arr`.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn timer_pluginload(identity: *mut ffi::c_void) -> timer_arr {
    let mut timers = TIMER_MAP.with(|cell| {
        let mut timer_map = cell.borrow_mut();

        timer_map
            .values_mut()
            .flat_map(|channel| channel.handle_pluginload(identity))
            .map(|detail| detail.to_info())
            .collect::<Vec<_>>()
    });

    let output = timer_arr::from_vec(&mut timers);
    std::mem::forget(timers);
    output
}
