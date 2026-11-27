// Copyright 2026 The Chromium Authors
// BSD-style license: see LICENSE.
#![forbid(unsafe_code)]
use crate::Services;

pub fn upload_data<S: Services>(s: &mut S) -> i64 {
    s.call(40000, 0);
    (s.call(40001, 0) != 0 && ((s.call(40002, 0) as u64) > 0 || s.call(40003, 0) != 0)) as i64
}

pub fn spdy_http_stream<S: Services>(s: &mut S) -> i64 {
    s.call(0, 0);
    0
}

pub fn destroy_spdy_http_stream<S: Services>(s: &mut S) -> i64 {
    if s.call(0, 0) != 0 {
        s.call(1, 0);
        s.call(2, 0);
    }
    0
}

pub fn register_request<S: Services>(s: &mut S) -> i64 {
    s.call(0, 0);
    s.call(1, 0);
    0
}

pub fn initialize_stream<S: Services>(s: &mut S) -> i64 {
    s.call(0, 0);
    if s.call(1, 0) == 0 {
        return s.call(2, 0);
    }
    s.call(3, 0);
    let rv = s.call(4, 0);
    if rv == s.call(5, 0) {
        s.call(6, 0);
        s.call(7, 0);
    }
    rv
}

pub fn read_response_headers<S: Services>(s: &mut S) -> i64 {
    s.call(0, 0);
    if s.call(1, 0) != 0 {
        return s.call(2, 0);
    }
    s.call(3, 0);
    if s.call(4, 0) != 0 {
        s.call(5, 0);
        return s.call(6, 0);
    }
    s.call(7, 0);
    s.call(8, 0);
    s.call(9, 0)
}

pub fn read_response_body<S: Services>(s: &mut S) -> i64 {
    if s.call(0, 0) != 0 {
        s.call(1, 0);
    }
    s.call(2, 0);
    if s.call(3, 0) == 0 {
        s.call(4, 0);
        let rv = s.call(5, 0);
        if s.call(6, 0) == 0 {
            return s.call(7, 0);
        }
        return rv;
    } else if s.call(8, 0) != 0 {
        return s.call(9, 0);
    }
    s.call(10, 0);
    s.call(11, 0);
    s.call(12, 0);
    s.call(13, 0);
    s.call(14, 0)
}

pub fn close<S: Services>(s: &mut S) -> i64 {
    s.call(0, 0);
    s.call(1, 0);
    0
}

pub fn is_response_body_complete<S: Services>(s: &mut S) -> i64 {
    s.call(0, 0)
}

pub fn is_connection_reused<S: Services>(s: &mut S) -> i64 {
    s.call(0, 0)
}

pub fn get_total_received_bytes<S: Services>(s: &mut S) -> i64 {
    if s.call(0, 0) != 0 {
        s.call(1, 0);
        return 0;
    }
    if s.call(2, 0) == 0 {
        s.call(3, 0);
        return 0;
    }
    s.call(4, 0);
    0
}

pub fn get_total_sent_bytes<S: Services>(s: &mut S) -> i64 {
    if s.call(0, 0) != 0 {
        s.call(1, 0);
        return 0;
    }
    if s.call(2, 0) == 0 {
        s.call(3, 0);
        return 0;
    }
    s.call(4, 0);
    0
}

pub fn get_alternative_service<S: Services>(_s: &mut S) -> i64 {
    0
}

pub fn get_load_timing_info<S: Services>(s: &mut S) -> i64 {
    if s.call(0, 0) != 0 {
        if s.call(1, 0) == 0 {
            return 0;
        }
        s.call(2, 0);
    } else {
        if s.call(3, 0) == 0 || s.call(4, 0) == 0 {
            return 0;
        }
        if s.call(5, 0) == 0 {
            return 0;
        }
    }
    if s.call(6, 0) == 0 && s.call(7, 0) == 0 {
        s.call(8, 0);
        s.call(9, 0);
    }
    1
}

pub fn populate_load_timing_internal_info<S: Services>(s: &mut S) -> i64 {
    s.call(0, 0);
    s.call(1, 0);
    if s.call(2, 0) != 0 {
        s.call(3, 0);
    }
    0
}

pub fn send_request<S: Services>(s: &mut S) -> i64 {
    if s.call(0, 0) != 0 {
        return s.call(1, 0);
    }
    s.call(2, 0);
    s.call(3, 0);
    s.call(4, 0);
    if s.call(5, 0) != 0 {
        s.call(6, 0);
    }
    s.call(7, 0);
    if upload_data(s) != 0 {
        s.call(8, 0);
        s.call(9, 0);
    }
    s.call(10, 0);
    s.call(11, 0);
    s.call(12, 0);
    let result = s.call(13, 0);
    if result != s.call(14, 0) {
        s.call(15, 0);
        return result;
    }
    s.call(16, 0);
    s.call(17, 0);
    s.call(18, 0);
    s.call(19, 0);
    let will_send = upload_data(s) != 0 || s.call(20, 0) != 0;
    let send_status = if will_send { 0 } else { 1 };
    let result = s.call(21, send_status);
    if result == s.call(23, 0) {
        s.call(24, 0);
        s.call(25, 0);
    }
    s.call(22, 0);
    s.call(15, 0);
    result
}

