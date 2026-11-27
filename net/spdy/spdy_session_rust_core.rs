// Copyright 2026 The Chromium Authors
// BSD-style license; see LICENSE.
#![forbid(unsafe_code)]
pub trait Services {
    fn op(&mut self, operation: u32, a: u64, b: u64, c: u64) -> i64;
}
macro_rules! op {
    ($p:expr,$n:expr) => {$p.op($n,0,0,0)};
    ($p:expr,$n:expr,$a:expr) => {{let arg_a=$a; $p.op($n,arg_a,0,0)}};
    ($p:expr,$n:expr,$a:expr,$b:expr) => {{let arg_a=$a; let arg_b=$b; $p.op($n,arg_a,arg_b,0)}};
}

/// HTTP/2 protocol defaults, guarded by C++ static assertions at integration.
pub fn setting_is_default(id: u32, value: u32) -> bool {
    match id {
        1 => value == 4096,
        2 => value == 1,
        4 => value == 65535,
        5 => value == 16384,
        8 => value == 0,
        _ => false,
    }
}

/// SpdySession::ShouldSendHttp2Priority; native action contract: session-operations.json.
pub fn should_send_http2_priority(p: &mut impl Services) -> i64 {
    i64::from(op!(p,0) == 0 || op!(p,1) == 0)
}

/// SpdySession::ShouldSendPriorityUpdate; native action contract: session-operations.json.
pub fn should_send_priority_update(p: &mut impl Services) -> i64 {
    if op!(p,0) == 0 {
        return 0;
    }
    if op!(p,1) != 0 {
        op!(p,2)
    } else {
        1
    }
}

/// SpdySession::VerifyDomainAuthentication; native action contract: session-operations.json.
pub fn verify_domain_authentication(p: &mut impl Services) -> i64 {
    if op!(p,0) != 0 {
        return 0;
    }
    if op!(p,1) == 0 {
        return 1;
    }
    op!(p,2)
}

/// SpdySession::ConfirmHandshake; native action contract: session-operations.json.
pub fn confirm_handshake(p: &mut impl Services) -> i64 {
    if op!(p,0) != 0 || op!(p,1) != 0 {
        return op!(p,2);
    }
    if op!(p,3) != 0 {
        return op!(p,4);
    }
    let rv = if op!(p,5) == 0 { op!(p,6) } else { -1 };
    if rv == -1 {
        op!(p,7);
    }
    rv
}

/// SpdySession::MakeUnavailable; native action contract: session-operations.json.
pub fn make_unavailable(p: &mut impl Services) -> i64 {
    if op!(p,0) != 0 {
        op!(p,1);
        op!(p,2);
    }
    0
}

/// SpdySession::StartGoingAway; native action contract: session-operations.json.
pub fn start_going_away(p: &mut impl Services) -> i64 {
    op!(p,0);
    op!(p,1);
    while op!(p,2) != 0 {
        op!(p,3);
        op!(p,4);
        op!(p,17);
    }
    while op!(p,5) != 0 {
        op!(p,6);
        op!(p,7);
        op!(p,8);
    }
    while op!(p,9) != 0 {
        op!(p,10);
        op!(p,11);
        op!(p,12);
        op!(p,13);
    }
    op!(p,14);
    op!(p,15);
    op!(p,16);
    0
}

/// SpdySession::MaybeFinishGoingAway; native action contract: session-operations.json.
pub fn maybe_finish_going_away(p: &mut impl Services) -> i64 {
    if op!(p,0) != 0 && op!(p,1) != 0 && op!(p,2) != 0 && op!(p,3) == 0 {
        op!(p,4);
    }
    0
}

/// SpdySession::TryCreateStream; native action contract: session-operations.json.
pub fn try_create_stream(p: &mut impl Services) -> i64 {
    op!(p,0);
    if op!(p,1) != 0 || op!(p,2) != 0 {
        return op!(p,3);
    }
    if op!(p,4) != 0 || op!(p,6) != 0 {
        return op!(p,5);
    }
    if (op!(p,7) as u64).wrapping_add(op!(p,8) as u64) < op!(p,9) as u64 {
        return op!(p,10);
    }
    op!(p,11);
    op!(p,12);
    -1
}

/// SpdySession::CreateStream; native action contract: session-operations.json.
pub fn create_stream(p: &mut impl Services) -> i64 {
    op!(p,0);
    op!(p,1);
    if op!(p,2) == 0 {
        return op!(p,3);
    }
    op!(p,4);
    op!(p,5);
    if op!(p,6) != 0 {
        op!(p,7);
    }
    0
}

/// SpdySession::ChangeStreamRequestPriority; native action contract: session-operations.json.
pub fn change_stream_request_priority(p: &mut impl Services) -> i64 {
    op!(p,0);
    if op!(p,1) != 0 {
        op!(p,2);
    }
    0
}

/// SpdySession::GetNextPendingStreamRequest; native action contract: session-operations.json.
pub fn get_next_pending_stream_request(p: &mut impl Services) -> i64 {
    let mut j = op!(p,0);
    let min = op!(p,1);
    while j >= min {
        if op!(p,2,j as u64) != 0 {
            j -= 1;
            continue;
        }
        op!(p,3,j as u64);
        if op!(p,4) != 0 {
            op!(p,5);
        }
        op!(p,6);
        return 0;
    }
    0
}

/// SpdySession::ProcessPendingStreamRequests; native action contract: session-operations.json.
pub fn process_pending_stream_requests(p: &mut impl Services) -> i64 {
    if op!(p,0) == 0 {
        return 0;
    }
    let n = (op!(p,1) as u64).wrapping_sub((op!(p,2) as u64).wrapping_add(op!(p,3) as u64));
    for _ in 0..n {
        if op!(p,4) == 0 {
            break;
        }
        op!(p,5);
    }
    0
}

/// SpdySession::PumpReadLoop; native action contract: session-operations.json.
pub fn pump_read_loop(p: &mut impl Services) -> i64 {
    op!(p,0);
    if op!(p,1) != 0 || op!(p,2) != 0 {
        return 0;
    }
    op!(p,3);
    0
}

/// SpdySession::DoReadLoop; native action contract: session-operations.json.
pub fn do_read_loop(p: &mut impl Services) -> i64 {
    op!(p,0);
    let mut result = op!(p,1);
    let mut bytes = 0_i32;
    loop {
        if op!(p,2) != 0 {
            result = op!(p,3);
        } else if op!(p,4) != 0 {
            if result > 0 {
                bytes = bytes.wrapping_add(result as i32);
            }
            result = op!(p,5);
        } else {
            op!(p,6);
        }
        if op!(p,7) != 0 || op!(p,8) != 0 || result == -1 {
            break;
        }
        if op!(p,2) != 0 && (i64::from(bytes) > op!(p,9) || op!(p,10) != 0) {
            op!(p,11);
            result = -1;
            break;
        }
    }
    op!(p,12);
    result
}

/// SpdySession::DoRead; native action contract: session-operations.json.
pub fn do_read(p: &mut impl Services) -> i64 {
    op!(p,0);
    op!(p,1);
    let rv = op!(p,2);
    if rv == -1 {
        op!(p,3);
        return rv;
    }
    if rv == op!(p,4) {
        return op!(p,5);
    }
    rv
}

