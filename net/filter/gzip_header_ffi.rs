// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

mod gzip_header_parser;

// SAFETY: State is repr(C), Copy, and contains only fixed-width integers.
// gzip_header_state.h defines the identical trivial C++ layout. Both languages
// assert its size, alignment and field offsets at compile time; differential
// tests also compare all state fields after each input chunk.
unsafe impl cxx::ExternType for gzip_header_parser::State {
    type Id = cxx::type_id!("net::gzip_header_internal::State");
    type Kind = cxx::kind::Trivial;
}

#[cxx::bridge(namespace = "net::gzip_header_internal")]
mod ffi {
    unsafe extern "C++" {
        include!("net/filter/gzip_header_state.h");
        type State = super::gzip_header_parser::State;
    }
    extern "Rust" {
        fn read_more(input: &[u8], state: &mut State, header_end: &mut usize) -> u8;
        fn read_byte(input: u8, state: &mut State, header_end: &mut usize) -> u8;
        fn has_header(input: &[u8]) -> bool;
    }
}

fn read_more(input: &[u8], state: &mut gzip_header_parser::State, header_end: &mut usize) -> u8 {
    state.read_more(input, header_end) as u8
}

fn has_header(input: &[u8]) -> bool {
    gzip_header_parser::has_header(input)
}

fn read_byte(input: u8, state: &mut gzip_header_parser::State, header_end: &mut usize) -> u8 {
    state.read_more(&[input], header_end) as u8
}
