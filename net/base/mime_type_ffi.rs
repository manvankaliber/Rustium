// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

mod mime_type_parser;

#[cxx::bridge(namespace = "net::mime_type_internal")]
mod ffi {
    struct Span {
        start: usize,
        end: usize,
        scratch: bool,
    }
    struct Parameter {
        name: Span,
        value: Span,
    }
    struct Parsed {
        valid: bool,
        mime: Span,
        parameters: Vec<Parameter>,
        scratch: Vec<u8>,
    }
    struct Components {
        valid: bool,
        top: Span,
        sub: Span,
    }
    struct ContentTypePlan {
        write_mime: bool,
        mime: Span,
        write_charset: bool,
        charset: Span,
        write_boundary: bool,
        boundary: Span,
        scratch: Vec<u8>,
    }
    // SAFETY: State getters only borrow initialized caller storage during this
    // call. Rust controls when a getter is invoked (invalid input reads
    // neither, and equal MIME does not read had_charset). They cannot mutate
    // storage or retain a borrow. C++ commits staged outputs only after all
    // Rust calls end.
    unsafe extern "C++" {
        include!("net/base/mime_type_rust_state.h");
        type ContentTypeState;
        fn current_mime(self: &ContentTypeState) -> &[u8];
        fn had_charset(self: &ContentTypeState) -> bool;
    }
    extern "Rust" {
        fn parse_mime(input: &[u8], want_parameters: bool) -> Parsed;
        fn parse_without_parameters(input: &[u8]) -> Components;
        fn parse_content_type(
            input: &[u8],
            state: &ContentTypeState,
            want_boundary: bool,
            boundary_is_mime: bool,
        ) -> ContentTypePlan;
    }
}

fn span(value: mime_type_parser::Span) -> ffi::Span {
    ffi::Span { start: value.start, end: value.end, scratch: value.scratch }
}

fn parse_mime(input: &[u8], want_parameters: bool) -> ffi::Parsed {
    let mut parameters = Vec::new();
    let parsed = mime_type_parser::parse(input, want_parameters, |name, value| {
        parameters.push(ffi::Parameter { name: span(name), value: span(value) });
    });
    let valid = parsed.is_some();
    let parsed = parsed.unwrap_or_default();
    ffi::Parsed { valid, mime: span(parsed.mime), parameters, scratch: parsed.scratch }
}

fn parse_without_parameters(input: &[u8]) -> ffi::Components {
    let parsed = mime_type_parser::without_parameters(input);
    let valid = parsed.is_some();
    let (top, sub) = parsed.unwrap_or_default();
    ffi::Components { valid, top: span(top), sub: span(sub) }
}

fn parse_content_type(
    input: &[u8],
    state: &ffi::ContentTypeState,
    want_boundary: bool,
    boundary_is_mime: bool,
) -> ffi::ContentTypePlan {
    let plan = mime_type_parser::content_type(
        input,
        || state.current_mime(),
        || state.had_charset(),
        want_boundary,
        boundary_is_mime,
    );
    ffi::ContentTypePlan {
        write_mime: plan.write_mime,
        mime: span(plan.mime),
        write_charset: plan.write_charset,
        charset: span(plan.charset),
        write_boundary: plan.write_boundary,
        boundary: span(plan.boundary),
        scratch: plan.scratch,
    }
}
