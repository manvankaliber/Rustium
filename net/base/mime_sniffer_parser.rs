// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

//! Chromium's existing MIME-sniffing policy. No new formats or acceptance
//! rules. Input bytes are borrowed for one call; results refer only to static
//! strings.

#![forbid(unsafe_code)]

const MAX_SNIFF: usize = 1024;
const MAGIC_BYTES: usize = 42;
const OFFICE_BYTES: usize = 8;

struct Magic {
    mime: &'static str,
    bytes: &'static [u8],
    insensitive: bool,
    mask: &'static [u8],
}

impl Magic {
    const fn new(mime: &'static str, bytes: &'static [u8]) -> Self {
        assert!(bytes.len() <= MAGIC_BYTES);
        Self { mime, bytes, insensitive: false, mask: &[] }
    }

    const fn insensitive(mime: &'static str, bytes: &'static [u8]) -> Self {
        Self { mime, bytes, insensitive: true, mask: &[] }
    }

    const fn masked(mime: &'static str, bytes: &'static [u8], mask: &'static [u8]) -> Self {
        assert!(bytes.len() == mask.len());
        Self { mime, bytes, insensitive: false, mask }
    }
}

const MAGIC: &[Magic] = &[
    Magic::new("application/pdf", b"%PDF-"),
    Magic::new("application/postscript", b"%!PS-Adobe-"),
    Magic::new("image/gif", b"GIF87a"),
    Magic::new("image/gif", b"GIF89a"),
    Magic::new("image/png", b"\x89PNG\x0D\x0A\x1A\x0A"),
    Magic::new("image/jpeg", b"\xFF\xD8\xFF"),
    Magic::new("image/bmp", b"BM"),
    Magic::new("text/plain", b"#!"),
    Magic::new("text/plain", b"%!"),
    Magic::new("text/plain", b"From"),
    Magic::new("text/plain", b">From"),
    Magic::new("application/x-gzip", b"\x1F\x8B\x08"),
    Magic::new("audio/x-pn-realaudio", b"\x2E\x52\x4D\x46"),
    Magic::new(
        "video/x-ms-asf",
        b"\x30\x26\xB2\x75\x8E\x66\xCF\x11\xA6\xD9\x00\xAA\x00\x62\xCE\x6C",
    ),
    Magic::new("image/tiff", b"I I"),
    Magic::new("image/tiff", b"II*"),
    Magic::new("image/tiff", b"MM\x00*"),
    Magic::new("audio/mpeg", b"ID3"),
    Magic::new("image/webp", b"RIFF....WEBPVP"),
    Magic::new("video/webm", b"\x1A\x45\xDF\xA3"),
    Magic::new("application/zip", b"PK\x03\x04"),
    Magic::new("application/x-rar-compressed", b"Rar!\x1A\x07\x00"),
    Magic::new("application/x-msmetafile", b"\xD7\xCD\xC6\x9A"),
    Magic::new("application/octet-stream", b"MZ"),
];

const EXTRA_MAGIC: &[Magic] = &[
    Magic::new("image/x-xbitmap", b"#define"),
    Magic::new("image/x-icon", b"\x00\x00\x01\x00"),
    Magic::new("audio/wav", b"RIFF....WAVEfmt "),
    Magic::new("video/avi", b"RIFF....AVI LIST"),
    Magic::new("audio/ogg", b"OggS\0"),
    Magic::masked("video/mpeg", b"\x00\x00\x01\xB0", b"\xFF\xFF\xFF\xF0"),
    Magic::masked("audio/mpeg", b"\xFF\xE0", b"\xFF\xE0"),
    Magic::new("video/3gpp", b"....ftyp3g"),
    Magic::new("video/3gpp", b"....ftypavcl"),
    Magic::new("video/mp4", b"....ftyp"),
    Magic::new("video/quicktime", b"....moov"),
    Magic::new("application/x-shockwave-flash", b"CWS"),
    Magic::new("application/x-shockwave-flash", b"FWS"),
    Magic::new("video/x-flv", b"FLV"),
    Magic::new("audio/x-flac", b"fLaC"),
    Magic::new("audio/amr", b"#!AMR\n"),
    // Preserve the C++ literal's greedy hex escape: its final \x00C is 0x0c.
    Magic::new("image/x-canon-cr2", b"II\x2A\x00\x10\x00\x00\x0CR"),
    Magic::new("image/x-canon-crw", b"II\x1A\x00\x00\x00HEAPCCDR"),
    Magic::new("image/x-minolta-mrw", b"\x00MRM"),
    Magic::new("image/x-olympus-orf", b"MMOR"),
    Magic::new("image/x-olympus-orf", b"IIRO"),
    Magic::new("image/x-olympus-orf", b"IIRS"),
    Magic::new("image/x-fuji-raf", b"FUJIFILMCCD-RAW "),
    Magic::new("image/x-panasonic-raw", b"IIU\x00\x08\x00\x00\x00"),
    Magic::new("image/x-panasonic-raw", b"IIU\x00\x18\x00\x00\x00"),
    Magic::new("image/x-phaseone-raw", b"MMMMRaw"),
    Magic::new("image/x-x3f", b"FOVb"),
];

