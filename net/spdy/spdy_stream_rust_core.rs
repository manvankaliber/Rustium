// Copyright 2026 The Chromium Authors
// BSD-style license: see LICENSE.
#![forbid(unsafe_code)]
use crate::Services;

pub fn state_is_closed(state: i64) -> bool {
    state == 5
}

pub fn spdy_stream<S: Services>(s: &mut S) -> i64 {
    s.call(0, 0);
    0
}

pub fn destroy_spdy_stream<S: Services>(s: &mut S) -> i64 {
    s.call(0, 0);
    0
}

pub fn set_delegate<S: Services>(s: &mut S) -> i64 {
    s.call(0, 0);
    0
}

pub fn produce_headers_frame<S: Services>(s: &mut S) -> i64 {
    s.call(0, 0);
    let flags = if s.call(1, 0) == 1 { s.call(2, 0) } else { s.call(3, 0) };
    s.call(4, flags);
    s.call(5, 0);
    s.call(6, 0);
    0
}

pub fn detach_delegate<S: Services>(s: &mut S) -> i64 {
    s.call(0, 0);
    s.call(1, 0);
    s.call(2, 0);
    0
}

pub fn set_priority<S: Services>(s: &mut S) -> i64 {
    if s.call(0, 0) == s.call(1, 0) {
        return 0;
    }
    s.call(2, 0);
    s.call(3, 0);
    0
}

pub fn adjust_send_window_size<S: Services>(s: &mut S) -> i64 {
    if state_is_closed(s.call(0, 0)) {
        return 1;
    }
    let delta = s.call(1, 0);
    let current = s.call(2, 0);
    if delta > 0 {
        if current > i32::MAX as i64 - delta {
            return 0;
        }
    } else {
        s.call(3, 0);
    }
    s.call(4, current + delta);
    s.call(5, 0);
    s.call(6, 0);
    1
}

pub fn on_write_buffer_consumed<S: Services>(s: &mut S) -> i64 {
    if s.call(0, 0) == 1 {
        let amount = (s.call(1, 0) as u64).min(s.call(2, 0) as u64) as i64;
        s.call(3, amount);
        s.call(4, amount);
    }
    0
}

pub fn increase_send_window_size<S: Services>(s: &mut S) -> i64 {
    s.call(0, 0);
    if s.call(1, 0) == 0 {
        return 0;
    }
    if s.call(2, 0) == 0 {
        s.call(3, 0);
    }
    0
}

pub fn decrease_send_window_size<S: Services>(s: &mut S) -> i64 {
    if state_is_closed(s.call(0, 0)) {
        return 0;
    }
    s.call(1, 0);
    let value = s.call(2, 0) - s.call(3, 0);
    s.call(4, value);
    s.call(5, 0);
    0
}

pub fn on_read_buffer_consumed<S: Services>(s: &mut S) -> i64 {
    s.call(0, 0);
    s.call(1, 0);
    0
}

pub fn increase_recv_window_size<S: Services>(s: &mut S) -> i64 {
    if s.call(0, 0) == 0 {
        return 0;
    }
    s.call(1, 0);
    let value = s.call(2, 0) + s.call(3, 0);
    s.call(4, value);
    s.call(5, 0);
    let unacked = s.call(6, 0) + s.call(3, 0);
    s.call(7, unacked);
    s.call(8, 0);
    if s.call(6, 0) > s.call(9, 0) / 2 || s.call(10, 0) >= s.call(11, 0) {
        s.call(12, 0);
        s.call(13, 0);
        s.call(14, 0);
        if s.call(15, 0) == 0 {
            return 0;
        }
        s.call(7, 0);
    }
    0
}

pub fn decrease_recv_window_size<S: Services>(s: &mut S) -> i64 {
    s.call(0, 0);
    if s.call(1, 0) > s.call(2, 0) - s.call(3, 0) {
        s.call(4, 0);
        return 0;
    }
    let value = s.call(2, 0) - s.call(1, 0);
    s.call(5, value);
    s.call(6, 0);
    0
}

pub fn get_peer_address<S: Services>(s: &mut S) -> i64 {
    s.call(0, 0)
}

pub fn get_local_address<S: Services>(s: &mut S) -> i64 {
    s.call(0, 0)
}

