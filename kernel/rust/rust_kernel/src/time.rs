#[repr(C)]
pub struct RtcDate {
    second: u64,
    minute: u64,
    hour: u64,
    day: u64,
    month: u64,
    year: u64,
}

const fn is_leap_year(year: u64) -> bool {
    year % 4 == 0 && (year % 100 != 0 || year % 400 == 0)
}
const fn day_of_year(year: u64, month: u64, day: u64) -> u64 {
    if !(month >= 1 && month <= 12) {}
    const SEEK_TABLE: [u64; 12] = [0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334];
    let mut day_of_year = SEEK_TABLE[month as usize - 1] + day - 1;

    if is_leap_year(year) && month >= 3 {
        day_of_year += 1;
    }
    day_of_year
}

const fn days_in_year(year: u64) -> u64 {
    365 + (is_leap_year(year) as u64)
}
const fn floor_div_by<const DIVISOR: u64>(dividend: u64) -> u64 {
    assert!(DIVISOR > 1);
    dividend / DIVISOR
}

const fn mod_zeroes_in_range<const POSITIVE_MOD: u64>(begin: u64, end: u64) -> u64 {
    floor_div_by::<POSITIVE_MOD>(end - 1) - floor_div_by::<POSITIVE_MOD>(begin - 1)
}

const fn years_to_days_since_epoch(year: u64) -> u64 {
    let begin_year: u64;
    let end_year: u64;
    let leap_sign: i64;
    if year < 1970 {
        begin_year = year;
        end_year = 1970;
        leap_sign = -1;
    } else {
        begin_year = 1970;
        end_year = year;
        leap_sign = 1;
    }
    let days: u64 = 365 * (year - 1970);
    let mut extra_leap_days: u64 = 0;
    extra_leap_days += mod_zeroes_in_range::<4>(begin_year, end_year);
    extra_leap_days += mod_zeroes_in_range::<100>(begin_year, end_year);
    extra_leap_days += mod_zeroes_in_range::<400>(begin_year, end_year);
    days + ((extra_leap_days as i64 * leap_sign) as u64)
}

const fn days_since_epoch(year: u64, month: u64, day: u64) -> u64 {
    years_to_days_since_epoch(year) + day_of_year(year, month, day)
}
const fn round_down(value: f64) -> u64 {
    value as u64
}
const fn seconds_since_epoch_to_year(seconds: u64) -> u64 {
    const SECONDS_PER_YEAR: f64 = 60.0 * 60.0 * 24.0 * 365.2425;
    let years_since_epoch = seconds as f64 / SECONDS_PER_YEAR;
    1970 + round_down(years_since_epoch)
}

#[unsafe(no_mangle)]
pub extern "C" fn rtc_to_epoch(rtc: RtcDate) -> u64 {
    let days = days_since_epoch(rtc.year, rtc.month, rtc.day);
    ((days * 24 + rtc.hour) * 60 + rtc.minute) * 60 + rtc.second
}