// Exclude impossible signatures before walking the local-data tables. Bits
// retain their original table order, including every wildcard or masked match.
// This is built at compile time; sniffing still borrows only the supplied
// bytes.
const fn first_byte_index(entries: &[Magic]) -> [u32; 256] {
    assert!(entries.len() <= u32::BITS as usize);
    let mut index = [0; 256];
    let mut entry = 0;
    while entry < entries.len() {
        assert!(!entries[entry].insensitive);
        let expected = entries[entry].bytes[0];
        let mask = if entries[entry].mask.is_empty() { 0xff } else { entries[entry].mask[0] };
        let mut byte = 0;
        while byte < index.len() {
            if expected == b'.' || byte as u8 & mask == expected {
                index[byte] |= 1 << entry;
            }
            byte += 1;
        }
        entry += 1;
    }
    index
}

const MAGIC_FIRST: [u32; 256] = first_byte_index(MAGIC);
const EXTRA_MAGIC_FIRST: [u32; 256] = first_byte_index(EXTRA_MAGIC);

const HTML: &[Magic] = &[
    // This XML prefix was case-sensitive in the HTML phase.
    Magic::new("text/xml", b"<?xml"),
    Magic::insensitive("text/html", b"<!DOCTYPE html"),
    Magic::insensitive("text/html", b"<script"),
    Magic::insensitive("text/html", b"<html"),
    Magic::insensitive("text/html", b"<!--"),
    Magic::insensitive("text/html", b"<head"),
    Magic::insensitive("text/html", b"<iframe"),
    Magic::insensitive("text/html", b"<h1"),
    Magic::insensitive("text/html", b"<div"),
    Magic::insensitive("text/html", b"<font"),
    Magic::insensitive("text/html", b"<table"),
    Magic::insensitive("text/html", b"<a"),
    Magic::insensitive("text/html", b"<style"),
    Magic::insensitive("text/html", b"<title"),
    Magic::insensitive("text/html", b"<b"),
    Magic::insensitive("text/html", b"<body"),
    Magic::insensitive("text/html", b"<br"),
    Magic::insensitive("text/html", b"<p"),
];

const OFFICE: &[Magic] =
    &[Magic::new("CFB", b"\xD0\xCF\x11\xE0\xA1\xB1\x1A\xE1"), Magic::new("OOXML", b"PK\x03\x04")];

// Preserve the C++ constructors' compile-time checks for every table, including
// case-insensitive and masked entries. This block emits no runtime code.
const _: () = {
    let tables = [MAGIC, EXTRA_MAGIC, HTML, OFFICE];
    let mut table = 0;
    while table < tables.len() {
        let mut index = 0;
        while index < tables[table].len() {
            let entry = &tables[table][index];
            assert!(entry.bytes.len() <= MAGIC_BYTES);
            assert!(entry.mask.is_empty() || entry.mask.len() == entry.bytes.len());
            if entry.insensitive {
                let mut byte = 0;
                while byte < entry.bytes.len() {
                    assert!(entry.bytes[byte] != 0);
                    byte += 1;
                }
            }
            index += 1;
        }
        table += 1;
    }
};

