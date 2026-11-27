// Copyright 2026 The Chromium Authors
// BSD-style license; see LICENSE.
#![forbid(unsafe_code)]

/// Native operations own containers/WeakPtrs/providers. Rust owns iteration,
/// branch decisions and mutation/callback sequencing. Operations are synchronous;
/// no native borrow/reference is retained by this safe core across a callback.
pub trait Services {
    fn op(&mut self, operation: u32, a: u64, b: u64, c: u64) -> i64;
}
macro_rules! op {
    ($p:expr,$n:expr) => { $p.op($n,0,0,0) };
    ($p:expr,$n:expr,$a:expr) => { { let arg_a=$a; $p.op($n,arg_a,0,0) } };
    ($p:expr,$n:expr,$a:expr,$b:expr) => { { let arg_a=$a; let arg_b=$b; $p.op($n,arg_a,arg_b,0) } };
}

/// FindAvailableSession: source-bound native operation list is in pool-operations.json.
pub fn find_available_session_v0(p: &mut impl Services) -> i64 {
    let mut found = op!(p,0) != 0;
    if !found && op!(p,1) != 0 && op!(p,2) != 0 && op!(p,3) != 0 && op!(p,4) != 0 && op!(p,5) != 0 {
        found = op!(p,0) != 0;
    }
    if !found || (op!(p,6) != 0 && op!(p,7) == 0) {
        return 0;
    }
    if op!(p,8) != 0 {
        op!(p,9);
        op!(p,11);
    } else if op!(p,1) != 0 {
        op!(p,10);
        op!(p,11);
    }
    0
}

/// HasAvailableSession: source-bound native operation list is in pool-operations.json.
pub fn has_available_session_v0(p: &mut impl Services) -> i64 {
    if op!(p,0) == 0 || (op!(p,1) != 0 && op!(p,2) == 0) {
        return 0;
    }
    i64::from(op!(p,3) != 0 || op!(p,4) != 0)
}

/// RequestSession: source-bound native operation list is in pool-operations.json.
pub fn request_session_v0(p: &mut impl Services) -> i64 {
    op!(p,0);
    if op!(p,1) != 0 {
        op!(p,2);
        return 0;
    }
    let blocking = op!(p,3) == 0;
    op!(p,4,u64::from(blocking));
    op!(p,5);
    if blocking {
        op!(p,6);
    } else if op!(p,7) != 0 {
        op!(p,8);
    }
    0
}

/// MakeSessionUnavailable: source-bound native operation list is in pool-operations.json.
pub fn make_session_unavailable_v0(p: &mut impl Services) -> i64 {
    op!(p,0);
    op!(p,1);
    op!(p,2);
    while op!(p,3) != 0 {
        op!(p,4);
        op!(p,5);
        op!(p,6);
    }
    op!(p,7);
    0
}

/// RemoveUnavailableSession: source-bound native operation list is in pool-operations.json.
pub fn remove_unavailable_session_v0(p: &mut impl Services) -> i64 {
    op!(p,0);
    op!(p,1);
    op!(p,2);
    op!(p,3);
    op!(p,4);
    0
}

/// CloseAllSessions: source-bound native operation list is in pool-operations.json.
pub fn close_all_sessions_v0(p: &mut impl Services) -> i64 {
    loop {
        op!(p,0);
        let mut all_draining = true;
        while op!(p,1) != 0 {
            if op!(p,2) == 0 {
                all_draining = false;
                break;
            }
            op!(p,3);
        }
        if all_draining {
            return 0;
        }
        op!(p,4);
    }
}

/// MakeCurrentSessionsGoingAway: source-bound native operation list is in pool-operations.json.
pub fn make_current_sessions_going_away_v0(p: &mut impl Services) -> i64 {
    let count = op!(p,0) as u64;
    for i in 0..count {
        if op!(p,1,i) != 0 {
            op!(p,2,i);
            op!(p,3,i);
            op!(p,4,i);
            op!(p,5,i);
        }
    }
    0
}

/// OnIPAddressChanged: source-bound native operation list is in pool-operations.json.
pub fn on_i_p_address_changed_v0(p: &mut impl Services) -> i64 {
    op!(p,0);
    if op!(p,1) != 0 && op!(p,2) != 0 {
        return 0;
    }
    if op!(p,3) != 0 {
        op!(p,4);
    } else {
        op!(p,5);
    }
    0
}