/// SpdySession::DoReadComplete; native action contract: session-operations.json.
pub fn do_read_complete(p: &mut impl Services) -> i64 {
    let result = op!(p,0);
    if result == 0 {
        return op!(p,1);
    }
    if result < 0 {
        op!(p,2);
        return result;
    }
    op!(p,3);
    while op!(p,4) != 0 {
        op!(p,5);
        if op!(p,6) != 0 || op!(p,7) != 0 {
            return op!(p,8);
        }
        op!(p,9);
    }
    op!(p,10);
    0
}

/// SpdySession::PumpWriteLoop; native action contract: session-operations.json.
pub fn pump_write_loop(p: &mut impl Services) -> i64 {
    op!(p,0);
    op!(p,1);
    if op!(p,2) != 0 && op!(p,3) == 0 && op!(p,4) != 0 {
        op!(p,5);
    }
    0
}

/// SpdySession::MaybePostWriteLoop; native action contract: session-operations.json.
pub fn maybe_post_write_loop(p: &mut impl Services) -> i64 {
    if op!(p,0) != 0 {
        op!(p,1);
        op!(p,2);
    }
    0
}

/// SpdySession::DoWriteLoop; native action contract: session-operations.json.
pub fn do_write_loop(p: &mut impl Services) -> i64 {
    op!(p,0);
    let mut result = op!(p,1);
    loop {
        if op!(p,2) != 0 {
            result = op!(p,3);
        } else if op!(p,4) != 0 {
            result = op!(p,5);
        } else {
            op!(p,6);
        }
        if op!(p,7) != 0 {
            op!(p,8);
            break;
        }
        if result == -1 {
            break;
        }
    }
    op!(p,9);
    result
}

/// SpdySession::DoWrite; native action contract: session-operations.json.
pub fn do_write(p: &mut impl Services) -> i64 {
    op!(p,0);
    if op!(p,1) != 0 {
        op!(p,2);
    } else {
        if op!(p,3) == 0 {
            op!(p,4);
            return -1;
        }
        if op!(p,5) != 0 {
            op!(p,6);
        }
        if op!(p,7) != 0 {
            op!(p,8);
            op!(p,9);
            if op!(p,10) != 0 {
                op!(p,11);
                op!(p,12);
                op!(p,13);
            }
        }
        if op!(p,14) == 0 {
            op!(p,15);
        }
        op!(p,16);
        op!(p,17);
    }
    op!(p,18);
    op!(p,19)
}

/// SpdySession::DoWriteComplete; native action contract: session-operations.json.
pub fn do_write_complete(p: &mut impl Services) -> i64 {
    let result = op!(p,0);
    if result < 0 {
        op!(p,1);
        op!(p,2);
        return 0;
    }
    op!(p,3);
    if result > 0 {
        op!(p,4);
        if op!(p,5) != 0 {
            op!(p,6);
        }
        if op!(p,7) == 0 {
            if op!(p,5) != 0 {
                op!(p,8);
            }
            op!(p,9);
        }
    }
    op!(p,10);
    0
}

/// SpdySession::NotifyRequestsOfConfirmation; native action contract: session-operations.json.
pub fn notify_requests_of_confirmation(p: &mut impl Services) -> i64 {
    let n = op!(p,0) as u64;
    for i in 0..n {
        op!(p,1,i);
    }
    op!(p,2);
    op!(p,3);
    0
}

/// SpdySession::HandleSetting; native action contract: session-operations.json.
pub fn handle_setting(p: &mut impl Services) -> i64 {
    let id = op!(p,0);
    let value = op!(p,1) as u32;
    if id == op!(p,19) {
        op!(p,2);
    } else if id == op!(p,20) {
        let max = op!(p,3) as u64;
        op!(p,4,u64::from(value).min(max));
        op!(p,5);
    } else if id == op!(p,21) {
        if value > i32::MAX as u32 {
            op!(p,6);
            return 0;
        }
        let delta = (value as i32).wrapping_sub(op!(p,7) as i32);
        op!(p,8,delta as u64);
        op!(p,9);
        op!(p,10);
    } else if id == op!(p,22) {
        if value > 1 || (op!(p,11) != 0 && value == 0) {
            op!(p,12);
            return 0;
        }
        if value == 1 {
            op!(p,13);
        }
    } else if id == op!(p,23) {
        if value > 1 {
            op!(p,16);
            return 0;
        }
        if op!(p,14) != 0 {
            if value != u32::from(op!(p,15) != 0) {
                op!(p,17);
                return 0;
            }
        } else if value == 1 {
            op!(p,18);
        }
    }
    0
}

/// SpdySession::UpdateStreamsSendWindowSize; native action contract: session-operations.json.
pub fn update_streams_send_window_size(p: &mut impl Services) -> i64 {
    op!(p,0);
    while op!(p,1) != 0 {
        if op!(p,2) == 0 {
            op!(p,3);
            return 0;
        }
        op!(p,4);
    }
    op!(p,5);
    while op!(p,6) != 0 {
        if op!(p,7) == 0 {
            op!(p,8);
            return 0;
        }
        op!(p,9);
    }
    0
}

/// SpdySession::MaybeCheckConnectionStatus; native action contract: session-operations.json.
pub fn maybe_check_connection_status(p: &mut impl Services) -> i64 {
    if op!(p,0) != 0 {
        op!(p,1);
    } else {
        op!(p,2);
    }
    0
}

/// SpdySession::MaybeSendPrefacePing; native action contract: session-operations.json.
pub fn maybe_send_preface_ping(p: &mut impl Services) -> i64 {
    if op!(p,0) != 0 || op!(p,1) != 0 || op!(p,2) == 0 {
        return 0;
    }
    if op!(p,3) != 0 {
        op!(p,4);
    }
    0
}

/// SpdySession::WritePingFrame; native action contract: session-operations.json.
pub fn write_ping_frame(p: &mut impl Services) -> i64 {
    op!(p,0);
    op!(p,1);
    op!(p,2);
    if op!(p,3) == 0 {
        let next = op!(p,4) as u64;
        op!(p,5,next.wrapping_add(1));
        op!(p,6);
        op!(p,7);
    }
    0
}

/// SpdySession::PlanToCheckPingStatus; native action contract: session-operations.json.
pub fn plan_to_check_ping_status(p: &mut impl Services) -> i64 {
    if op!(p,0) != 0 {
        return 0;
    }
    op!(p,1);
    op!(p,2);
    0
}

/// SpdySession::CheckPingStatus; native action contract: session-operations.json.
pub fn check_ping_status(p: &mut impl Services) -> i64 {
    op!(p,0);
    if op!(p,1) == 0 {
        op!(p,2);
        return 0;
    }
    op!(p,3);
    if op!(p,4) != 0 || op!(p,5) != 0 {
        op!(p,2);
        op!(p,6);
        return 0;
    }
    op!(p,7);
    0
}

/// SpdySession::GetNewStreamId; native action contract: session-operations.json.
pub fn get_new_stream_id(p: &mut impl Services) -> i64 {
    let id = op!(p,0) as u32;
    op!(p,1,u64::from(id.wrapping_add(2)));
    i64::from(id)
}

/// SpdySession::EnqueueWrite; native action contract: session-operations.json.
pub fn enqueue_write(p: &mut impl Services) -> i64 {
    if op!(p,0) != 0 {
        return 0;
    }
    op!(p,1);
    op!(p,2);
    0
}

/// SpdySession::DeleteStream; native action contract: session-operations.json.
pub fn delete_stream(p: &mut impl Services) -> i64 {
    if op!(p,0) != 0 {
        op!(p,1);
    }
    op!(p,2);
    if op!(p,3) != 0 {
        op!(p,4);
    }
    op!(p,5);
    if op!(p,6) != 0 && op!(p,7) != 0 && op!(p,8) == 0 {
        if op!(p,9) != 0 && op!(p,10) != 0 {
            op!(p,11);
        } else {
            op!(p,12);
        }
    }
    if op!(p,13) != 0 {
        op!(p,14);
    }
    0
}

