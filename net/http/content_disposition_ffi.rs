// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
mod content_disposition_parser;

#[cxx::bridge(namespace = "net::content_disposition_internal")]
mod ffi {
    struct Conversion {
        ok: bool,
        bytes: Vec<u8>,
    }
    struct ParsedDisposition {
        attachment: bool,
        flags: u32,
        filename: Vec<u8>,
    }
    // SAFETY: CXX validates the slice and vector layouts. convert borrows
    // immutable inputs only for this call, retains no pointers, and returns
    // owned bytes. It never receives a pointer to an uninitialized output.
    unsafe extern "C++" {
        include!("net/http/content_disposition_services.h");
        fn convert(input: &[u8], charset: &[u8], mode: u8) -> Conversion;
    }
    extern "Rust" {
        fn parse_disposition(input: &[u8], charset: &[u8], first_value: bool) -> ParsedDisposition;
    }
}
struct Services;
impl content_disposition_parser::CharsetServices for Services {
    fn convert(&self, input: &[u8], charset: &[u8], mode: u8) -> Option<Vec<u8>> {
        let result = ffi::convert(input, charset, mode);
        result.ok.then_some(result.bytes)
    }
}
fn parse_disposition(input: &[u8], charset: &[u8], first_value: bool) -> ffi::ParsedDisposition {
    let result = content_disposition_parser::parse(input, charset, first_value, &Services);
    ffi::ParsedDisposition {
        attachment: result.attachment,
        flags: result.flags,
        filename: result.filename,
    }
}
