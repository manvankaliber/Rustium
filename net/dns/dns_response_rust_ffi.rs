// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
pub mod dns_response_rust_core;
#[cxx::bridge(namespace = "net::dns_response_rust")]
pub mod ffi {
    struct ParseOutput {
        name: [u8; 255],
        len: usize,
        touched: bool,
        fields: u8,
        kind: u16,
        klass: u16,
        ttl: u32,
        rdata_offset: usize,
        rdata_len: usize,
        has_rdata: bool,
    }
    struct Question {
        name: Vec<u8>,
        qtype: u16,
    }
    struct Record {
        name: Vec<u8>,
        kind: u16,
        klass: u16,
        ttl: u32,
        data: Vec<u8>,
    }
    struct ParseResult {
        success: bool,
        id_available: bool,
        reset_parser: bool,
        offset: usize,
        records: usize,
        questions: Vec<Question>,
    }
    struct BuildResult {
        success: bool,
        bytes: Vec<u8>,
    }
    unsafe extern "C++" {
        include!("net/dns/dns_response_rust_ffi.h");
        type ParserContext;
        fn aliased(ctx: &ParserContext) -> bool;
        fn commit(ctx: Pin<&mut ParserContext>, output: &ParseOutput);
        fn packet(ctx: &ParserContext) -> &[u8];
        fn clear_name(ctx: Pin<&mut ParserContext>);
        fn name_empty(ctx: &ParserContext) -> bool;
        fn append_name(ctx: Pin<&mut ParserContext>, bytes: &[u8]);
        fn field(ctx: Pin<&mut ParserContext>, index: u8, value: u32);
        fn rdata(ctx: Pin<&mut ParserContext>, offset: usize, len: usize);
    }
    extern "Rust" {
        fn parse(ctx: Pin<&mut ParserContext>, mode: u8, offset: usize, allowed: bool) -> usize;
        fn init_parse(
            input: &[u8],
            question: &[u8],
            qname: &[u8],
            query_id: u16,
            qtype: u16,
            matching: bool,
        ) -> ParseResult;
        fn field_value(input: &[u8], index: usize) -> u16;
        fn valid_rcode(rcode: u8) -> bool;
        fn record_size(name: &[u8], kind: u16, owned: usize, borrowed: usize) -> usize;
        #[allow(clippy::too_many_arguments)]
        fn build(
            id: u16,
            authoritative: bool,
            rcode: u8,
            query_id: u16,
            qtype: u16,
            question: &[u8],
            has_query: bool,
            records: &[Record],
            answers: usize,
            authority: usize,
            validate: bool,
            internet: bool,
        ) -> BuildResult;
    }
}
struct Io<'a>(std::pin::Pin<&'a mut ffi::ParserContext>);
impl dns_response_rust_core::ParserIo for Io<'_> {
    fn packet(&self) -> &[u8] {
        ffi::packet(&self.0)
    }
    fn clear_name(&mut self) {
        ffi::clear_name(self.0.as_mut());
    }
    fn name_empty(&self) -> bool {
        ffi::name_empty(&self.0)
    }
    fn append_name(&mut self, b: &[u8]) {
        ffi::append_name(self.0.as_mut(), b);
    }
    fn field(&mut self, i: u8, v: u32) {
        ffi::field(self.0.as_mut(), i, v);
    }
    fn rdata(&mut self, o: usize, n: usize) {
        ffi::rdata(self.0.as_mut(), o, n);
    }
}
struct FastIo<'a> {
    packet: &'a [u8],
    output: ffi::ParseOutput,
}
impl dns_response_rust_core::ParserIo for FastIo<'_> {
    fn packet(&self) -> &[u8] {
        self.packet
    }
    fn clear_name(&mut self) {
        self.output.touched = true;
        self.output.len = 0;
    }
    fn name_empty(&self) -> bool {
        self.output.len == 0
    }
    fn append_name(&mut self, b: &[u8]) {
        let n = self.output.len;
        self.output.name[n..n + b.len()].copy_from_slice(b);
        self.output.len += b.len();
    }
    fn field(&mut self, i: u8, v: u32) {
        self.output.fields |= 1 << i;
        match i {
            0 => self.output.kind = v as u16,
            1 => self.output.klass = v as u16,
            _ => self.output.ttl = v,
        }
    }
    fn rdata(&mut self, o: usize, n: usize) {
        self.output.has_rdata = true;
        self.output.rdata_offset = o;
        self.output.rdata_len = n;
    }
}
fn dispatch(
    io: &mut impl dns_response_rust_core::ParserIo,
    mode: u8,
    offset: usize,
    allowed: bool,
) -> usize {
    match mode {
        0 => dns_response_rust_core::read_name(io, offset, allowed),
        1 => dns_response_rust_core::parse_record(io, offset, allowed),
        _ => dns_response_rust_core::parse_question(io, offset),
    }
}
fn parse(
    mut ctx: std::pin::Pin<&mut ffi::ParserContext>,
    mode: u8,
    offset: usize,
    allowed: bool,
) -> usize {
    if ffi::aliased(&ctx) {
        return dispatch(&mut Io(ctx), mode, offset, allowed);
    }
    let (consumed, output) = {
        let mut io = FastIo {
            packet: ffi::packet(&ctx),
            output: ffi::ParseOutput {
                name: [0; 255],
                len: 0,
                touched: false,
                fields: 0,
                kind: 0,
                klass: 0,
                ttl: 0,
                rdata_offset: 0,
                rdata_len: 0,
                has_rdata: false,
            },
        };
        let consumed = dispatch(&mut io, mode, offset, allowed);
        (consumed, io.output)
    };
    // The packet borrow ended. aliased() verified commits cannot change input.
    ffi::commit(ctx.as_mut(), &output);
    consumed
}

use dns_response_rust_core::{build, field_value, init_parse, record_size, valid_rcode};