/// SpdySession::DoDrainSession; native action contract: session-operations.json.
pub fn do_drain_session(p: &mut impl Services) -> i64 {
    op!(p,0);
    if op!(p,1) != 0 {
        return 0;
    }
    op!(p,2);
    op!(p,3);
    if op!(p,4) != 0 {
        op!(p,5);
    }
    let err = op!(p,7);
    let send = op!(p,6) != 0
        || (err != 0
            && err != op!(p,8)
            && err != op!(p,9)
            && err != op!(p,10)
            && err != op!(p,11)
            && err != op!(p,12)
            && err != op!(p,13));
    if send {
        op!(p,14);
        op!(p,15);
    }
    op!(p,16);
    op!(p,17);
    op!(p,18);
    if err == 0 {
        op!(p,19);
    } else {
        op!(p,20);
    }
    op!(p,21);
    op!(p,22);
    0
}

/// SpdySession::DoDrainSessionAsync; native action contract: session-operations.json.
pub fn do_drain_session_async(p: &mut impl Services) -> i64 {
    if op!(p,0) != 0 || op!(p,1) != 0 {
        return 0;
    }
    op!(p,2);
    op!(p,3);
    op!(p,4);
    0
}

/// SpdySession::CompleteStreamRequest; native action contract: session-operations.json.
pub fn complete_stream_request(p: &mut impl Services) -> i64 {
    if op!(p,0) == 0 {
        return 0;
    }
    let rv = op!(p,1);
    if rv == 0 {
        op!(p,2);
        return 0;
    }
    op!(p,3);
    if rv != -1 {
        op!(p,4);
    }
    0
}

/// SpdySession::OnWriteBufferConsumed; native action contract: session-operations.json.
pub fn on_write_buffer_consumed(p: &mut impl Services) -> i64 {
    if op!(p,0) != 0 {
        let amount = (op!(p,1) as u64).min(op!(p,2) as u64);
        op!(p,3,amount);
        op!(p,4);
    }
    0
}

/// SpdySession::IncreaseSendWindowSize; native action contract: session-operations.json.
pub fn increase_send_window_size(p: &mut impl Services) -> i64 {
    let delta = op!(p,0) as i32;
    let current = op!(p,1) as i32;
    if delta > i32::MAX.wrapping_sub(current) {
        op!(p,2);
        op!(p,3);
        return 0;
    }
    op!(p,4,current.wrapping_add(delta) as u64);
    op!(p,5);
    op!(p,6);
    op!(p,7);
    0
}

/// SpdySession::DecreaseSendWindowSize; native action contract: session-operations.json.
pub fn decrease_send_window_size(p: &mut impl Services) -> i64 {
    let delta = op!(p,0) as i32;
    let current = op!(p,1) as i32;
    op!(p,2,current.wrapping_sub(delta) as u64);
    op!(p,3);
    0
}

/// SpdySession::IncreaseRecvWindowSize; native action contract: session-operations.json.
pub fn increase_recv_window_size(p: &mut impl Services) -> i64 {
    let delta = op!(p,0) as i32;
    let current = op!(p,1) as i32;
    op!(p,2,current.wrapping_add(delta) as u64);
    op!(p,3);
    let unacked = (op!(p,4) as i32).wrapping_add(delta);
    op!(p,5,unacked as u64);
    op!(p,6);
    if (unacked as u64) > (op!(p,7) as u64) / 2 || op!(p,8) != 0 {
        op!(p,9);
        op!(p,10);
        op!(p,11);
    }
    0
}

/// SpdySession::DecreaseRecvWindowSize; native action contract: session-operations.json.
pub fn decrease_recv_window_size(p: &mut impl Services) -> i64 {
    let delta = op!(p,0) as i32;
    let current = op!(p,1) as i32;
    let receiving = current.wrapping_sub(op!(p,2) as i32);
    op!(p,3,receiving as u64);
    if delta > receiving {
        op!(p,4);
        op!(p,5);
        return 0;
    }
    op!(p,6,current.wrapping_sub(delta) as u64);
    op!(p,7);
    0
}

/// SpdySession::ResumeSendStalledStreams; native action contract: session-operations.json.
pub fn resume_send_stalled_streams(p: &mut impl Services) -> i64 {
    if op!(p,0) != 0 || op!(p,1) != 0 {
        return 0;
    }
    while op!(p,2) == 0 {
        op!(p,3);
        if op!(p,4) == 0 {
            break;
        }
        if op!(p,5) != 0 && op!(p,6) != 0 {
            op!(p,7);
        }
        if op!(p,2) == 0 {
            op!(p,8);
        }
    }
    while op!(p,9) != 0 {
        op!(p,10);
    }
    0
}

/// SpdySession::PopStreamToPossiblyResume; native action contract: session-operations.json.
pub fn pop_stream_to_possibly_resume(p: &mut impl Services) -> i64 {
    let mut i = op!(p,0);
    let min = op!(p,1);
    while i >= min {
        if op!(p,2,i as u64) == 0 {
            return op!(p,3,i as u64);
        }
        i -= 1;
    }
    0
}

/// SpdySession::OnDefaultNetworkActive; native action contract: session-operations.json.
pub fn on_default_network_active(p: &mut impl Services) -> i64 {
    if op!(p,0) == 0 {
        return 0;
    }
    op!(p,1);
    op!(p,2);
    0
}

/// SpdySession::MaybeDisableBrokenConnectionDetection; native action contract: session-operations.json.
pub fn maybe_disable_broken_connection_detection(p: &mut impl Services) -> i64 {
    let count = op!(p,0).wrapping_sub(1);
    op!(p,1,count as u64);
    if count > 0 {
        return 0;
    }
    op!(p,2);
    op!(p,3);
    op!(p,4);
    0
}

/// SpdyStreamRequest::StartRequest; native action contract: session-operations.json.
pub fn request_start(p: &mut impl Services) -> i64 {
    op!(p,0);
    op!(p,1);
    let rv = if op!(p,2) == 0 { op!(p,3) } else { 0 };
    if rv != 0 {
        return rv;
    }
    let rv = op!(p,4);
    if rv != 0 {
        return rv;
    }
    op!(p,5);
    op!(p,6);
    0
}

/// SpdyStreamRequest::CancelRequest; native action contract: session-operations.json.
pub fn request_cancel(p: &mut impl Services) -> i64 {
    if op!(p,0) != 0 {
        op!(p,1);
    }
    op!(p,2);
    op!(p,3);
    0
}

/// SpdyStreamRequest::ReleaseStream; native action contract: session-operations.json.
pub fn request_release(p: &mut impl Services) -> i64 {
    op!(p,0);
    op!(p,1);
    0
}

/// SpdyStreamRequest::SetPriority; native action contract: session-operations.json.
pub fn request_priority(p: &mut impl Services) -> i64 {
    if op!(p,0) != 0 {
        return 0;
    }
    if op!(p,1) != 0 {
        op!(p,2);
    }
    if op!(p,3) != 0 {
        op!(p,4);
    }
    op!(p,5);
    0
}

/// SpdyStreamRequest::OnRequestCompleteSuccess; native action contract: session-operations.json.
pub fn request_success(p: &mut impl Services) -> i64 {
    op!(p,0);
    op!(p,1);
    op!(p,2);
    op!(p,3);
    0
}

