// Copyright 2017 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_SOCKET_TCP_CLIENT_SOCKET_RUST_NATIVE_REFERENCE_SOCKET_OPTIONS_H_
#define NET_SOCKET_TCP_CLIENT_SOCKET_RUST_NATIVE_REFERENCE_SOCKET_OPTIONS_H_

#include <stdint.h>

#include "net/base/net_export.h"
#include "net/socket/socket_descriptor.h"

namespace net {

// This function enables/disables buffering in the kernel. By default, on Linux,
// TCP sockets will wait up to 200ms for more data to complete a packet before
// transmitting. After calling this function, the kernel will not wait. See
// TCP_NODELAY in `man 7 tcp`.
//
// For Windows:
//
// The Nagle implementation on Windows is governed by RFC 896.  The idea
// behind Nagle is to reduce small packets on the network.  When Nagle is
// enabled, if a partial packet has been sent, the TCP stack will disallow
// further *partial* packets until an ACK has been received from the other
// side.  Good applications should always strive to send as much data as
// possible and avoid partial-packet sends.  However, in most real world
// applications, there are edge cases where this does not happen, and two
// partial packets may be sent back to back.  For a browser, it is NEVER
// a benefit to delay for an RTT before the second packet is sent.
//
// As a practical example in Chromium today, consider the case of a small
// POST.  I have verified this:
//     Client writes 649 bytes of header  (partial packet #1)
//     Client writes 50 bytes of POST data (partial packet #2)
// In the above example, with Nagle, a RTT delay is inserted between these
// two sends due to nagle.  RTTs can easily be 100ms or more.  The best
// fix is to make sure that for POSTing data, we write as much data as
// possible and minimize partial packets.  We will fix that.  But disabling
// Nagle also ensure we don't run into this delay in other edge cases.
// See also:
//    http://technet.microsoft.com/en-us/library/bb726981.aspx
//
// TCPClientSocketRustNativeReferenceSetTCPNoDelay() sets the TCP_NODELAY
// option. Use |no_delay| to enable or disable it. On error returns a net error
// code, on success returns OK.
int TCPClientSocketRustNativeReferenceSetTCPNoDelay(SocketDescriptor fd,
                                                    bool no_delay);

// TCPClientSocketRustNativeReferenceSetReuseAddr() sets the SO_REUSEADDR socket
// option. Use |reuse| to enable or disable it. On error returns a net error
// code, on success returns OK.
int TCPClientSocketRustNativeReferenceSetReuseAddr(SocketDescriptor fd,
                                                   bool reuse);

// TCPClientSocketRustNativeReferenceSetReceiveBufferSize() sets the SO_RCVBUF
// socket option. On error returns a net error code, on success returns OK.
int TCPClientSocketRustNativeReferenceSetReceiveBufferSize(SocketDescriptor fd,
                                                           int32_t size);

// TCPClientSocketRustNativeReferenceSetSendBufferSize() sets the SO_SNDBUF
// socket option. On error returns a net error code, on success returns OK.
int TCPClientSocketRustNativeReferenceSetSendBufferSize(SocketDescriptor fd,
                                                        int32_t size);

// TCPClientSocketRustNativeReferenceSetIPv6Only() sets the IPV6_V6ONLY socket
// option. On error returns a net error code, on success returns OK.
int TCPClientSocketRustNativeReferenceSetIPv6Only(SocketDescriptor fd,
                                                  bool ipv6_only);

}  // namespace net

#endif  // NET_SOCKET_TCP_CLIENT_SOCKET_RUST_NATIVE_REFERENCE_SOCKET_OPTIONS_H_