// One list drives both Office output strings and the length-dispatched hint
// matcher, avoiding a repeated linear table walk on every sniffing operation.
macro_rules! office_types {
    ($($name:ident: $mime:literal),+ $(,)?) => {
        const OFFICE_TYPES: [&str; 14] = [$($mime),+];
        $(const $name: &[u8] = $mime.as_bytes();)+
        #[inline]
        fn office_type(hint: &[u8]) -> bool { matches!(hint, $($name)|+) }
    };
}
office_types! {
    WORD: "application/msword",
    EXCEL: "application/vnd.ms-excel",
    POWERPOINT: "application/vnd.ms-powerpoint",
    WORD_OPEN: "application/vnd.openxmlformats-officedocument.wordprocessingml.document",
    EXCEL_OPEN: "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet",
    POWERPOINT_OPEN: "application/vnd.openxmlformats-officedocument.presentationml.presentation",
    EXCEL_MACRO: "application/vnd.ms-excel.sheet.macroenabled.12",
    WORD_MACRO: "application/vnd.ms-word.document.macroenabled.12",
    POWERPOINT_MACRO: "application/vnd.ms-powerpoint.presentation.macroenabled.12",
    MSPOWERPOINT: "application/mspowerpoint",
    MSEXCEL: "application/msexcel",
    MS_WORD: "application/vnd.ms-word",
    MS_WORD_12: "application/vnd.ms-word.document.12",
    MSWORD: "application/vnd.msword",
}

#[inline]
fn magic(content: &[u8], entries: &[Magic]) -> Option<&'static str> {
    for entry in entries {
        let Some(prefix) = content.get(..entry.bytes.len()) else { continue };
        let matched = if entry.insensitive {
            prefix.eq_ignore_ascii_case(entry.bytes)
        } else if entry.mask.is_empty() {
            // Dots are wildcards even when originally spelled as \x2E.
            prefix
                .iter()
                .zip(entry.bytes)
                .all(|(&byte, &expected)| expected == b'.' || byte == expected)
        } else {
            prefix
                .iter()
                .zip(entry.bytes)
                .zip(entry.mask)
                .all(|((&byte, &expected), &mask)| expected == b'.' || byte & mask == expected)
        };
        if matched {
            return Some(entry.mime);
        }
    }
    None
}

#[inline]
fn indexed_magic(content: &[u8], entries: &[Magic], index: &[u32; 256]) -> Option<&'static str> {
    let mut candidates = index[usize::from(*content.first()?)];
    while candidates != 0 {
        let entry = candidates.trailing_zeros() as usize;
        candidates &= candidates - 1;
        if let Some(mime) = magic(content, &entries[entry..entry + 1]) {
            return Some(mime);
        }
    }
    None
}

fn truncate<'a>(content: &'a [u8], count: usize, enough: &mut bool) -> &'a [u8] {
    *enough &= content.len() >= count;
    &content[..content.len().min(count)]
}

fn unknown_type(hint: &[u8]) -> bool {
    matches!(hint, b"" | b"unknown/unknown" | b"application/unknown" | b"*/*")
        || !hint.contains(&b'/')
}

fn office(content: &[u8], path: &[u8], enough: &mut bool) -> Option<&'static str> {
    let version = magic(truncate(content, OFFICE_BYTES, enough), OFFICE)?;
    for (index, extension) in
        [b".doc".as_slice(), b".xls", b".ppt", b".docx", b".xlsx", b".pptx"].iter().enumerate()
    {
        if path.len() >= extension.len()
            && path[path.len() - extension.len()..].eq_ignore_ascii_case(extension)
        {
            return Some(OFFICE_TYPES[index % 3 + if version == "CFB" { 0 } else { 3 }]);
        }
    }
    None
}