/// SpdyStreamRequest::OnRequestCompleteFailure; native action contract: session-operations.json.
pub fn request_failure(p: &mut impl Services) -> i64 {
    op!(p,0);
    op!(p,1);
    op!(p,2);
    op!(p,3);
    0
}

/// SpdyStreamRequest::Reset; native action contract: session-operations.json.
pub fn request_reset(p: &mut impl Services) -> i64 {
    op!(p,0);
    op!(p,1);
    op!(p,2);
    op!(p,3);
    op!(p,4);
    op!(p,5);
    op!(p,6);
    op!(p,7);
    op!(p,8);
    0
}

/// SpdyStreamRequest::OnConfirmHandshakeComplete; native action contract: session-operations.json.
pub fn request_handshake_complete(p: &mut impl Services) -> i64 {
    op!(p,0);
    if op!(p,1) == 0 {
        return 0;
    }
    let rv = op!(p,2);
    if rv != 0 {
        op!(p,3,rv as u64);
        return 0;
    }
    op!(p,4);
    if op!(p,1) == 0 {
        let err = op!(p,5);
        op!(p,3,err as u64);
        return 0;
    }
    let rv = op!(p,6);
    if rv == 0 {
        op!(p,7);
    } else if rv != -1 {
        op!(p,3,rv as u64);
    }
    0
}

/// SpdySession::ParseAlps; native action contract: session-operations.json.
pub fn parse_alps(p: &mut impl Services) -> i64 {
    if op!(p,0) == 0 {
        return 0;
    }
    if op!(p,1) == 0 {
        return op!(p,2);
    }
    op!(p,3);
    while op!(p,4) != 0 {
        op!(p,5);
        op!(p,6);
    }
    let mut valid = false;
    let mut invalid = false;
    let n = op!(p,7) as u64;
    for i in 0..n {
        if op!(p,8,i) != 0 || op!(p,9,i) == 0 {
            invalid = true;
            continue;
        }
        valid = true;
        op!(p,10,i);
        op!(p,11,i);
    }
    let classification = if valid {
        if invalid {
            3
        } else {
            1
        }
    } else if invalid {
        2
    } else {
        0
    };
    op!(p,12,classification);
    0
}

/// SpdySession::GetRemoteEndpoint; native action contract: session-operations.json.
pub fn get_remote_endpoint(p: &mut impl Services) -> i64 {
    let rv = op!(p,0);
    if rv == op!(p,1) && op!(p,2) != 0 {
        op!(p,3);
    }
    rv
}

/// SpdySession::GetResolutionDetails; native action contract: session-operations.json.
pub fn get_resolution_details(p: &mut impl Services) -> i64 {
    if op!(p,0) != 0 {
        op!(p,1);
    }
    0
}

/// SpdySession::GetAcceptChViaAlps; native action contract: session-operations.json.
pub fn get_accept_ch_via_alps(p: &mut impl Services) -> i64 {
    if op!(p,0) == 0 {
        op!(p,1);
    } else {
        op!(p,2);
    }
    0
}

/// SpdySession::CloseActiveStream; native action contract: session-operations.json.
pub fn close_active_stream(p: &mut impl Services) -> i64 {
    op!(p,0);
    if op!(p,1) == 0 {
        op!(p,2);
    }
    op!(p,3);
    0
}

/// SpdySession::CloseCreatedStream; native action contract: session-operations.json.
pub fn close_created_stream(p: &mut impl Services) -> i64 {
    op!(p,0);
    if op!(p,1) == 0 {
        op!(p,2);
    }
    op!(p,3);
    0
}

/// SpdySession::ResetStream; native action contract: session-operations.json.
pub fn reset_stream(p: &mut impl Services) -> i64 {
    op!(p,0);
    if op!(p,1) == 0 {
        op!(p,2);
    }
    op!(p,3);
    0
}

/// SpdySession::CreateDataBuffer; native action contract: session-operations.json.
pub fn create_data_buffer(p: &mut impl Services) -> i64 {
    if op!(p,0) != 0 {
        return 0;
    }
    op!(p,1);
    let len = op!(p,2) as i32;
    if len < 0 {
        op!(p,3);
    }
    op!(p,5,len.min(op!(p,4) as i32) as u64);
    let stream_stalled = op!(p,6) <= 0;
    let session_stalled = op!(p,7) != 0;
    let category = u64::from(stream_stalled) + 2 * u64::from(session_stalled);
    op!(p,8,category);
    if stream_stalled {
        op!(p,9);
        op!(p,10);
        op!(p,11);
        return 0;
    }
    let effective = (op!(p,12) as i32).min(op!(p,6) as i32);
    op!(p,5,effective as u64);
    if session_stalled {
        op!(p,9);
        op!(p,10);
        op!(p,13);
        return 0;
    }
    let effective = (op!(p,12) as i32).min(op!(p,14) as i32);
    op!(p,5,effective as u64);
    op!(p,15);
    if op!(p,12) < i64::from(len) {
        let flags = op!(p,16) as u64;
        let fin = op!(p,17) as u64;
        op!(p,18,flags & !fin);
    }
    if op!(p,12) > 0 {
        op!(p,19);
    }
    op!(p,20);
    op!(p,21);
    if op!(p,12) != 0 {
        op!(p,22);
        op!(p,23);
    }
    let flags = op!(p,16) as u64;
    let fin = op!(p,17) as u64;
    op!(p,24,u64::from(flags & fin == fin));
    0
}

/// SpdySession::UpdateStreamPriority; native action contract: session-operations.json.
pub fn update_stream_priority(p: &mut impl Services) -> i64 {
    op!(p,0);
    if op!(p,1) == 0 {
        return 0;
    }
    op!(p,2);
    let n = op!(p,3) as u64;
    for i in 0..n {
        op!(p,4,i);
    }
    0
}

/// SpdySession::CreateHeaders; native action contract: session-operations.json.
pub fn create_headers(p: &mut impl Services) -> i64 {
    op!(p,0);
    op!(p,1);
    op!(p,2);
    op!(p,3);
    op!(p,4);
    op!(p,5);
    let flags = op!(p,6) as u64;
    let fin = op!(p,7) as u64;
    op!(p,8,u64::from(flags&fin!=0));
    let count = op!(p,9) as i32;
    op!(p,10,count.wrapping_add(1) as u64);
    op!(p,11);
    0
}

/// SpdySession::IsReused; native action contract: session-operations.json.
pub fn is_reused(p: &mut impl Services) -> i64 {
    i64::from(op!(p,0) > 0 || op!(p,1) != 0 || op!(p,2) != 0)
}

/// SpdySession::GetLoadTimingInfo; native action contract: session-operations.json.
pub fn get_load_timing_info(p: &mut impl Services) -> i64 {
    if op!(p,0) != 0 {
        op!(p,1);
        let reused = op!(p,3) as u64;
        return op!(p,2,reused);
    }
    op!(p,4);
    let reused = op!(p,3) as u64;
    op!(p,5,reused);
    if op!(p,6) == 0 {
        op!(p,7);
    }
    op!(p,8);
    1
}

/// SpdySession::GetPeerAddress; native action contract: session-operations.json.
pub fn get_peer_address(p: &mut impl Services) -> i64 {
    if op!(p,0) != 0 {
        op!(p,1)
    } else {
        op!(p,2)
    }
}