/// OnSSLConfigChanged: source-bound native operation list is in pool-operations.json.
pub fn on_s_s_l_config_changed_v0(p: &mut impl Services) -> i64 {
    let kind = op!(p,0);
    if kind == op!(p,1) {
        op!(p,4);
    } else if kind == op!(p,2) {
        op!(p,5);
    } else if kind == op!(p,3) {
        op!(p,6);
    }
    0
}

/// OnSSLConfigForServersChanged: source-bound native operation list is in pool-operations.json.
pub fn on_s_s_l_config_for_servers_changed_v0(p: &mut impl Services) -> i64 {
    let count = op!(p,0) as u64;
    for i in 0..count {
        if op!(p,1,i) == 0 {
            continue;
        }
        let mut matched = op!(p,2,i) != 0;
        if !matched {
            let n = op!(p,3,i) as u64;
            for j in 0..n {
                if op!(p,4,i,j) != 0 && op!(p,5,i,j) == 0 && op!(p,6,i,j) != 0 {
                    matched = true;
                    break;
                }
            }
        }
        if matched {
            op!(p,7,i);
            op!(p,8,i);
            op!(p,9,i);
            op!(p,10,i);
        }
    }
    0
}

/// RemoveRequestForSpdySession: source-bound native operation list is in pool-operations.json.
pub fn remove_request_for_spdy_session_v0(p: &mut impl Services) -> i64 {
    op!(p,0);
    op!(p,1);
    if op!(p,2) != 0 && op!(p,3) != 0 {
        op!(p,4);
    }
    op!(p,5);
    op!(p,6);
    0
}

/// IsSessionAvailable: source-bound native operation list is in pool-operations.json.
pub fn is_session_available_v0(p: &mut impl Services) -> i64 {
    op!(p,0);
    while op!(p,1) != 0 {
        if op!(p,2) != 0 {
            return 1;
        }
        op!(p,3);
    }
    0
}

/// MapKeyToAvailableSession: source-bound native operation list is in pool-operations.json.
pub fn map_key_to_available_session_v0(p: &mut impl Services) -> i64 {
    op!(p,0);
    op!(p,1);
    op!(p,2);
    0
}

/// UnmapKey: source-bound native operation list is in pool-operations.json.
pub fn unmap_key_v0(p: &mut impl Services) -> i64 {
    op!(p,0);
    op!(p,1);
    op!(p,2);
    0
}

/// RemoveAliases: source-bound native operation list is in pool-operations.json.
pub fn remove_aliases_v0(p: &mut impl Services) -> i64 {
    op!(p,0);
    while op!(p,1) != 0 {
        if op!(p,2) != 0 {
            op!(p,3);
        } else {
            op!(p,4);
        }
    }
    0
}

/// GetCurrentSessions: source-bound native operation list is in pool-operations.json.
pub fn get_current_sessions_v0(p: &mut impl Services) -> i64 {
    op!(p,0);
    while op!(p,1) != 0 {
        op!(p,2);
        op!(p,3);
    }
    0
}

/// CloseCurrentSessionsHelper: source-bound native operation list is in pool-operations.json.
pub fn close_current_sessions_helper_v0(p: &mut impl Services) -> i64 {
    let n = op!(p,0) as u64;
    for i in 0..n {
        if op!(p,1,i) == 0 {
            continue;
        }
        if op!(p,2) != 0 && op!(p,3,i) != 0 {
            continue;
        }
        if op!(p,4,i) != 0 {
            continue;
        }
        op!(p,5,i);
        op!(p,6,i);
        op!(p,7,i);
        op!(p,8,i);
    }
    0
}