fn xml(content: &[u8], enough: &mut bool, mime: &mut Option<&'static str>) -> bool {
    let content = truncate(content, 300, enough);
    let mut pos = 0;
    for _ in 0..5 {
        if pos >= content.len() {
            break;
        }
        let Some(offset) = content[pos..].iter().position(|&byte| byte == b'<') else {
            return false;
        };
        pos += offset;
        let current = &content[pos..];
        if current.get(..5).is_some_and(|p| p.eq_ignore_ascii_case(b"<?xml"))
            || current.get(..9).is_some_and(|p| p.eq_ignore_ascii_case(b"<!DOCTYPE"))
        {
            pos += 1;
            continue;
        }
        if current.get(..5).is_some_and(|p| p.eq_ignore_ascii_case(b"<feed")) {
            *mime = Some("application/atom+xml");
        } else if current.get(..4).is_some_and(|p| p.eq_ignore_ascii_case(b"<rss")) {
            *mime = Some("application/rss+xml");
        }
        return true;
    }
    pos < content.len()
}

#[inline]
pub fn binary_byte(byte: u8) -> bool {
    const BINARY: u32 = !((1 << b'\t') | (1 << b'\n') | (1 << b'\r') | (1 << 12) | (1 << 27));
    byte < 32 && BINARY & (1 << byte) != 0
}

pub fn looks_binary(content: &[u8]) -> bool {
    if content.first().is_some_and(|&byte| binary_byte(byte)) {
        return true;
    }
    // A fixed-size reduction permits SIMD for long text without reading beyond
    // the supplied slice. Keep short inputs/tails scalar, preserving fast
    // exits.
    let (blocks, tail) = content.as_chunks::<32>();
    for block in blocks {
        let mut found = false;
        for &byte in block {
            found |= (byte < 9)
                | (byte == 11)
                | (byte.wrapping_sub(14) < 13)
                | (byte.wrapping_sub(28) < 4);
        }
        if found {
            return true;
        }
    }
    tail.iter().any(|&byte| binary_byte(byte))
}

fn binary(content: &[u8], enough: &mut bool) -> (bool, &'static str) {
    let full = content.len() >= MAX_SNIFF;
    let content = &content[..content.len().min(MAX_SNIFF)];
    if content.starts_with(b"\xFE\xFF")
        || content.starts_with(b"\xFF\xFE")
        || content.starts_with(b"\xEF\xBB\xBF")
    {
        return (false, "text/plain");
    }
    if looks_binary(content) {
        return (true, "application/octet-stream");
    }
    *enough &= full;
    (false, "text/plain")
}

pub struct Result {
    pub mime: Option<&'static str>,
    pub complete: bool,
}

// The legacy C++ API permits the hint or content view to alias its output.
// Its early string assignments then affect subsequent input reads. Ordinary
// calls remain borrowed; this bounded, out-of-line path models only those
// defined alias cases without retaining pointers or allocating heap memory.
trait Content {
    fn bytes(&self) -> &[u8];
    fn assign(&mut self, output: &[u8]);
}
impl Content for &[u8] {
    fn bytes(&self) -> &[u8] {
        self
    }
    fn assign(&mut self, _: &[u8]) {}
}
struct AliasedContent<'a> {
    storage: &'a mut [u8],
    offset: usize,
}
impl Content for AliasedContent<'_> {
    fn bytes(&self) -> &[u8] {
        self.storage
    }
    fn assign(&mut self, output: &[u8]) {
        let count = output.len().saturating_sub(self.offset).min(self.storage.len());
        if count != 0 {
            self.storage[..count].copy_from_slice(&output[self.offset..self.offset + count]);
        }
        if let Some(terminator) = output.len().checked_sub(self.offset)
            && terminator < self.storage.len()
        {
            self.storage[terminator] = 0;
        }
    }
}

#[inline(never)]
fn sniff_aliased(
    content: &[u8],
    path: &[u8],
    hint: &[u8],
    file_url: bool,
    force_html: bool,
    hint_alias: bool,
    offset: usize,
) -> Result {
    let len = content.len().min(MAX_SNIFF);
    let parse = |storage: &mut [u8]| {
        let storage = &mut storage[..len];
        storage.copy_from_slice(&content[..len]);
        sniff_impl(AliasedContent { storage, offset }, path, hint, file_url, force_html, hint_alias)
    };
    // This selects scratch capacity only, without changing the parser's view
    // length or sniffing thresholds. Borrow storage instead of moving an array.
    if len <= 64 {
        parse(&mut [0; 64])
    } else {
        parse(&mut [0; MAX_SNIFF])
    }
}