pub fn was_ever_used<S: Services>(s: &mut S) -> i64 {
    s.call(0, 0)
}

pub fn get_s_s_l_info<S: Services>(s: &mut S) -> i64 {
    s.call(0, 0)
}

pub fn get_negotiated_protocol<S: Services>(s: &mut S) -> i64 {
    s.call(0, 0)
}

pub fn get_request_time<S: Services>(s: &mut S) -> i64 {
    s.call(0, 0);
    0
}

pub fn set_request_time<S: Services>(s: &mut S) -> i64 {
    s.call(0, 0);
    0
}

pub fn on_headers_received<S: Services>(s: &mut S) -> i64 {
    match s.call(0, 0) {
        0 => {
            s.call(1, 0);
            s.call(2, 0);
            if s.call(3, 0) == 0 {
                s.call(4, 0);
                return 0;
            }
            if s.call(5, 0) == 0 {
                s.call(6, 0);
                return 0;
            }
            let status = s.call(7, 0);
            s.call(8, 0);
            if s.call(9, 0) != 0 {
                s.call(10, 0);
            }
            if status / 100 != 1 {
                s.call(11, 0);
            }
            if status / 100 == 1 && status != 101 {
                if status == 103 {
                    s.call(12, 0);
                }
                return 0;
            }
            s.call(13, 1);
            let kind = s.call(14, 0);
            if (kind == s.call(15, 0) || kind == s.call(16, 0)) && s.call(17, 0) == 0 {
                s.call(18, 0);
                return 0;
            }
            s.call(19, 0);
            s.call(20, 0);
            s.call(21, 0);
        }
        1 => {
            s.call(13, 2);
            s.call(22, 0);
        }
        2 => {
            s.call(23, 0);
        }
        _ => {}
    }
    0
}

pub fn on_data_received<S: Services>(s: &mut S) -> i64 {
    s.call(0, 0);
    if s.call(1, 0) == 0 {
        s.call(2, 0);
        return 0;
    }
    if s.call(1, 0) == 2 && s.call(3, 0) != 0 {
        s.call(4, 0);
        return 0;
    }
    if s.call(5, 0) == 3 {
        s.call(6, 0);
        return 0;
    }
    let bytes = if s.call(3, 0) != 0 { s.call(7, 0) } else { 0 };
    let received = (s.call(8, 0) as i32).wrapping_add(bytes as i32);
    s.call(9, received as i64);
    s.call(10, 0);
    s.call(11, 0);
    if s.call(3, 0) == 0 {
        match s.call(5, 0) {
            1 => {
                s.call(12, 3);
                s.call(13, 0);
            }
            2 => {
                s.call(12, 5);
                s.call(14, 0);
            }
            _ => {
                s.call(15, 0);
            }
        }
        return 0;
    }
    let length = s.call(7, 0);
    s.call(16, length);
    s.call(17, 0);
    s.call(18, length);
    if s.call(19, 0) == 0 {
        return 0;
    }
    s.call(20, 0);
    s.call(21, 0);
    0
}

pub fn on_padding_consumed<S: Services>(s: &mut S) -> i64 {
    s.call(0, 0);
    s.call(1, 0);
    if s.call(2, 0) == 0 {
        return 0;
    }
    s.call(3, 0);
    0
}

pub fn on_frame_write_complete<S: Services>(s: &mut S) -> i64 {
    let headers = s.call(0, 0) == s.call(1, 0);
    if !headers && s.call(0, 0) != s.call(2, 0) {
        return 0;
    }
    let result = if headers { s.call(3, 0) } else { s.call(4, 0) };
    if result == s.call(5, 0) {
        return 0;
    }
    if s.call(6, 0) == 1 {
        match s.call(7, 0) {
            1 => {
                s.call(8, 2);
            }
            3 => {
                s.call(8, 5);
            }
            _ => {
                s.call(9, 0);
            }
        }
    }
    s.call(10, 0);
    s.call(11, 0);
    s.call(12, 1);
    if headers {
        s.call(13, 0);
    } else {
        s.call(14, 0);
    }
    s.call(15, 0);
    s.call(12, 0);
    s.call(16, 0);
    if s.call(7, 0) == 5 {
        s.call(17, 0);
    }
    0
}

pub fn on_headers_sent<S: Services>(s: &mut S) -> i64 {
    s.call(0, 0);
    s.call(1, 1);
    s.call(2, 0)
}