/// UpdatePendingRequests: source-bound native operation list is in pool-operations.json.
pub fn update_pending_requests_v0(p: &mut impl Services) -> i64 {
    if op!(p,0) != 0 {
        let pooled = op!(p,1) != 0;
        while op!(p,2) != 0 && op!(p,3) != 0 {
            if op!(p,4) == 0 {
                break;
            }
            op!(p,5);
            while op!(p,6) != 0 {
                if op!(p,7) != 0 && op!(p,8) == 0 {
                    op!(p,10);
                    continue;
                }
                if op!(p,9) == 0 && pooled {
                    op!(p,10);
                    continue;
                }
                break;
            }
            if op!(p,6) == 0 {
                break;
            }
            op!(p,11);
            op!(p,12);
            op!(p,13);
        }
        op!(p,22);
    }
    if op!(p,4) == 0 || op!(p,14) != 0 {
        return 0;
    }
    op!(p,15);
    if op!(p,16) != 0 {
        op!(p,17);
    }
    op!(p,18);
    while op!(p,19) != 0 {
        op!(p,20);
        op!(p,21);
    }
    0
}

/// RemoveRequestInternal: source-bound native operation list is in pool-operations.json.
pub fn remove_request_internal_v0(p: &mut impl Services) -> i64 {
    op!(p,0);
    if op!(p,1) != 0 {
        op!(p,2);
    }
    if op!(p,3) != 0 && op!(p,4) != 0 {
        op!(p,5);
    }
    op!(p,6);
    0
}

/// AddConnectionManagementConfig: source-bound native operation list is in pool-operations.json.
pub fn add_connection_management_config_v0(p: &mut impl Services) -> i64 {
    if op!(p,0) == 0 {
        return 0;
    }
    if op!(p,1) != 0 {
        if op!(p,2) == 0 {
            op!(p,3);
        }
        op!(p,4);
    }
    0
}

/// NotifyOnSessionClosed: source-bound native operation list is in pool-operations.json.
pub fn notify_on_session_closed_v0(p: &mut impl Services) -> i64 {
    if op!(p,0) != 0 {
        op!(p,1);
    }
    0
}

/// NotifyOnConnectionFailure: source-bound native operation list is in pool-operations.json.
pub fn notify_on_connection_failure_v0(p: &mut impl Services) -> i64 {
    if op!(p,0) != 0 {
        op!(p,1);
    }
    0
}

/// FindMatchingIpSessionForServiceEndpoint: source-bound native operation list is in pool-operations.json.
pub fn find_matching_ip_session_for_service_endpoint_v0(p: &mut impl Services) -> i64 {
    op!(p,0);
    if op!(p,1) == 0 {
        op!(p,2);
    }
    0
}

/// GetDnsAliasesForSessionKey: source-bound native operation list is in pool-operations.json.
pub fn get_dns_aliases_for_session_key_v0(p: &mut impl Services) -> i64 {
    if op!(p,0) == 0 {
        op!(p,1);
    } else {
        op!(p,2);
    }
    0
}

/// SpdySessionPoolInfoToValue: source-bound native operation list is in pool-operations.json.
pub fn spdy_session_pool_info_to_value_v0(p: &mut impl Services) -> i64 {
    op!(p,0);
    while op!(p,1) != 0 {
        op!(p,2);
        op!(p,3);
    }
    0
}

/// CreateAvailableSessionFromSocketHandle: source-bound native operation list is in pool-operations.json.
pub fn create_available_session_from_socket_handle_v0(p: &mut impl Services) -> i64 {
    op!(p,0);
    op!(p,1);
    op!(p,2);
    op!(p,3);
    if op!(p,4) != 0 {
        op!(p,5);
        0
    } else {
        op!(p,6)
    }
}

/// CreateAvailableSessionFromSocket: source-bound native operation list is in pool-operations.json.
pub fn create_available_session_from_socket_v0(p: &mut impl Services) -> i64 {
    op!(p,0);
    op!(p,1);
    op!(p,2);
    op!(p,3);
    let check = op!(p,4) as u64;
    op!(p,5,check);
    0
}

/// CreateSession: source-bound native operation list is in pool-operations.json.
pub fn create_session_v0(p: &mut impl Services) -> i64 {
    op!(p,0);
    if op!(p,1) != 0 {
        op!(p,2);
        op!(p,3);
        op!(p,4);
        op!(p,5);
    }
    if op!(p,6) != 0 {
        op!(p,7);
    }
    if op!(p,8) != 0 {
        op!(p,9);
    }
    op!(p,10);
    0
}