/// SpdySession::GetLocalAddress; native action contract: session-operations.json.
pub fn get_local_address(p: &mut impl Services) -> i64 {
    if op!(p,0) != 0 {
        op!(p,1)
    } else {
        op!(p,2)
    }
}

/// SpdySession::HasAcceptableTransportSecurity; native action contract: session-operations.json.
pub fn has_acceptable_transport_security(p: &mut impl Services) -> i64 {
    op!(p,0);
    if op!(p,1) < op!(p,2) {
        return 0;
    }
    i64::from(op!(p,3) != 0)
}

/// SpdySession::CloseOneIdleConnection; native action contract: session-operations.json.
pub fn close_one_idle_connection(p: &mut impl Services) -> i64 {
    op!(p,0);
    if op!(p,1) != 0 {
        op!(p,2);
    }
    0
}

/// SpdySession::ChangeSocketTag; native action contract: session-operations.json.
pub fn change_socket_tag(p: &mut impl Services) -> i64 {
    if op!(p,0) == 0 || op!(p,1) == 0 || op!(p,2) != 0 {
        return 0;
    }
    op!(p,3);
    op!(p,4);
    1
}

/// SpdySession::EnableBrokenConnectionDetection; native action contract: session-operations.json.
pub fn enable_broken_connection_detection(p: &mut impl Services) -> i64 {
    let old = op!(p,0) as i32;
    op!(p,1,old.wrapping_add(1) as u64);
    if old > 0 {
        return 0;
    }
    op!(p,2);
    op!(p,3);
    op!(p,4);
    op!(p,5);
    0
}

/// SpdySession::InitializeInternal; native action contract: session-operations.json.
pub fn initialize_internal(p: &mut impl Services) -> i64 {
    op!(p,0);
    op!(p,1);
    op!(p,2);
    op!(p,3);
    if op!(p,4) != 0 {
        op!(p,5);
    }
    op!(p,6);
    op!(p,7);
    0
}

/// SpdySession::ResetStreamIterator; native action contract: session-operations.json.
pub fn reset_stream_iterator(p: &mut impl Services) -> i64 {
    let err = op!(p,0);
    let code = if err == op!(p,1) {
        op!(p,7)
    } else if err == op!(p,2) {
        op!(p,8)
    } else if err == op!(p,3) {
        op!(p,9)
    } else if err == op!(p,4) {
        op!(p,10)
    } else if err == op!(p,5) {
        op!(p,11)
    } else {
        op!(p,6)
    };
    op!(p,12);
    op!(p,13,code as u64);
    if op!(p,14) != 0 {
        op!(p,15);
    }
    0
}

/// SpdySession::OnStreamError; native action contract: session-operations.json.
pub fn on_stream_error(p: &mut impl Services) -> i64 {
    if op!(p,0) == 0 {
        op!(p,1);
    } else {
        op!(p,2);
    }
    0
}

/// SpdySession::OnPing; native action contract: session-operations.json.
pub fn on_ping(p: &mut impl Services) -> i64 {
    op!(p,0);
    if op!(p,1) == 0 {
        op!(p,2);
        return 0;
    }
    if op!(p,3) == 0 {
        op!(p,4);
        op!(p,5);
        return 0;
    }
    op!(p,6);
    if op!(p,7) != 0 {
        op!(p,8);
    }
    0
}

/// SpdySession::OnRstStream; native action contract: session-operations.json.
pub fn on_rst_stream(p: &mut impl Services) -> i64 {
    op!(p,0);
    if op!(p,1) == 0 {
        op!(p,2);
        return 0;
    }
    op!(p,3);
    let code = op!(p,5);
    if op!(p,4) != 0 {
        if code == op!(p,6) {
            op!(p,11);
            return 0;
        }
        if (code == op!(p,7) || code == op!(p,8))
            && op!(p,12) != 0
            && op!(p,13) != 0
            && op!(p,14) >= 300
        {
            op!(p,11);
            return 0;
        }
    }
    if code == op!(p,6) {
        op!(p,15);
    } else if code == op!(p,9) {
        op!(p,16);
    } else if code == op!(p,10) {
        op!(p,17);
        op!(p,18);
    } else {
        op!(p,19);
        op!(p,20);
        op!(p,21);
    }
    0
}

/// SpdySession::OnGoAway; native action contract: session-operations.json.
pub fn on_go_away(p: &mut impl Services) -> i64 {
    op!(p,0);
    let code = op!(p,1);
    let http1 = code == op!(p,2);
    if http1 {
        op!(p,4);
    } else if code == op!(p,3) {
        op!(p,5);
    } else {
        op!(p,6);
    }
    op!(p,7);
    if http1 {
        op!(p,8);
    } else {
        op!(p,9);
    }
    op!(p,10);
    0
}

/// SpdySession::OnDataFrameHeader; native action contract: session-operations.json.
pub fn on_data_frame_header(p: &mut impl Services) -> i64 {
    op!(p,0);
    if op!(p,1) == 0 {
        return 0;
    }
    op!(p,2);
    0
}

/// SpdySession::OnStreamFrameData; native action contract: session-operations.json.
pub fn on_stream_frame_data(p: &mut impl Services) -> i64 {
    op!(p,0);
    if op!(p,1) != 0 {
        op!(p,2);
        op!(p,3);
        op!(p,4);
    } else {
        op!(p,5);
    }
    if op!(p,6) == 0 {
        return 0;
    }
    op!(p,7);
    op!(p,8);
    op!(p,9);
    0
}

/// SpdySession::OnStreamEnd; native action contract: session-operations.json.
pub fn on_stream_end(p: &mut impl Services) -> i64 {
    if op!(p,0) != 0 {
        op!(p,1);
    }
    0
}

/// SpdySession::OnStreamPadding; native action contract: session-operations.json.
pub fn on_stream_padding(p: &mut impl Services) -> i64 {
    op!(p,0);
    op!(p,1);
    op!(p,2);
    if op!(p,3) != 0 {
        op!(p,4);
    }
    0
}

/// SpdySession::OnSettings; native action contract: session-operations.json.
pub fn on_settings(p: &mut impl Services) -> i64 {
    op!(p,0);
    if op!(p,1) == 0 {
        op!(p,2);
    }
    op!(p,3);
    op!(p,4);
    0
}

/// SpdySession::OnWindowUpdate; native action contract: session-operations.json.
pub fn on_window_update(p: &mut impl Services) -> i64 {
    op!(p,0);
    if op!(p,1) != 0 {
        if op!(p,2) < 1 {
            op!(p,3);
            op!(p,4);
            return 0;
        }
        op!(p,5);
    } else {
        if op!(p,6) == 0 {
            op!(p,7);
            return 0;
        }
        op!(p,8);
        if op!(p,2) < 1 {
            op!(p,9);
            return 0;
        }
        op!(p,10);
    }
    0
}

/// SpdySession::OnHeaders; native action contract: session-operations.json.
pub fn on_headers(p: &mut impl Services) -> i64 {
    op!(p,0);
    if op!(p,1) == 0 {
        op!(p,2);
        return 0;
    }
    op!(p,3);
    op!(p,4);
    0
}

/// SpdySession::OnAltSvc; native action contract: session-operations.json.
pub fn on_alt_svc(p: &mut impl Services) -> i64 {
    if op!(p,0) != 0 && op!(p,1) != 0 {
        return 0;
    }
    if op!(p,2) == 0 || op!(p,3) == 0 || op!(p,4) != 0 {
        return 0;
    }
    if op!(p,1) == 0 {
        if op!(p,5) != 0 || op!(p,6) == 0 || op!(p,7) != 0 || op!(p,8) == 0 || op!(p,9) == 0 {
            return 0;
        }
        op!(p,10);
    } else {
        if op!(p,5) == 0 || op!(p,11) == 0 || op!(p,12) == 0 {
            return 0;
        }
        op!(p,13);
    }
    op!(p,14);
    0
}

