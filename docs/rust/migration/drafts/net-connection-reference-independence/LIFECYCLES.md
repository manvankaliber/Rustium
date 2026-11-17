# Existing 16 lifecycle verification obligations

Source: hash-bound `reference/current/net/socket/connection_jobs_upper_rust_job_workloads.h`,
`.cc`, and `connection_jobs_upper_rust_jobs_perftest.cc`. This is a mapping of
existing executable verification source, not a new test result. The driver
iterates all cases in the order below, prepares 32 scripts for each side,
checks each side's result and auth count, then compares full checksums. It
requires QUIC proxy support for the complete set. The `--verify-only` path
does not read measurement clocks. Preserve the existing script-consumption
checks, cancellation/teardown and exact checksum construction; HTTP2 pending,
QUIC pending and QUIC error are the existing consumption exceptions. The
timing path retains 20 paired repetitions and 32 lifecycles per side.

| ID | Existing expected result | Source-mapped behavior to verify |
|---|---|---|
| tls_sync | OK | synchronous transport and TLS; script consumed |
| tls_async | OK | asynchronous resolver/connect/TLS; task drain and script consumed |
| tls_error | ERR_FAILED | TLS failure; script consumed |
| tls_transport_error | ERR_CONNECTION_FAILED | transport failure before TLS; script consumed |
| tls_timeout | ERR_TIMED_OUT | pending TLS, advance mock time 30 seconds, teardown; script consumed |
| socks4 | OK | SOCKS4 request/reply; script consumed |
| socks5 | OK | SOCKS5 greeting/request/reply; script consumed |
| socks_error | ERR_UNEXPECTED | failed socket write; script consumed |
| http_direct | OK | HTTP proxy without tunnel; script consumed |
| http_tunnel | OK | HTTP/1 CONNECT 200, original auth cache negative lookup and header/parser paths; script consumed |
| http_auth | ERR_IO_PENDING | CONNECT 407 Basic challenge; exactly one auth callback, no invented token/restart; script consumed |
| https_direct | OK | secure proxy without tunnel; script consumed |
| https_tunnel | OK | secure proxy and HTTP/1 CONNECT 200; script consumed |
| http2_pending | ERR_IO_PENDING | H2 CONNECT pending; job reset, close correct side's pool and drain tasks |
| quic_pending | ERR_IO_PENDING | QUIC request pending, job cancellation; provider is not a successful tunnel |
| quic_error | ERR_FAILED | QUIC request failure; provider is not a successful tunnel |

Every case other than `http_auth` expects zero auth callbacks. All cases
contribute completion count, socket/error state, resolve error and timing-field
presence to the checksum; QUIC additionally includes request/cancel counts.
The fixture destroys jobs and drains tasks before providers, pool/session and
script storage; preserve that order. The preexisting fixture source, rather
than this table, remains the authoritative executable assertion and count.

Separate new provider-routing verification must cover NTLM first and later
rounds on portable and Windows routes, Negotiate on each supported provider,
captured and uncaptured logging, invalid/missing tokens, decoded-output
preservation and aliasing. The adapter unit source covers algorithm cases but
does not prove any registry/provider route or the other migrated policy families.
