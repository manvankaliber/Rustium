// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

mod data_pack_parser;

#[cxx::bridge(namespace = "ui::data_pack_internal")]
mod ffi {
    enum ParseStatus {
        Ok,
        IncompleteHeader,
        BadVersion,
        BadEncoding,
        TooShort,
        BoundsExceeded,
        OrderingViolation,
        AliasTableCorrupt,
    }

    struct ParseResult {
        status: ParseStatus,
        resource_count: usize,
        alias_count: usize,
        resource_table_offset: usize,
        alias_table_offset: usize,
        encoding: u8,
    }

    enum LookupStatus {
        Found,
        NotFound,
        CorruptIndex,
        BoundsExceeded,
        OrderingViolation,
    }

    struct LookupResult {
        status: LookupStatus,
        offset: u32,
        length: u32,
    }

    extern "Rust" {
        fn parse_data_pack(data: &[u8]) -> ParseResult;
        fn has_data_pack_resource(
            data: &[u8],
            resource_count: usize,
            alias_count: usize,
            resource_table_offset: usize,
            id: u16,
        ) -> bool;
        fn lookup_data_pack_resource(
            data: &[u8],
            resource_count: usize,
            alias_count: usize,
            resource_table_offset: usize,
            id: u16,
        ) -> LookupResult;
    }
}

fn has_data_pack_resource(
    data: &[u8],
    count: usize,
    aliases: usize,
    start: usize,
    id: u16,
) -> bool {
    data_pack_parser::has_resource(data, count, aliases, start, id)
}

fn lookup_data_pack_resource(
    data: &[u8],
    count: usize,
    aliases: usize,
    start: usize,
    id: u16,
) -> ffi::LookupResult {
    let status = match data_pack_parser::lookup(data, count, aliases, start, id) {
        Ok(Some(range)) => {
            return ffi::LookupResult {
                status: ffi::LookupStatus::Found,
                offset: range.offset,
                length: range.length,
            };
        }
        Ok(None) => ffi::LookupStatus::NotFound,
        Err(data_pack_parser::LookupError::CorruptIndex) => ffi::LookupStatus::CorruptIndex,
        Err(data_pack_parser::LookupError::BoundsExceeded) => ffi::LookupStatus::BoundsExceeded,
        Err(data_pack_parser::LookupError::OrderingViolation) => {
            ffi::LookupStatus::OrderingViolation
        }
    };
    ffi::LookupResult { status, offset: 0, length: 0 }
}

fn parse_data_pack(data: &[u8]) -> ffi::ParseResult {
    match data_pack_parser::parse(data) {
        Ok(index) => ffi::ParseResult {
            status: ffi::ParseStatus::Ok,
            resource_count: index.resource_count,
            alias_count: index.alias_count,
            resource_table_offset: index.resource_table_offset,
            alias_table_offset: index.alias_table_offset,
            encoding: index.encoding,
        },
        Err(error) => {
            use data_pack_parser::ParseError;
            ffi::ParseResult {
                status: match error {
                    ParseError::IncompleteHeader => ffi::ParseStatus::IncompleteHeader,
                    ParseError::BadVersion => ffi::ParseStatus::BadVersion,
                    ParseError::BadEncoding => ffi::ParseStatus::BadEncoding,
                    ParseError::TooShort => ffi::ParseStatus::TooShort,
                    ParseError::BoundsExceeded => ffi::ParseStatus::BoundsExceeded,
                    ParseError::OrderingViolation => ffi::ParseStatus::OrderingViolation,
                    ParseError::AliasTableCorrupt => ffi::ParseStatus::AliasTableCorrupt,
                },
                resource_count: 0,
                alias_count: 0,
                resource_table_offset: 0,
                alias_table_offset: 0,
                encoding: 0,
            }
        }
    }
}