/// SpdySession::OnUnknownFrame; native action contract: session-operations.json.
pub fn on_unknown_frame(p: &mut impl Services) -> i64 {
    let id = op!(p,0) as u32;
    if id % 2 == 1 {
        i64::from(u64::from(id) <= op!(p,1) as u64)
    } else {
        i64::from(id == 0)
    }
}

/// SpdySession::OnSendCompressedFrame; native action contract: session-operations.json.
pub fn on_send_compressed_frame(p: &mut impl Services) -> i64 {
    if op!(p,0) == 0 {
        return 0;
    }
    op!(p,1);
    let compressed = (op!(p,2) as u64).wrapping_sub(op!(p,3) as u64);
    let payload = op!(p,4) as u64;
    if payload != 0 {
        let percent = 100_u64.wrapping_sub(100_u64.wrapping_mul(compressed) / payload);
        op!(p,5,percent);
    }
    0
}

/// SpdySession::SendInitialData; native action contract: session-operations.json.
pub fn send_initial_data(p: &mut impl Services) -> i64 {
    op!(p,0);
    while op!(p,1) != 0 {
        let id = op!(p,2) as u32;
        let value = op!(p,3) as u32;
        if !setting_is_default(id, value) {
            op!(p,4);
        }
        op!(p,5);
    }
    if op!(p,6) != 0 {
        let high = op!(p,7) as u64;
        let low = op!(p,7) as u64;
        let id = 0x0a0a + 0x1000 * high + 0x0010 * low;
        let value = op!(p,8) as u64;
        op!(p,9,id,value);
    }
    op!(p,10);
    op!(p,11);
    let max = op!(p,12) as u64;
    let current = op!(p,13) as i32;
    let send = max > current as u64;
    if send {
        let delta = max.wrapping_sub(current as u64) as i32;
        op!(p,14,delta as u64,current.wrapping_add(delta) as u64);
        op!(p,15);
        let unacked = op!(p,16) as i32;
        op!(p,17,unacked.wrapping_add(delta) as u64);
        op!(p,18);
    }
    let mut size = (op!(p,19) as u64).wrapping_add(op!(p,20) as u64);
    if send {
        size = size.wrapping_add(op!(p,21) as u64);
    }
    op!(p,22,size);
    op!(p,23);
    if send {
        op!(p,24);
    }
    op!(p,25);
    op!(p,26);
    0
}

/// SpdySession::CanPool; native action contract: session-operations.json.
pub fn can_pool(p: &mut impl Services) -> i64 {
    if op!(p,0) != 0 {
        return 0;
    }
    if op!(p,1) != 0 && !(op!(p,2) != 0 && op!(p,3) != 0) {
        return 0;
    }
    if op!(p,4) == 0 || op!(p,5) != 0 {
        return 0;
    }
    op!(p,6);
    i64::from(op!(p,7) == 0)
}

/// GetTotalSize; native action contract: session-operations.json.
pub fn get_total_size(p: &mut impl Services) -> i64 {
    let count = op!(p,0) as u64;
    let mut sum = 0_u64;
    for i in 0..count {
        sum = sum.wrapping_add(op!(p,1,i) as u64);
    }
    sum as i64
}

/// MapFramerErrorToProtocolError; native action contract: session-operations.json.
pub fn map_framer_error_to_protocol_error(p: &mut impl Services) -> i64 {
    let input = op!(p,0);
    let n = op!(p,1) as u64;
    for i in 0..n {
        if input == op!(p,2,i) {
            return op!(p,3,i);
        }
    }
    op!(p,4)
}

/// MapFramerErrorToNetError; native action contract: session-operations.json.
pub fn map_framer_error_to_net_error(p: &mut impl Services) -> i64 {
    let input = op!(p,0);
    let n = op!(p,1) as u64;
    for i in 0..n {
        if input == op!(p,2,i) {
            return op!(p,3,i);
        }
    }
    op!(p,4)
}

/// MapRstStreamStatusToProtocolError; native action contract: session-operations.json.
pub fn map_rst_stream_status_to_protocol_error(p: &mut impl Services) -> i64 {
    let input = op!(p,0);
    let n = op!(p,1) as u64;
    for i in 0..n {
        if input == op!(p,2,i) {
            return op!(p,3,i);
        }
    }
    op!(p,4)
}

/// MapNetErrorToGoAwayStatus; native action contract: session-operations.json.
pub fn map_net_error_to_go_away_status(p: &mut impl Services) -> i64 {
    let input = op!(p,0);
    let n = op!(p,1) as u64;
    for i in 0..n {
        if input == op!(p,2,i) {
            return op!(p,3,i);
        }
    }
    op!(p,4)
}

/// IsSpdySettingAtDefaultInitialValue; native action contract: session-operations.json.
pub fn is_spdy_setting_at_default_initial_value(p: &mut impl Services) -> i64 {
    let id = op!(p,0) as u32;
    let value = op!(p,1) as u32;
    i64::from(setting_is_default(id, value))
}

/// SpdySession::CancelStreamRequest; native action contract: session-operations.json.
pub fn cancel_stream_request(p: &mut impl Services) -> i64 {
    op!(p,0);
    op!(p,1);
    while op!(p,2) != 0 {
        if op!(p,3) != 0 {
            op!(p,5);
            return 1;
        }
        op!(p,4);
    }
    0
}

/// SpdySession::GetInfoAsValue; native action contract: session-operations.json.
pub fn get_info_as_value(p: &mut impl Services) -> i64 {
    let n = op!(p,0) as u64;
    let mut count = 0_i32;
    for i in 0..n {
        count = count.wrapping_add(op!(p,1,i) as i32);
    }
    op!(p,2,count as u64);
    op!(p,3);
    let frames = if op!(p,4) != 0 { op!(p,5) } else { 0 };
    op!(p,6,frames as u64);
    op!(p,7);
    if op!(p,8) != 0 {
        op!(p,9);
    }
    if op!(p,10) != 0 {
        op!(p,11);
    }
    if op!(p,12) == 0 {
        op!(p,13);
        while op!(p,14) != 0 {
            op!(p,15);
            op!(p,16);
        }
        op!(p,17);
    }
    op!(p,18);
    while op!(p,19) != 0 {
        op!(p,20);
        op!(p,21);
    }
    op!(p,22);
    0
}

/// SpdySession::AvailabilityStateToString; native action contract: session-operations.json.
pub fn availability_state_to_string(p: &mut impl Services) -> i64 {
    match op!(p,0) {
        0 => {
            op!(p,1);
        }
        1 => {
            op!(p,2);
        }
        2 => {
            op!(p,3);
        }
        _ => {
            op!(p,4);
        }
    }
    0
}

/// SpdySession::RecordHistograms; native action contract: session-operations.json.
pub fn record_histograms(p: &mut impl Services) -> i64 {
    op!(p,0);
    if op!(p,1) != 0 {
        let used = op!(p,2) > 0;
        op!(p,3,u64::from(used));
    }
    0
}

/// SpdySession::RecordProtocolErrorHistogram; native action contract: session-operations.json.
pub fn record_protocol_error_histogram(p: &mut impl Services) -> i64 {
    op!(p,0);
    if op!(p,1) != 0 {
        op!(p,2);
    }
    0
}