pub fn on_data_sent<S: Services>(s: &mut S) -> i64 {
    s.call(0, 0);
    let payload = (s.call(1, 0) as u64).wrapping_sub(s.call(2, 0) as u64) as i64;
    s.call(3, payload);
    s.call(4, payload);
    if s.call(5, 0) > 0 {
        s.call(6, 0);
        return s.call(7, 0);
    }
    s.call(8, 0);
    s.call(9, 0)
}

pub fn log_stream_error<S: Services>(s: &mut S) -> i64 {
    s.call(0, 0);
    0
}

pub fn on_close<S: Services>(s: &mut S) -> i64 {
    s.call(0, 5);
    let mut status = s.call(1, 0);
    if status == s.call(2, 0) {
        status = if s.call(3, 0) == 0 { s.call(4, 0) } else { s.call(5, 0) };
    }
    s.call(6, 0);
    s.call(7, 0);
    if s.call(8, 0) != 0 {
        s.call(9, status);
    }
    s.call(10, 0);
    0
}

pub fn cancel<S: Services>(s: &mut S) -> i64 {
    if s.call(0, 0) == 5 {
        return 0;
    }
    if s.call(1, 0) != 0 {
        s.call(2, 0);
    } else {
        s.call(3, 0);
    }
    0
}

pub fn close<S: Services>(s: &mut S) -> i64 {
    if s.call(0, 0) == 5 {
        return 0;
    }
    if s.call(1, 0) != 0 {
        s.call(2, 0);
    } else {
        s.call(3, 0);
    }
    0
}

pub fn get_weak_ptr<S: Services>(s: &mut S) -> i64 {
    s.call(0, 0);
    0
}

pub fn send_request_headers<S: Services>(s: &mut S) -> i64 {
    s.call(0, 0);
    s.call(1, 0);
    s.call(2, 0);
    s.call(3, 0);
    s.call(4, 0);
    s.call(5, 0);
    s.call(6, 0)
}

pub fn send_data<S: Services>(s: &mut S) -> i64 {
    s.call(0, 0);
    s.call(1, 0);
    s.call(2, 0);
    s.call(3, 0);
    0
}

pub fn possibly_resume_if_send_stalled<S: Services>(s: &mut S) -> i64 {
    let state = s.call(0, 0);
    if state == 2 || state == 5 || s.call(1, 0) == 0 {
        return s.call(2, 0);
    }
    if s.call(3, 0) != 0 || s.call(4, 0) <= 0 {
        return s.call(5, 0);
    }
    s.call(6, 0);
    s.call(7, 0);
    s.call(8, 0);
    s.call(2, 0)
}

pub fn is_closed<S: Services>(s: &mut S) -> i64 {
    state_is_closed(s.call(0, 0)) as i64
}

pub fn is_locally_closed<S: Services>(s: &mut S) -> i64 {
    let state = s.call(0, 0);
    (state == 2 || state == 5) as i64
}

pub fn is_remote_closed<S: Services>(s: &mut S) -> i64 {
    let state = s.call(0, 0);
    (state == 3) as i64
}

pub fn is_idle<S: Services>(s: &mut S) -> i64 {
    let state = s.call(0, 0);
    (state == 0) as i64
}

pub fn is_open<S: Services>(s: &mut S) -> i64 {
    let state = s.call(0, 0);
    (state == 1) as i64
}

pub fn is_reserved_remote<S: Services>(s: &mut S) -> i64 {
    let state = s.call(0, 0);
    (state == 4) as i64
}

pub fn add_raw_received_bytes<S: Services>(s: &mut S) -> i64 {
    s.call(0, 0);
    0
}

pub fn add_raw_sent_bytes<S: Services>(s: &mut S) -> i64 {
    s.call(0, 0);
    0
}

pub fn get_load_timing_info<S: Services>(s: &mut S) -> i64 {
    if s.call(0, 0) == 0 {
        return 0;
    }
    let result = s.call(1, 0);
    s.call(2, 0);
    result
}

pub fn get_info_as_value<S: Services>(s: &mut S) -> i64 {
    s.call(0, 0);
    s.call(1, 0);
    s.call(2, 0);
    s.call(3, 0);
    0
}