/// InsertSession: source-bound native operation list is in pool-operations.json.
pub fn insert_session_v0(p: &mut impl Services) -> i64 {
    op!(p,0);
    op!(p,1);
    op!(p,2);
    op!(p,3);
    op!(p,4);
    if op!(p,5) != 0 && op!(p,6) == 0 {
        op!(p,7);
    }
    if op!(p,8) == 0 {
        op!(p,9);
        return 0;
    }
    if op!(p,10) == 0 {
        op!(p,11);
        op!(p,12);
        op!(p,13);
        return 0;
    }
    let rv = op!(p,14);
    if rv != 0 {
        op!(p,15,rv as u64);
        op!(p,12);
        op!(p,16,rv as u64);
    } else {
        op!(p,9);
    }
    0
}

/// OnHostResolutionComplete: source-bound native operation list is in pool-operations.json.
pub fn on_host_resolution_complete_v0(p: &mut impl Services) -> i64 {
    if op!(p,0) == 0 {
        return 0;
    }
    if op!(p,1) != 0 {
        return i64::from(!(op!(p,2) != 0 && op!(p,3) == 0));
    }
    let n = op!(p,4) as u64;
    for i in 0..n {
        if op!(p,5,i) != 0 {
            return 1;
        }
    }
    if op!(p,6) != 0 {
        op!(p,7);
        for i in 0..n {
            if op!(p,8,i) != 0 {
                op!(p,9,i);
            }
        }
        op!(p,10);
    }
    0
}

/// OnHostResolutionComplete: source-bound native operation list is in pool-operations.json.
pub fn on_host_resolution_complete_v1(p: &mut impl Services) -> i64 {
    if op!(p,0) == 0 {
        return 0;
    }
    if op!(p,1) != 0 {
        return i64::from(!(op!(p,2) != 0 && op!(p,3) == 0));
    }
    let n = op!(p,4) as u64;
    for i in 0..n {
        if op!(p,5,i) != 0 || op!(p,6,i) != 0 {
            return 1;
        }
    }
    if op!(p,7) != 0 {
        op!(p,8);
        for i in 0..n {
            if op!(p,9,i) != 0 {
                op!(p,10,i);
                op!(p,11,i);
            }
        }
        op!(p,12);
    }
    0
}

/// FindMatchingIpSession: source-bound native operation list is in pool-operations.json.
pub fn find_matching_ip_session_v0(p: &mut impl Services) -> i64 {
    let n = op!(p,0) as u64;
    for i in 0..n {
        op!(p,1,i);
        while op!(p,2) != 0 {
            if op!(p,4) == 0 {
                op!(p,3);
                continue;
            }
            op!(p,5);
            if op!(p,6) == 0 {
                op!(p,3);
                continue;
            }
            if op!(p,7) == 0 {
                op!(p,10);
                op!(p,3);
                continue;
            }
            op!(p,8);
            op!(p,9);
            return 0;
        }
    }
    0
}

/// TryPoolToExistingIpSession: source-bound native operation list is in pool-operations.json.
pub fn try_pool_to_existing_ip_session_v0(p: &mut impl Services) -> i64 {
    if op!(p,0) == 0 {
        return 0;
    }
    let n = op!(p,1) as u64;
    for i in 0..n {
        op!(p,2,i);
        while op!(p,3) != 0 {
            op!(p,5);
            if op!(p,6) == 0
                || (op!(p,7) == 0 && op!(p,8) == 0)
                || (op!(p,9) != 0 && op!(p,10) == 0)
            {
                op!(p,4);
                continue;
            }
            if op!(p,11) == 0 {
                if op!(p,12) != 0 {
                    op!(p,13);
                }
                op!(p,4);
                continue;
            }
            op!(p,14);
            let mut adding = true;
            if op!(p,7) == 0 {
                op!(p,15);
                if op!(p,16) != 0 || op!(p,17) == 0 {
                    op!(p,38);
                    op!(p,4);
                    continue;
                }
                if op!(p,18) != 0 {
                    adding = false;
                }
                op!(p,19);
                op!(p,20);
                op!(p,21);
                op!(p,22);
                op!(p,23);
                while op!(p,24) != 0 {
                    if op!(p,25) != 0 {
                        op!(p,26);
                        continue;
                    }
                    op!(p,27);
                    op!(p,28);
                    op!(p,29);
                    if op!(p,30) != 0 {
                        op!(p,31);
                        op!(p,32);
                    }
                    if op!(p,33) != 0 {
                        adding = false;
                    }
                    op!(p,37);
                }
                op!(p,38);
            }
            if adding {
                op!(p,34);
                op!(p,35);
            }
            op!(p,36);
            return 1;
        }
    }
    0
}