pub fn cancel<S: Services>(s: &mut S) -> i64 {
    s.call(0, 0);
    s.call(1, 0);
    if s.call(2, 0) != 0 {
        s.call(3, 0);
        s.call(4, 0);
    }
    0
}

pub fn on_headers_sent<S: Services>(s: &mut S) -> i64 {
    if upload_data(s) != 0 {
        s.call(0, 0);
    } else if s.call(1, 0) != 0 {
        s.call(2, 0);
    } else {
        s.call(3, 0);
    }
    0
}

pub fn on_early_hints_received<S: Services>(s: &mut S) -> i64 {
    s.call(0, 0);
    let rv = s.call(1, 0);
    s.call(2, rv);
    if s.call(3, 0) != 0 {
        s.call(4, 0);
    }
    0
}

pub fn on_headers_received<S: Services>(s: &mut S) -> i64 {
    s.call(0, 0);
    s.call(1, 0);
    let rv = s.call(2, 0);
    s.call(3, rv);
    if rv == s.call(4, 0) || rv == s.call(5, 0) {
        s.call(6, rv);
        return 0;
    }
    s.call(7, 0);
    s.call(8, 0);
    s.call(9, 0);
    s.call(10, 0);
    s.call(11, 0);
    s.call(12, 0);
    if s.call(13, 0) == 0 {
        s.call(14, 0);
    }
    if s.call(15, 0) != 0 {
        s.call(16, 0);
    }
    0
}

pub fn on_data_received<S: Services>(s: &mut S) -> i64 {
    s.call(0, 0);
    if s.call(1, 0) != 0 {
        s.call(2, 0);
        s.call(3, 0);
    }
    0
}

pub fn on_data_sent<S: Services>(s: &mut S) -> i64 {
    if s.call(0, 0) != 0 && upload_data(s) != 0 {
        s.call(1, 0);
        s.call(2, 0);
    } else {
        s.call(3, 0);
        s.call(4, 0);
    }
    0
}

pub fn on_trailers<S: Services>(_s: &mut S) -> i64 {
    0
}

pub fn on_close<S: Services>(s: &mut S) -> i64 {
    s.call(0, 0);
    if s.call(1, 0) != 0 && s.call(2, 0) != 0 {
        s.call(3, 0);
    }
    s.call(4, 0);
    s.call(5, 0);
    s.call(6, 0);
    s.call(7, 0);
    s.call(8, 0);
    s.call(9, 0);
    s.call(10, 0);
    s.call(11, 0);
    if s.call(12, 0) != 0 {
        s.call(13, 0);
        if s.call(14, 0) == 0 {
            return 0;
        }
    }
    if s.call(15, 0) == s.call(16, 0) {
        s.call(17, 0);
        if s.call(14, 0) == 0 {
            return 0;
        }
    }
    if s.call(18, 0) != 0 {
        s.call(19, 0);
    }
    0
}

pub fn can_grease_frame_type<S: Services>(_s: &mut S) -> i64 {
    1
}

pub fn source_dependency<S: Services>(s: &mut S) -> i64 {
    s.call(0, 0);
    0
}

pub fn has_upload_data<S: Services>(s: &mut S) -> i64 {
    upload_data(s)
}

pub fn on_stream_created<S: Services>(s: &mut S) -> i64 {
    let rv = s.call(0, 0);
    if rv == s.call(1, 0) {
        s.call(2, 0);
        s.call(3, 0);
    }
    s.call(4, rv);
    0
}

pub fn read_and_send_request_body_data<S: Services>(s: &mut S) -> i64 {
    s.call(0, 0);
    s.call(1, 1);
    s.call(2, 0);
    if s.call(3, 0) != 0 {
        s.call(4, 0);
        s.call(1, 0);
        if s.call(5, 0) != 0 {
            s.call(6, 0);
        }
        return 0;
    }
    let rv = s.call(7, 0);
    if rv != s.call(8, 0) {
        s.call(9, rv);
    }
    0
}

pub fn send_empty_body<S: Services>(s: &mut S) -> i64 {
    s.call(0, 0);
    s.call(1, 0);
    s.call(2, 0);
    0
}

pub fn initialize_stream_helper<S: Services>(s: &mut S) -> i64 {
    s.call(0, 0);
    0
}

pub fn reset_stream<S: Services>(s: &mut S) -> i64 {
    s.call(0, 0);
    0
}

pub fn on_request_body_read_completed<S: Services>(s: &mut S) -> i64 {
    let status = s.call(0, 0);
    if status < 0 {
        s.call(1, 0);
        s.call(2, 0);
        return 0;
    }
    s.call(3, 0);
    s.call(4, status);
    let eof = s.call(5, 0) != 0;
    if eof {
        s.call(6, 0);
    } else {
        s.call(7, 0);
    }
    let send_status = if eof { 1 } else { 0 };
    s.call(8, send_status);
    0
}