/// SpdySession::SpdySession; native action contract: session-operations.json.
pub fn spdy_session(p: &mut impl Services) -> i64 {
    op!(p,0);
    op!(p,1);
    if op!(p,2) != 0 {
        op!(p,3);
    }
    0
}

/// SpdySession::~SpdySession; native action contract: session-operations.json.
pub fn _spdy_session(p: &mut impl Services) -> i64 {
    op!(p,0);
    op!(p,1);
    op!(p,2);
    op!(p,3);
    op!(p,4);
    op!(p,5);
    0
}

/// SpdySession::EnqueueSessionWrite; native action contract: session-operations.json.
pub fn enqueue_session_write(p: &mut impl Services) -> i64 {
    op!(p,0);
    if op!(p,1) > op!(p,2) {
        op!(p,3);
        op!(p,4);
        return 0;
    }
    op!(p,5);
    op!(p,6);
    if op!(p,7) != 0 && op!(p,8) != 0 {
        op!(p,9);
        op!(p,10);
    }
    0
}

/// SpdySession::EnqueueGreasedFrame; native action contract: session-operations.json.
pub fn enqueue_greased_frame(p: &mut impl Services) -> i64 {
    if op!(p,0) != 0 {
        return 0;
    }
    op!(p,1);
    op!(p,2);
    0
}

/// SpdySession::SendWindowUpdateFrame; native action contract: session-operations.json.
pub fn send_window_update_frame(p: &mut impl Services) -> i64 {
    if op!(p,0) != 0 {
        op!(p,1);
    } else {
        op!(p,2);
    }
    op!(p,3);
    op!(p,4);
    op!(p,5);
    0
}

/// SpdySession::SendStreamWindowUpdate; native action contract: session-operations.json.
pub fn send_stream_window_update(p: &mut impl Services) -> i64 {
    op!(p,0);
    op!(p,1);
    0
}

/// SpdySession::InitializeWithSocketHandle; native action contract: session-operations.json.
pub fn initialize_with_socket_handle(p: &mut impl Services) -> i64 {
    op!(p,0);
    op!(p,1);
    op!(p,2);
    op!(p,3);
    op!(p,4);
    0
}

/// SpdySession::InitializeWithSocket; native action contract: session-operations.json.
pub fn initialize_with_socket(p: &mut impl Services) -> i64 {
    op!(p,0);
    op!(p,1);
    op!(p,2);
    op!(p,3);
    op!(p,4);
    0
}

/// SpdySession::EnqueueStreamWrite; native action contract: session-operations.json.
pub fn enqueue_stream_write(p: &mut impl Services) -> i64 {
    op!(p,0);
    op!(p,1);
    0
}

/// SpdySession::CloseSessionOnError; native action contract: session-operations.json.
pub fn close_session_on_error(p: &mut impl Services) -> i64 {
    op!(p,0);
    op!(p,1);
    0
}

/// SpdySession::CloseActiveStreamIterator; native action contract: session-operations.json.
pub fn close_active_stream_iterator(p: &mut impl Services) -> i64 {
    op!(p,0);
    op!(p,1);
    op!(p,2);
    op!(p,3);
    0
}

/// SpdySession::CloseCreatedStreamIterator; native action contract: session-operations.json.
pub fn close_created_stream_iterator(p: &mut impl Services) -> i64 {
    op!(p,0);
    op!(p,1);
    op!(p,2);
    0
}

/// SpdySession::EnqueueResetStreamFrame; native action contract: session-operations.json.
pub fn enqueue_reset_stream_frame(p: &mut impl Services) -> i64 {
    op!(p,0);
    op!(p,1);
    op!(p,2);
    op!(p,3);
    op!(p,4);
    0
}

/// SpdySession::EnqueuePriorityFrame; native action contract: session-operations.json.
pub fn enqueue_priority_frame(p: &mut impl Services) -> i64 {
    op!(p,0);
    op!(p,1);
    op!(p,2);
    0
}

/// SpdySession::InsertCreatedStream; native action contract: session-operations.json.
pub fn insert_created_stream(p: &mut impl Services) -> i64 {
    op!(p,0);
    op!(p,1);
    0
}

/// SpdySession::ActivateCreatedStream; native action contract: session-operations.json.
pub fn activate_created_stream(p: &mut impl Services) -> i64 {
    op!(p,0);
    op!(p,1);
    op!(p,2);
    0
}

/// SpdySession::InsertActivatedStream; native action contract: session-operations.json.
pub fn insert_activated_stream(p: &mut impl Services) -> i64 {
    op!(p,0);
    op!(p,1);
    op!(p,2);
    0
}

/// SpdySession::LogAbandonedStream; native action contract: session-operations.json.
pub fn log_abandoned_stream(p: &mut impl Services) -> i64 {
    op!(p,0);
    op!(p,1);
    0
}

/// SpdySession::LogAbandonedActiveStream; native action contract: session-operations.json.
pub fn log_abandoned_active_stream(p: &mut impl Services) -> i64 {
    op!(p,0);
    op!(p,1);
    let count = op!(p,2) as i32;
    op!(p,3,count.wrapping_add(1) as u64);
    0
}

/// SpdySession::OnError; native action contract: session-operations.json.
pub fn on_error(p: &mut impl Services) -> i64 {
    op!(p,0);
    op!(p,1);
    op!(p,2);
    op!(p,3);
    0
}

/// SpdySession::OnSettingsAck; native action contract: session-operations.json.
pub fn on_settings_ack(p: &mut impl Services) -> i64 {
    op!(p,0);
    op!(p,1);
    0
}

/// SpdySession::OnSetting; native action contract: session-operations.json.
pub fn on_setting(p: &mut impl Services) -> i64 {
    op!(p,0);
    op!(p,1);
    op!(p,2);
    0
}

/// SpdySession::OnPushPromise; native action contract: session-operations.json.
pub fn on_push_promise(p: &mut impl Services) -> i64 {
    op!(p,0);
    op!(p,1);
    0
}

/// SpdySession::OnReadBufferConsumed; native action contract: session-operations.json.
pub fn on_read_buffer_consumed(p: &mut impl Services) -> i64 {
    op!(p,0);
    op!(p,1);
    0
}

/// SpdySession::QueueSendStalledStream; native action contract: session-operations.json.
pub fn queue_send_stalled_stream(p: &mut impl Services) -> i64 {
    op!(p,0);
    op!(p,1);
    0
}

/// SpdySession::CheckConnectionStatus; native action contract: session-operations.json.
pub fn check_connection_status(p: &mut impl Services) -> i64 {
    op!(p,0);
    op!(p,1);
    0
}

/// LogSessionCreationInitiatorToHistogram; native action contract: session-operations.json.
pub fn log_session_creation_initiator_to_histogram(p: &mut impl Services) -> i64 {
    if op!(p,0) != 0 {
        op!(p,1);
    } else {
        op!(p,2);
    }
    op!(p,3);
    0
}

/// NetLogSpdyHeadersSentParams; native action contract: session-operations.json.
pub fn net_log_spdy_headers_sent_params(p: &mut impl Services) -> i64 {
    op!(p,0);
    if op!(p,1) != 0 {
        op!(p,2);
    }
    if op!(p,3) != 0 {
        op!(p,4);
    }
    0
}

/// NetLogSpdyInitializedParams; native action contract: session-operations.json.
pub fn net_log_spdy_initialized_params(p: &mut impl Services) -> i64 {
    if op!(p,0) != 0 {
        op!(p,1);
    }
    op!(p,2);
    0
}

