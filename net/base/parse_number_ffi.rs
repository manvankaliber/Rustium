// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

use parse_number_core as parser;

#[cxx::bridge(namespace = "net::parse_number_internal")]
mod ffi {
    struct I32Result {
        value: i32,
        status: u8,
    }
    struct I64Result {
        value: i64,
        status: u8,
    }
    struct U32Result {
        value: u32,
        status: u8,
    }
    struct U64Result {
        value: u64,
        status: u8,
    }
    extern "Rust" {
        fn parse_i32(input: &[u8], format: i32, report_error: bool) -> I32Result;
        fn parse_i64(input: &[u8], format: i32, report_error: bool) -> I64Result;
        fn parse_u32(input: &[u8], format: i32, report_error: bool) -> U32Result;
        fn parse_u64(input: &[u8], format: i32, report_error: bool) -> U64Result;
    }
}

fn parse_i32(input: &[u8], format: i32, report_error: bool) -> ffi::I32Result {
    match parser::parse_i32(input, format, report_error) {
        Ok(value) => ffi::I32Result { value, status: 0 },
        Err(error) => ffi::I32Result { value: 0, status: error as u8 },
    }
}

fn parse_i64(input: &[u8], format: i32, report_error: bool) -> ffi::I64Result {
    match parser::parse_i64(input, format, report_error) {
        Ok(value) => ffi::I64Result { value, status: 0 },
        Err(error) => ffi::I64Result { value: 0, status: error as u8 },
    }
}

fn parse_u32(input: &[u8], format: i32, report_error: bool) -> ffi::U32Result {
    match parser::parse_u32(input, format, report_error) {
        Ok(value) => ffi::U32Result { value, status: 0 },
        Err(error) => ffi::U32Result { value: 0, status: error as u8 },
    }
}

fn parse_u64(input: &[u8], format: i32, report_error: bool) -> ffi::U64Result {
    match parser::parse_u64(input, format, report_error) {
        Ok(value) => ffi::U64Result { value, status: 0 },
        Err(error) => ffi::U64Result { value: 0, status: error as u8 },
    }
}