pub fn maybe_schedule_buffered_read_callback<S: Services>(s: &mut S) -> i64 {
    s.call(0, 0);
    if s.call(1, 0) == 0 {
        return 0;
    }
    if (s.call(2, 0) as u64) >= (s.call(3, 0) as u64) {
        s.call(4, 0);
        return 0;
    }
    s.call(5, 0);
    0
}

pub fn do_buffered_read_callback<S: Services>(s: &mut S) -> i64 {
    s.call(0, 0);
    if s.call(1, 0) != 0 && s.call(2, 0) != s.call(3, 0) {
        if s.call(4, 0) != 0 {
            s.call(5, 0);
        }
        return 0;
    }
    if s.call(6, 0) == 0 {
        return 0;
    }
    if s.call(7, 0) == 0 {
        s.call(8, 0);
        let rv = s.call(9, 0);
        if s.call(10, 0) == 0 {
            return 0;
        }
        s.call(11, 0);
        s.call(12, 0);
        if s.call(4, 0) != 0 {
            s.call(13, rv);
        }
        return 0;
    }
    if s.call(1, 0) != 0 && s.call(4, 0) != 0 {
        s.call(5, 0);
    }
    0
}

pub fn do_request_callback<S: Services>(s: &mut S) -> i64 {
    s.call(0, 0);
    s.call(1, 0);
    0
}

pub fn maybe_do_request_callback<S: Services>(s: &mut S) -> i64 {
    s.call(0, 0);
    if s.call(1, 0) != 0 {
        s.call(2, 0);
    }
    0
}

pub fn maybe_post_request_callback<S: Services>(s: &mut S) -> i64 {
    s.call(0, 0);
    if s.call(1, 0) != 0 {
        s.call(2, 0);
    }
    0
}

pub fn do_response_callback<S: Services>(s: &mut S) -> i64 {
    s.call(0, 0);
    s.call(1, 0);
    0
}

pub fn get_remote_endpoint<S: Services>(s: &mut S) -> i64 {
    if s.call(0, 0) != 0 {
        return s.call(1, 0);
    }
    if s.call(2, 0) == 0 {
        return s.call(3, 0);
    }
    s.call(4, 0)
}

pub fn populate_net_error_details<S: Services>(s: &mut S) -> i64 {
    s.call(0, 0);
    0
}

pub fn set_priority<S: Services>(s: &mut S) -> i64 {
    s.call(0, 0);
    if s.call(1, 0) != 0 {
        s.call(2, 0);
    }
    0
}

pub fn get_dns_aliases<S: Services>(s: &mut S) -> i64 {
    s.call(0, 0);
    0
}

pub fn get_accept_ch_via_alps<S: Services>(s: &mut S) -> i64 {
    if s.call(0, 0) == 0 {
        return 0;
    }
    s.call(1, 0);
    0
}

pub fn set_h_t_t_p11_required<S: Services>(s: &mut S) -> i64 {
    if s.call(0, 0) != 0 {
        s.call(1, 0);
    }
    0
}

pub fn dispatch<S: Services>(method: u32, services: &mut S) -> i64 {
    match method {
        0 => spdy_http_stream(services),
        1 => destroy_spdy_http_stream(services),
        2 => register_request(services),
        3 => initialize_stream(services),
        4 => read_response_headers(services),
        5 => read_response_body(services),
        6 => close(services),
        7 => is_response_body_complete(services),
        8 => is_connection_reused(services),
        9 => get_total_received_bytes(services),
        10 => get_total_sent_bytes(services),
        11 => get_alternative_service(services),
        12 => get_load_timing_info(services),
        13 => populate_load_timing_internal_info(services),
        14 => send_request(services),
        15 => cancel(services),
        16 => on_headers_sent(services),
        17 => on_early_hints_received(services),
        18 => on_headers_received(services),
        19 => on_data_received(services),
        20 => on_data_sent(services),
        21 => on_trailers(services),
        22 => on_close(services),
        23 => can_grease_frame_type(services),
        24 => source_dependency(services),
        25 => has_upload_data(services),
        26 => on_stream_created(services),
        27 => read_and_send_request_body_data(services),
        28 => send_empty_body(services),
        29 => initialize_stream_helper(services),
        30 => reset_stream(services),
        31 => on_request_body_read_completed(services),
        32 => maybe_schedule_buffered_read_callback(services),
        33 => do_buffered_read_callback(services),
        34 => do_request_callback(services),
        35 => maybe_do_request_callback(services),
        36 => maybe_post_request_callback(services),
        37 => do_response_callback(services),
        38 => get_remote_endpoint(services),
        39 => populate_net_error_details(services),
        40 => set_priority(services),
        41 => get_dns_aliases(services),
        42 => get_accept_ch_via_alps(services),
        43 => set_h_t_t_p11_required(services),
        _ => panic!("invalid private HTTP/2 family method"),
    }
}