/// NetLogSpdySendSettingsParams; native action contract: session-operations.json.
pub fn net_log_spdy_send_settings_params(p: &mut impl Services) -> i64 {
    op!(p,0);
    while op!(p,1) != 0 {
        op!(p,2);
        op!(p,3);
    }
    0
}

/// GreasedBufferProducer::ProduceBuffer; native action contract: session-operations.json.
pub fn greased_buffer_producer_produce_buffer(p: &mut impl Services) -> i64 {
    let id = if op!(p,0) != 0 { op!(p,1) } else { 0 };
    op!(p,2,id as u64);
    op!(p,3);
    op!(p,4);
    op!(p,5);
    0
}

/// SpdySession::IsAvailable; native action contract: session-operations.json.
pub fn is_available(p: &mut impl Services) -> i64 {
    i64::from(op!(p,0) == 0)
}

/// SpdySession::IsGoingAway; native action contract: session-operations.json.
pub fn is_going_away(p: &mut impl Services) -> i64 {
    i64::from(op!(p,0) == 1)
}

/// SpdySession::IsDraining; native action contract: session-operations.json.
pub fn is_draining(p: &mut impl Services) -> i64 {
    i64::from(op!(p,0) == 2)
}

/// SpdySession::WasEverUsedToCreateStreams; native action contract: session-operations.json.
pub fn was_ever_used_to_create_streams(p: &mut impl Services) -> i64 {
    i64::from(op!(p,0) > 0)
}

/// SpdySession::IsSendStalled; native action contract: session-operations.json.
pub fn is_send_stalled(p: &mut impl Services) -> i64 {
    i64::from(op!(p,0) == 0)
}

/// SpdySession::is_active; native action contract: session-operations.json.
pub fn is_active(p: &mut impl Services) -> i64 {
    i64::from(op!(p,0) == 0 || op!(p,1) == 0)
}

pub fn dispatch(method: u32, p: &mut impl Services) -> i64 {
    match method {
        0 => should_send_http2_priority(p),
        1 => should_send_priority_update(p),
        2 => verify_domain_authentication(p),
        3 => confirm_handshake(p),
        4 => make_unavailable(p),
        5 => start_going_away(p),
        6 => maybe_finish_going_away(p),
        7 => try_create_stream(p),
        8 => create_stream(p),
        9 => change_stream_request_priority(p),
        10 => get_next_pending_stream_request(p),
        11 => process_pending_stream_requests(p),
        12 => pump_read_loop(p),
        13 => do_read_loop(p),
        14 => do_read(p),
        15 => do_read_complete(p),
        16 => pump_write_loop(p),
        17 => maybe_post_write_loop(p),
        18 => do_write_loop(p),
        19 => do_write(p),
        20 => do_write_complete(p),
        21 => notify_requests_of_confirmation(p),
        22 => handle_setting(p),
        23 => update_streams_send_window_size(p),
        24 => maybe_check_connection_status(p),
        25 => maybe_send_preface_ping(p),
        26 => write_ping_frame(p),
        27 => plan_to_check_ping_status(p),
        28 => check_ping_status(p),
        29 => get_new_stream_id(p),
        30 => enqueue_write(p),
        31 => delete_stream(p),
        32 => do_drain_session(p),
        33 => do_drain_session_async(p),
        34 => complete_stream_request(p),
        35 => on_write_buffer_consumed(p),
        36 => increase_send_window_size(p),
        37 => decrease_send_window_size(p),
        38 => increase_recv_window_size(p),
        39 => decrease_recv_window_size(p),
        40 => resume_send_stalled_streams(p),
        41 => pop_stream_to_possibly_resume(p),
        42 => on_default_network_active(p),
        43 => maybe_disable_broken_connection_detection(p),
        44 => request_start(p),
        45 => request_cancel(p),
        46 => request_release(p),
        47 => request_priority(p),
        48 => request_success(p),
        49 => request_failure(p),
        50 => request_reset(p),
        51 => request_handshake_complete(p),
        52 => parse_alps(p),
        53 => get_remote_endpoint(p),
        54 => get_resolution_details(p),
        55 => get_accept_ch_via_alps(p),
        56 => close_active_stream(p),
        57 => close_created_stream(p),
        58 => reset_stream(p),
        59 => create_data_buffer(p),
        60 => update_stream_priority(p),
        61 => create_headers(p),
        62 => is_reused(p),
        63 => get_load_timing_info(p),
        64 => get_peer_address(p),
        65 => get_local_address(p),
        66 => has_acceptable_transport_security(p),
        67 => close_one_idle_connection(p),
        68 => change_socket_tag(p),
        69 => enable_broken_connection_detection(p),
        70 => initialize_internal(p),
        71 => reset_stream_iterator(p),
        72 => on_stream_error(p),
        73 => on_ping(p),
        74 => on_rst_stream(p),
        75 => on_go_away(p),
        76 => on_data_frame_header(p),
        77 => on_stream_frame_data(p),
        78 => on_stream_end(p),
        79 => on_stream_padding(p),
        80 => on_settings(p),
        81 => on_window_update(p),
        82 => on_headers(p),
        83 => on_alt_svc(p),
        84 => on_unknown_frame(p),
        85 => on_send_compressed_frame(p),
        86 => send_initial_data(p),
        87 => can_pool(p),
        88 => get_total_size(p),
        89 => map_framer_error_to_protocol_error(p),
        90 => map_framer_error_to_net_error(p),
        91 => map_rst_stream_status_to_protocol_error(p),
        92 => map_net_error_to_go_away_status(p),
        93 => is_spdy_setting_at_default_initial_value(p),
        94 => cancel_stream_request(p),
        95 => get_info_as_value(p),
        96 => availability_state_to_string(p),
        97 => record_histograms(p),
        98 => record_protocol_error_histogram(p),
        99 => spdy_session(p),
        100 => _spdy_session(p),
        101 => enqueue_session_write(p),
        102 => enqueue_greased_frame(p),
        103 => send_window_update_frame(p),
        104 => send_stream_window_update(p),
        105 => initialize_with_socket_handle(p),
        106 => initialize_with_socket(p),
        107 => enqueue_stream_write(p),
        108 => close_session_on_error(p),
        109 => close_active_stream_iterator(p),
        110 => close_created_stream_iterator(p),
        111 => enqueue_reset_stream_frame(p),
        112 => enqueue_priority_frame(p),
        113 => insert_created_stream(p),
        114 => activate_created_stream(p),
        115 => insert_activated_stream(p),
        116 => log_abandoned_stream(p),
        117 => log_abandoned_active_stream(p),
        118 => on_error(p),
        119 => on_settings_ack(p),
        120 => on_setting(p),
        121 => on_push_promise(p),
        122 => on_read_buffer_consumed(p),
        123 => queue_send_stalled_stream(p),
        124 => check_connection_status(p),
        125 => log_session_creation_initiator_to_histogram(p),
        126 => net_log_spdy_headers_sent_params(p),
        127 => net_log_spdy_initialized_params(p),
        128 => net_log_spdy_send_settings_params(p),
        129 => greased_buffer_producer_produce_buffer(p),
        130 => is_available(p),
        131 => is_going_away(p),
        132 => is_draining(p),
        133 => was_ever_used_to_create_streams(p),
        134 => is_send_stalled(p),
        135 => is_active(p),
        _ => panic!("invalid session method"),
    }
}
