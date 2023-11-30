use std::ffi;
use std::time::{Duration, Instant};

use bitflags::bitflags;

bitflags! {
    #[derive(Default)]
    pub struct TimerFlags: i32 {
        const TIMER_REPEAT              = 1 << 0;
        const TIMER_FLAG_NO_MAPCHANGE   = 1 << 1;
        const TIMER_DATA_HNDL_CLOSE     = 1 << 9;
    }
}

const TIMERS_MIN_CAPACITY: usize = 1024;

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
    pub fn append(&mut self, timer: TimerDetail) {
        self.timers.push(timer);
    }
    pub fn update(&mut self) -> Option<Vec<TimerDetail>> {
        if let Some(_) = self.stopped {
            return None;
        }

        let elapsed_timers: Vec<TimerDetail> =
            self.timers.extract_if(|timer| timer.elapsed()).collect();
        if elapsed_timers.is_empty() {
            None
        } else {
            Some(elapsed_timers)
        }
    }
    pub fn pause(&mut self) {
        if let Some(_) = self.stopped {
            self.resume();
        }
        self.stopped = Some(Instant::now());
    }
    pub fn resume(&mut self) {
        if let Some(instant) = self.stopped {
            for timer in self.timers.iter_mut() {
                timer.interval += instant.elapsed();
            }
            self.stopped = None;
        }
    }
    pub fn clear(&mut self) -> Vec<TimerDetail> {
        self.timers.drain(..).collect()
    }
    pub fn handle_mapchange(&mut self) -> Vec<TimerDetail> {
        let drop_timers = self
            .timers
            .extract_if(|timer| timer.flags.contains(TimerFlags::TIMER_FLAG_NO_MAPCHANGE))
            .collect::<Vec<_>>();

        self.timers.shrink_to(TIMERS_MIN_CAPACITY);

        drop_timers
    }
    pub fn handle_pluginload(&mut self, identity: *mut ffi::c_void) -> Vec<TimerDetail> {
        let drop_timers = self
            .timers
            .extract_if(|timer| timer.identity == identity)
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
    pub fn new(
        hook: *mut ffi::c_void,
        context: *mut ffi::c_void,
        identity: *mut ffi::c_void,
        interval: u32,
        user_data: i32,
        flags: i32,
        channel: i32,
    ) -> Self {
        Self {
            hook,
            context,
            identity,
            time: Instant::now(),
            interval: Duration::from_millis(interval.into()),
            _interval: Duration::from_millis(interval.into()),
            user_data,
            flags: unsafe { TimerFlags::from_bits_unchecked(flags) },
            channel,
        }
    }

    pub fn elapsed(&self) -> bool {
        self.time.elapsed() >= self.interval
    }
}

unsafe impl Send for TimerDetail {}
unsafe impl Sync for TimerDetail {}

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
    pub fn to_info(self) -> TimerInfo {
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