/// SpdySessionRequest::~SpdySessionRequest: source-bound native operation list is in pool-operations.json.
pub fn spdy_session_request_spdy_session_request_v0(p: &mut impl Services) -> i64 {
    if op!(p,0) != 0 {
        op!(p,1);
    }
    0
}

/// SpdySessionPool: source-bound native operation list is in pool-operations.json.
pub fn spdy_session_pool_v0(p: &mut impl Services) -> i64 {
    if op!(p,0) != 0 {
        op!(p,1);
    }
    if op!(p,2) != 0 {
        op!(p,3);
    }
    0
}

/// ~SpdySessionPool: source-bound native operation list is in pool-operations.json.
pub fn _spdy_session_pool_v0(p: &mut impl Services) -> i64 {
    op!(p,0);
    op!(p,1);
    while op!(p,2) != 0 {
        op!(p,3);
    }
    if op!(p,4) != 0 {
        op!(p,5);
    }
    if op!(p,6) != 0 {
        op!(p,7);
    }
    0
}

/// IsEndpointEligibleForH2: source-bound native operation list is in pool-operations.json.
pub fn is_endpoint_eligible_for_h2_v0(p: &mut impl Services) -> i64 {
    let n = op!(p,0) as u64;
    if n == 0 {
        return 1;
    }
    for i in 0..n {
        if op!(p,1,i) != 0 {
            return 1;
        }
    }
    0
}

/// AppendUniqueIpEndpoints: source-bound native operation list is in pool-operations.json.
pub fn append_unique_ip_endpoints_v0(p: &mut impl Services) -> i64 {
    let source_count = op!(p,0) as u64;
    for i in 0..source_count {
        let target_count = op!(p,1) as u64;
        let mut seen = false;
        for j in 0..target_count {
            if op!(p,2,i,j) != 0 {
                seen = true;
                break;
            }
        }
        if !seen {
            op!(p,3,i);
        }
    }
    0
}

pub fn dispatch(method: u32, p: &mut impl Services) -> i64 {
    match method {
        0 => find_available_session_v0(p),
        1 => has_available_session_v0(p),
        2 => request_session_v0(p),
        3 => make_session_unavailable_v0(p),
        4 => remove_unavailable_session_v0(p),
        5 => close_all_sessions_v0(p),
        6 => make_current_sessions_going_away_v0(p),
        7 => on_i_p_address_changed_v0(p),
        8 => on_s_s_l_config_changed_v0(p),
        9 => on_s_s_l_config_for_servers_changed_v0(p),
        10 => remove_request_for_spdy_session_v0(p),
        11 => is_session_available_v0(p),
        12 => map_key_to_available_session_v0(p),
        13 => unmap_key_v0(p),
        14 => remove_aliases_v0(p),
        15 => get_current_sessions_v0(p),
        16 => close_current_sessions_helper_v0(p),
        17 => update_pending_requests_v0(p),
        18 => remove_request_internal_v0(p),
        19 => add_connection_management_config_v0(p),
        20 => notify_on_session_closed_v0(p),
        21 => notify_on_connection_failure_v0(p),
        22 => find_matching_ip_session_for_service_endpoint_v0(p),
        23 => get_dns_aliases_for_session_key_v0(p),
        24 => spdy_session_pool_info_to_value_v0(p),
        25 => create_available_session_from_socket_handle_v0(p),
        26 => create_available_session_from_socket_v0(p),
        27 => create_session_v0(p),
        28 => insert_session_v0(p),
        29 => on_host_resolution_complete_v0(p),
        30 => on_host_resolution_complete_v1(p),
        31 => find_matching_ip_session_v0(p),
        32 => try_pool_to_existing_ip_session_v0(p),
        33 => spdy_session_request_spdy_session_request_v0(p),
        34 => spdy_session_pool_v0(p),
        35 => _spdy_session_pool_v0(p),
        36 => is_endpoint_eligible_for_h2_v0(p),
        37 => append_unique_ip_endpoints_v0(p),
        _ => panic!("invalid pool method"),
    }
}