pub fn queue_next_data_frame<S: Services>(s: &mut S) -> i64 {
    s.call(0, 0);
    if s.call(1, 0) == 1 {
        s.call(2, 0);
    } else {
        s.call(3, 0);
    }
    let flags = if s.call(1, 0) == 1 { s.call(4, 0) } else { s.call(5, 0) };
    s.call(6, flags);
    if s.call(7, 0) == 0 {
        return 0;
    }
    s.call(8, 0);
    let payload = (s.call(9, 0) as u64).wrapping_sub(s.call(10, 0) as u64) as i64;
    s.call(11, payload);
    if payload != 0 {
        s.call(12, payload);
        s.call(13, payload);
    }
    if s.call(14, 0) != 0 && s.call(15, 0) != 0 && s.call(16, 0) != 0 {
        s.call(17, 0);
    }
    s.call(18, 0);
    s.call(19, 0);
    0
}

pub fn on_early_hints_received<S: Services>(s: &mut S) -> i64 {
    if s.call(0, 0) != 0 {
        s.call(1, 0);
    }
    if s.call(2, 0) != 0 {
        s.call(3, 0);
        return 0;
    }
    if s.call(4, 0) != s.call(5, 0) || s.call(6, 0) == 0 {
        s.call(7, 0);
        return 0;
    }
    s.call(8, 0);
    s.call(9, 0);
    0
}

pub fn save_response_headers<S: Services>(s: &mut S) -> i64 {
    if s.call(0, 0) != 0 {
        s.call(1, 0);
        return 0;
    }
    s.call(2, 0);
    s.call(3, 0);
    if s.call(4, 0) == 0 {
        return 0;
    }
    s.call(5, 0);
    0
}

pub fn describe_state<S: Services>(s: &mut S) -> i64 {
    let state = s.call(0, 0);
    match state {
        0 => {
            s.call(1, 0);
        }
        1 => {
            s.call(2, 0);
        }
        2 => {
            s.call(3, 0);
        }
        5 => {
            s.call(4, 0);
        }
        _ => {
            s.call(5, 0);
        }
    }
    0
}

pub fn headers_buffer_produce<S: Services>(s: &mut S) -> i64 {
    if s.call(0, 0) == 0 {
        s.call(1, 0);
    }
    s.call(2, 0);
    s.call(3, 0);
    0
}

pub fn dispatch<S: Services>(method: u32, services: &mut S) -> i64 {
    match method {
        0 => spdy_stream(services),
        1 => destroy_spdy_stream(services),
        2 => set_delegate(services),
        3 => produce_headers_frame(services),
        4 => detach_delegate(services),
        5 => set_priority(services),
        6 => adjust_send_window_size(services),
        7 => on_write_buffer_consumed(services),
        8 => increase_send_window_size(services),
        9 => decrease_send_window_size(services),
        10 => on_read_buffer_consumed(services),
        11 => increase_recv_window_size(services),
        12 => decrease_recv_window_size(services),
        13 => get_peer_address(services),
        14 => get_local_address(services),
        15 => was_ever_used(services),
        16 => get_s_s_l_info(services),
        17 => get_negotiated_protocol(services),
        18 => get_request_time(services),
        19 => set_request_time(services),
        20 => on_headers_received(services),
        21 => on_data_received(services),
        22 => on_padding_consumed(services),
        23 => on_frame_write_complete(services),
        24 => on_headers_sent(services),
        25 => on_data_sent(services),
        26 => log_stream_error(services),
        27 => on_close(services),
        28 => cancel(services),
        29 => close(services),
        30 => get_weak_ptr(services),
        31 => send_request_headers(services),
        32 => send_data(services),
        33 => possibly_resume_if_send_stalled(services),
        34 => is_closed(services),
        35 => is_locally_closed(services),
        36 => is_remote_closed(services),
        37 => is_idle(services),
        38 => is_open(services),
        39 => is_reserved_remote(services),
        40 => add_raw_received_bytes(services),
        41 => add_raw_sent_bytes(services),
        42 => get_load_timing_info(services),
        43 => get_info_as_value(services),
        44 => queue_next_data_frame(services),
        45 => on_early_hints_received(services),
        46 => save_response_headers(services),
        47 => describe_state(services),
        48 => headers_buffer_produce(services),
        _ => panic!("invalid private HTTP/2 family method"),
    }
}
