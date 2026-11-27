// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#![forbid(unsafe_code)]

//! Chromium HTTP/3 Stream and Handle control flow. Services perform individual
//! native ownership, QUICHE, task, clock, logging, or callback operations.
//! No native object/reference is held in this module. Reentrant operations
//! finish before another service is requested. Callback completion is terminal
//! unless a separate stack-owned weak guard proves the owner survived.
pub trait Services {
    fn call(&mut self, operation: u32, argument: i64) -> i64;
}

pub fn run(method: u32, s: &mut impl Services) -> i64 {
    match method {
        0 => {
            // Handle::~Handle
            s.call(0, 0);
            if s.call(1, 0) != 0 {
                s.call(2, 0);
            }
            0
        }
        1 => {
            // Handle::OnEarlyHintsAvailable
            if s.call(0, 0) != 0 {
                s.call(1, 0);
            }
            if !(s.call(2, 0) != 0) {
                return 0;
            }
            s.call(3, 0);
            s.call(4, 0);
            s.call(5, 0);
            0
        }
        2 => {
            // Handle::OnInitialHeadersAvailable
            if s.call(0, 0) != 0 {
                s.call(1, 0);
            }
            if !(s.call(2, 0) != 0) {
                return 0;
            }

            s.call(3, 0);
            s.call(4, 0);
            0
        }
        3 => {
            // Handle::OnTrailingHeadersAvailable
            if !(s.call(0, 0) != 0) {
                return 0;
            }
            if !(s.call(1, 0) != 0) {
                s.call(2, 0);
            }
            s.call(3, 0);
            s.call(4, 0);
            0
        }
        4 => {
            // Handle::OnDataAvailable
            if !(s.call(0, 0) != 0) {
                return 0;
            }
            s.call(1, 0);
            let rv = s.call(2, 0);
            if rv == -1 {
                return 0;
            }
            s.call(3, 0);
            s.call(4, 0);
            0
        }
        5 => {
            // Handle::OnCanWrite
            if s.call(0, 0) != 0 {
                s.call(1, 0);
            }
            0
        }
        6 => {
            // Handle::OnClose
            if s.call(0, 0) == -9 {
                let clean = s.call(1, 0) != 0
                    && s.call(2, 0) != 0
                    && s.call(3, 0) != 0
                    && s.call(4, 0) != 0;
                let error = if clean { -100 } else { -356 };
                s.call(5, error);
            }
            s.call(6, 0);
            s.call(7, 0);
            0
        }
        7 => {
            // Handle::OnError
            s.call(0, 0);
            s.call(1, 0);
            if s.call(2, 0) != 0 {
                s.call(3, 0);
            }
            s.call(4, 0);
            s.call(5, 0);
            0
        }
        8 => {
            // Handle::InvokeCallbacksOnClose
            s.call(0, 0);
            for slot in 0..3 {
                if s.call(1, slot) != 0 {
                    s.call(2, slot);
                }
                // The guard is stack-owned. No owner service may run after
                // deletion.
                if !(s.call(3, 0) != 0) {
                    return 0;
                }
            }
            0
        }
        9 => {
            // Handle::ReadInitialHeaders
            if !(s.call(0, 0) != 0) {
                return s.call(1, 0);
            }
            let rv = s.call(2, 0);
            if rv != -1 {
                return rv;
            }
            let rv = s.call(3, 0);
            if rv != -1 {
                return rv;
            }
            s.call(4, 0);
            s.call(5, 0);
            -1
        }
        10 => {
            // Handle::ReadBody
            if s.call(0, 0) != 0 {
                return 0;
            }
            if !(s.call(1, 0) != 0) {
                return s.call(2, 0);
            }
            if s.call(3, 0) != 0 {
                return 0;
            }
            let rv = s.call(4, 0);
            if rv != -1 {
                return rv;
            }
            s.call(5, 0);
            s.call(6, 0);
            s.call(7, 0);
            -1
        }
        11 => {
            // Handle::ReadTrailingHeaders
            if !(s.call(0, 0) != 0) {
                return s.call(1, 0);
            }
            if s.call(2, 0) != 0 {
                return s.call(3, 0);
            }
            s.call(4, 0);
            s.call(5, 0);
            -1
        }
        12 => {
            // Handle::WriteHeaders
            if !(s.call(0, 0) != 0) {
                return 0;
            }
            s.call(1, 0)
        }
        13 => {
            // Handle::WriteStreamData
            if !(s.call(0, 0) != 0) {
                return s.call(1, 0);
            }
            if s.call(2, 0) != 0 {
                return s.call(3, 0);
            }
            s.call(4, 0);
            -1
        }
        14 => {
            // Handle::WritevStreamData
            if !(s.call(0, 0) != 0) {
                return s.call(1, 0);
            }
            if s.call(2, 0) != 0 {
                return s.call(3, 0);
            }
            s.call(4, 0);
            -1
        }
        15 => {
            // Handle::WriteConnectUdpPayload
            if !(s.call(0, 0) != 0) {
                return s.call(1, 0);
            }
            s.call(2, 0);
            if !(s.call(3, 0) != 0) {
                s.call(4, 0);
                return 0;
            }
            s.call(5, 0);
            s.call(6, 0);
            if s.call(7, 0) != 0 || s.call(8, 0) != 0 {
                return s.call(9, 0);
            }
            s.call(10, 0);
            s.call(11, 0);
            -100
        }
        16 => {
            // Handle::Read
            if !(s.call(0, 0) != 0) {
                return s.call(1, 0);
            }
            s.call(2, 0)
        }
        17 => {
            // Handle::OnFinRead
            s.call(0, 0);
            if s.call(1, 0) != 0 {
                s.call(2, 0);
            }
            0
        }
        18 => {
            // Handle::DisableConnectionMigrationToCellularNetwork

            if s.call(0, 0) != 0 {
                s.call(1, 0);
            }
            0
        }
        19 => {
            // Handle::SetPriority

            if s.call(0, 0) != 0 {
                s.call(1, 0);
            }
            0
        }
        20 => {
            // Handle::Reset
            s.call(0, 0);
            if s.call(1, 0) != 0 {
                s.call(2, 0);
            }
            0
        }
        21 => {
            // Handle::RegisterHttp3DatagramVisitor

            if s.call(0, 0) != 0 {
                s.call(1, 0);
            }
            0
        }
        22 => {
            // Handle::UnregisterHttp3DatagramVisitor
            if s.call(0, 0) != 0 && s.call(1, 0) != 0 {
                s.call(2, 0);
                s.call(3, 0);
            }
            0
        }
        23 => {
            // Handle::id
            if !(s.call(0, 0) != 0) {
                return s.call(1, 0);
            }

            s.call(2, 0)
        }
        24 => {
            // Handle::connection_error
            if !(s.call(0, 0) != 0) {
                return s.call(1, 0);
            }

            s.call(2, 0)
        }
        25 => {
            // Handle::stream_error
            if !(s.call(0, 0) != 0) {
                return s.call(1, 0);
            }

            s.call(2, 0)
        }
        26 => {
            // Handle::connection_wire_error
            if !(s.call(0, 0) != 0) {
                return s.call(1, 0);
            }
            s.call(2, 0);
            s.call(3, 0)
        }
        27 => {
            // Handle::ietf_application_error
            if !(s.call(0, 0) != 0) {
                return s.call(1, 0);
            }

            s.call(2, 0)
        }
        28 => {
            // Handle::fin_sent
            if !(s.call(0, 0) != 0) {
                return s.call(1, 0);
            }

            s.call(2, 0)
        }
        29 => {
            // Handle::fin_received
            if !(s.call(0, 0) != 0) {
                return s.call(1, 0);
            }

            s.call(2, 0)
        }
        30 => {
            // Handle::stream_bytes_read
            if !(s.call(0, 0) != 0) {
                return s.call(1, 0);
            }

            s.call(2, 0)
        }
        31 => {
            // Handle::stream_bytes_written
            if !(s.call(0, 0) != 0) {
                return s.call(1, 0);
            }

            s.call(2, 0)
        }
        32 => {
            // Handle::NumBytesConsumed
            if !(s.call(0, 0) != 0) {
                return s.call(1, 0);
            }

            s.call(2, 0)
        }
        33 => {
            // Handle::HasBytesToRead
            if !(s.call(0, 0) != 0) {
                return s.call(1, 0);
            }

            s.call(2, 0)
        }
        34 => {
            // Handle::IsDoneReading
            if !(s.call(0, 0) != 0) {
                return s.call(1, 0);
            }

            s.call(2, 0)
        }
        35 => {
            // Handle::IsFirstStream
            if !(s.call(0, 0) != 0) {
                return s.call(1, 0);
            }

            s.call(2, 0)
        }
        36 => {
            // Handle::can_migrate_to_cellular_network
            if !(s.call(0, 0) != 0) {
                return s.call(1, 0);
            }

            s.call(2, 0)
        }
        37 => {
            // Handle::GetGuaranteedLargestMessagePayload
            if !(s.call(0, 0) != 0) {
                return s.call(1, 0);
            }

            s.call(2, 0)
        }
        38 => {
            // Handle::HandleIOComplete
            let rv = s.call(0, 0);
            if rv < 0 || s.call(1, 0) != 0 {
                return rv;
            }
            if s.call(2, 0) != 0 && s.call(3, 0) != 0 && s.call(4, 0) != 0 && s.call(5, 0) != 0 {
                return rv;
            }
            s.call(6, 0)
        }
        39 => {
            // ~QuicChromiumClientStream
            if s.call(0, 0) != 0 {
                s.call(1, 0);
            }
            0
        }
        40 => {
            // OnInitialHeadersComplete
            s.call(0, 0);
            s.call(1, 0);
            if s.call(2, 0) != 0 {
                s.call(3, 0);
                if s.call(4, 0) != 0 {
                    s.call(5, 0);
                }
            }
            if !(s.call(6, 0) != 0) {
                s.call(7, 0);
                s.call(8, 0);
                s.call(9, 0);
                return 0;
            }
            if !(s.call(10, 0) != 0) {
                s.call(11, 0);
                s.call(12, 0);
                return 0;
            }
            let status = s.call(13, 0);
            if status == 101 {
                s.call(14, 0);
                s.call(15, 0);
                return 0;
            }
            if (100..200).contains(&status) {
                s.call(16, 0);
                s.call(17, 0);
                if status == 103 {
                    s.call(18, 0);
                    if s.call(19, 0) != 0 {
                        s.call(20, 0);
                    }
                } else {
                    s.call(21, 0);
                }
                return 0;
            }
            s.call(22, 0);
            s.call(23, 0);
            if s.call(24, 0) != 0 {
                s.call(25, 0);
            }
            0
        }
        41 => {
            // OnTrailingHeadersComplete
            s.call(0, 0);
            s.call(1, 0);
            if s.call(2, 0) != 0 {
                s.call(3, 0);
            }
            0
        }
        42 => {
            // OnBodyAvailable
            if !(s.call(0, 0) != 0) || !(s.call(1, 0) != 0) {
                return 0;
            }
            if !(s.call(2, 0) != 0) && !(s.call(3, 0) != 0) {
                return 0;
            }
            if s.call(4, 0) != 0 {
                s.call(5, 0);
            }
            0
        }
        43 => {
            // OnClose
            if s.call(0, 0) != 0 {
                s.call(1, 0);
                s.call(2, 0);
            }
            s.call(3, 0);
            0
        }
        44 => {
            // OnCanWrite
            s.call(0, 0);
            if !(s.call(1, 0) != 0) && s.call(2, 0) != 0 {
                s.call(3, 0);
            }
            0
        }
        45 => {
            // WriteHeaders
            if !(s.call(0, 0) != 0) {
                s.call(1, 0);
                if s.call(2, 0) != 0 && !(s.call(3, 0) != 0 && s.call(4, 0) != 0) {
                    s.call(5, 0);
                }
            }
            s.call(6, 0);
            let len = s.call(7, 0);
            s.call(8, 0);
            len
        }
        46 => {
            // WriteStreamData
            s.call(0, 0);
            if s.call(1, 0) != 0 {
                0
            } else {
                1
            }
        }
        47 => {
            // WritevStreamData
            let count = s.call(0, 0) as u64;
            let fin = s.call(1, 0) != 0;
            for index in 0..count {
                let is_fin = fin && index == count - 1;
                s.call(2, ((index << 1) | u64::from(is_fin)) as i64);
            }
            if s.call(3, 0) != 0 {
                0
            } else {
                1
            }
        }
        48 => {
            // CreateHandle
            s.call(0, 0);
            s.call(1, 0);
            if s.call(2, 0) != 0 {
                s.call(3, 0);
            }
            0
        }
        49 => {
            // OnError
            if s.call(0, 0) != 0 {
                s.call(1, 0);
                s.call(2, 0);
            }
            0
        }
        50 => {
            // Read
            s.call(0, 0);
            if s.call(1, 0) != 0 {
                return 0;
            }
            if !(s.call(2, 0) != 0) {
                return -1;
            }
            s.call(3, 0);
            let bytes = s.call(4, 0);
            s.call(5, 0);
            bytes
        }
        51 => {
            // NotifyHandleOfInitialHeadersAvailable
            if s.call(0, 0) != 0 && !(s.call(1, 0) != 0) {
                s.call(2, 0);
            }
            0
        }
        52 => {
            // NotifyHandleOfTrailingHeadersAvailable
            if !(s.call(0, 0) != 0) {
                return 0;
            }
            if !(s.call(1, 0) != 0) {
                return 0;
            }
            if !(s.call(2, 0) != 0) {
                return 0;
            }
            s.call(3, 0);
            s.call(4, 0);
            0
        }
        53 => {
            // DeliverEarlyHints
            if s.call(0, 0) != 0 {
                return -1;
            }
            s.call(1, 0);
            s.call(2, 0);
            s.call(3, 0);
            s.call(4, 0)
        }
        54 => {
            // DeliverInitialHeaders
            if !(s.call(0, 0) != 0) {
                return -1;
            }
            s.call(1, 0);
            if s.call(2, 0) != 0 {
                return -320;
            }
            s.call(3, 0);
            s.call(4, 0);
            s.call(5, 0)
        }
        55 => {
            // DeliverTrailingHeaders
            if s.call(0, 0) == 0 {
                return 0;
            }
            s.call(1, 0);
            s.call(2, 0);
            s.call(3, 0);
            1
        }
        56 => {
            // NotifyHandleOfDataAvailable
            if s.call(0, 0) != 0 {
                s.call(1, 0);
            }
            0
        }
        57 => {
            // GetGuaranteedLargestMessagePayload
            if !(s.call(0, 0) != 0) {
                return 0;
            }
            s.call(1, 0)
        }
        58 => {
            // Handle::SupportsH3Datagram
            if s.call(0, 0) != 0 {
                s.call(1, 0)
            } else {
                0
            }
        }
        _ => unreachable!("private adapter method id"),
    }
}

/// Local callback gate; native ownership retains the original CHECK diagnostic.
pub fn callback_gate(current: bool, expected: bool) -> bool {
    current == expected
}
pub fn is_first_stream(id: u32, first: u32) -> bool {
    id == first
}