pub fn sniff(
    content: &[u8],
    path: &[u8],
    hint: &[u8],
    file_url: bool,
    force_html: bool,
    hint_alias: bool,
    offset: usize,
) -> Result {
    if offset == usize::MAX {
        sniff_impl(content, path, hint, file_url, force_html, hint_alias)
    } else {
        sniff_aliased(content, path, hint, file_url, force_html, hint_alias, offset)
    }
}

fn sniff_impl(
    mut content: impl Content,
    path: &[u8],
    hint: &[u8],
    file_url: bool,
    force_html: bool,
    hint_alias: bool,
) -> Result {
    content.assign(hint);
    let mut alias_octet = false;
    let mut enough = true;
    let mut mime = None;
    // An Office hint permits validation only, never general content sniffing.
    if office_type(hint) {
        let clipped = truncate(content.bytes(), OFFICE_BYTES, &mut enough);
        if enough && magic(clipped, OFFICE).is_none() {
            mime = Some("application/octet-stream");
        }
        return Result { mime, complete: enough };
    }
    let unknown = unknown_type(hint);
    // Executable HTML/XML is sniffed only for an unknown hint. File URLs also
    // require the existing explicit opt-in.
    if unknown && (!file_url || force_html) {
        let clipped = truncate(content.bytes(), 512, &mut enough);
        let first = clipped
            .iter()
            .position(|byte| !matches!(byte, b' ' | b'\t' | b'\n' | b'\r' | 11 | 12))
            .unwrap_or(clipped.len());
        if let Some(html) = magic(&clipped[first..], HTML) {
            return Result { mime: Some(html), complete: true };
        }
    }
    let plain = hint == b"text/plain";
    if unknown || plain {
        let (is_binary, binary_mime) = binary(content.bytes(), &mut enough);
        mime = Some(binary_mime);
        content.assign(binary_mime.as_bytes());
        alias_octet = hint_alias && is_binary;
        if !is_binary && plain {
            return Result { mime, complete: enough };
        }
    }
    if hint == b"text/xml" || hint == b"application/xml" {
        // An XML hint permits feed recognition only, not image/media sniffing.
        let matched = xml(content.bytes(), &mut enough, &mut mime);
        return Result { mime, complete: matched || enough };
    }
    if path.ends_with(b".crx") {
        let clipped = truncate(content.bytes(), MAGIC_BYTES, &mut enough);
        if clipped.starts_with(b"Cr24\x02\0\0\0") || clipped.starts_with(b"Cr24\x03\0\0\0") {
            return Result { mime: Some("application/x-chrome-extension"), complete: true };
        }
    }
    // Office precedes generic ZIP, which shares OOXML's magic.
    if let Some(office) = office(content.bytes(), path, &mut enough) {
        return Result { mime: Some(office), complete: true };
    }
    // Octet-stream permits the CRX/Office exceptions above, but nothing else.
    if hint != b"application/octet-stream"
        && !alias_octet
        && let Some(found) =
            indexed_magic(truncate(content.bytes(), MAGIC_BYTES, &mut enough), MAGIC, &MAGIC_FIRST)
    {
        return Result { mime: Some(found), complete: true };
    }
    Result { mime, complete: enough }
}

#[inline]
pub fn sniff_local(content: &[u8]) -> Option<&'static str> {
    indexed_magic(content, EXTRA_MAGIC, &EXTRA_MAGIC_FIRST)
        .or_else(|| indexed_magic(content, MAGIC, &MAGIC_FIRST))
}

pub fn should_sniff(mime_type: &[u8], scheme: u8, first_options: &[u8]) -> bool {
    scheme != 0
        && !first_options.eq_ignore_ascii_case(b"nosniff")
        && (matches!(
            mime_type,
            b"text/plain" | b"application/octet-stream" | b"text/xml" | b"application/xml"
        ) || office_type(mime_type)
            || unknown_type(mime_type))
}
