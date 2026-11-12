// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

//! Allocation-free validation and lookup of v4/v5 DataPack indexes. Payload
//! bytes are opaque; returned metadata and ranges never borrow the input.

#![forbid(unsafe_code)]

const HEADER_V4: usize = 9;
const HEADER_V5: usize = 12;
const ENTRY_SIZE: usize = 6;
const ALIAS_SIZE: usize = 4;

#[derive(Debug, PartialEq, Eq)]
pub enum ParseError {
    IncompleteHeader,
    BadVersion,
    BadEncoding,
    TooShort,
    BoundsExceeded,
    OrderingViolation,
    AliasTableCorrupt,
}

#[derive(Debug, PartialEq, Eq)]
pub struct PackIndex {
    pub resource_count: usize,
    pub alias_count: usize,
    pub resource_table_offset: usize,
    pub alias_table_offset: usize,
    pub encoding: u8,
}

#[derive(Debug, PartialEq, Eq)]
pub enum LookupError {
    CorruptIndex,
    BoundsExceeded,
    OrderingViolation,
}

#[derive(Debug, PartialEq, Eq)]
pub struct ResourceRange {
    pub offset: u32,
    pub length: u32,
}

struct Tables<'a> {
    entries: &'a [[u8; ENTRY_SIZE]],
    aliases: &'a [[u8; ALIAS_SIZE]],
}

impl<'a> Tables<'a> {
    // Reuse the cached layout rather than reparsing a possibly modified header.
    // Check its extents in constant time; every subsequent array read is safe,
    // even for bad metadata passed through the bridge or a truncated buffer.
    fn new(data: &'a [u8], count: usize, aliases: usize, start: usize) -> Option<Self> {
        let entries_end = count.checked_add(1)?.checked_mul(ENTRY_SIZE)?.checked_add(start)?;
        let aliases_end = aliases.checked_mul(ALIAS_SIZE)?.checked_add(entries_end)?;
        Some(Self {
            entries: data.get(start..entries_end)?.as_chunks::<ENTRY_SIZE>().0,
            aliases: data.get(entries_end..aliases_end)?.as_chunks::<ALIAS_SIZE>().0,
        })
    }

    // Keep the search inside its caller so the successful index bounds are
    // visible when lookup reads the resource's start and end offsets.
    #[inline]
    fn find(&self, id: u16) -> Result<Option<usize>, LookupError> {
        // The final entry is a sentinel, not a searchable resource. Search real
        // entries before aliases, including when the same ID occurs in both.
        let count = self.entries.len() - 1;
        if let Some(index) = find_id(&self.entries[..count], id) {
            return Ok(Some(index));
        }
        let Some(alias) = find_id(self.aliases, id) else {
            return Ok(None);
        };
        let index = u16::from_le_bytes(self.aliases[alias][2..4].try_into().unwrap()) as usize;
        if index >= count {
            return Err(LookupError::CorruptIndex);
        }
        Ok(Some(index))
    }
}

fn find_id<const WIDTH: usize>(table: &[[u8; WIDTH]], id: u16) -> Option<usize> {
    let mut first = 0;
    let mut count = table.len();
    while count != 0 {
        // Match the platform's previous libc bsearch probe order. In
        // particular, Windows UCRT uses the lower middle for even counts; other
        // supported platforms use the upper middle. This preserves duplicate-ID
        // behavior.
        let half = if cfg!(target_os = "windows") { (count - 1) / 2 } else { count / 2 };
        let middle = first + half;
        let candidate = u16::from_le_bytes([table[middle][0], table[middle][1]]);
        match id.cmp(&candidate) {
            std::cmp::Ordering::Equal => return Some(middle),
            std::cmp::Ordering::Less => count = half,
            std::cmp::Ordering::Greater => {
                first = middle + 1;
                count -= half + 1;
            }
        }
    }
    None
}

pub fn has_resource(data: &[u8], count: usize, aliases: usize, start: usize, id: u16) -> bool {
    // Existence deliberately does not inspect payload offsets. A resource may
    // still exist when its range has become corrupt since the pack was loaded.
    Tables::new(data, count, aliases, start)
        .is_some_and(|tables| matches!(tables.find(id), Ok(Some(_))))
}

pub fn lookup(
    data: &[u8],
    count: usize,
    aliases: usize,
    start: usize,
    id: u16,
) -> Result<Option<ResourceRange>, LookupError> {
    let tables = Tables::new(data, count, aliases, start).ok_or(LookupError::CorruptIndex)?;
    let Some(index) = tables.find(id)? else {
        return Ok(None);
    };
    let offset = u32::from_le_bytes(tables.entries[index][2..6].try_into().unwrap());
    let end = u32::from_le_bytes(tables.entries[index + 1][2..6].try_into().unwrap());
    // Preserve the previous range-error precedence. Do not rely on validation
    // at load time: these bytes can change after a memory-mapped pack is
    // loaded.
    if end as usize > data.len() {
        return Err(LookupError::BoundsExceeded);
    }
    if offset > end {
        return Err(LookupError::OrderingViolation);
    }
    Ok(Some(ResourceRange { offset, length: end - offset }))
}

pub fn parse(data: &[u8]) -> Result<PackIndex, ParseError> {
    let version_bytes = data.get(..4).ok_or(ParseError::IncompleteHeader)?;
    let version = u32::from_le_bytes(version_bytes.try_into().unwrap());
    let header_length = if version == 4 { HEADER_V4 } else { HEADER_V5 };
    // Preserve the existing precedence: truncated/zero-version headers are
    // incomplete, including truncated headers with an unknown version.
    if version == 0 || data.len() < header_length {
        return Err(ParseError::IncompleteHeader);
    }

    let (resource_count, alias_count, encoding) = match version {
        4 => (u32::from_le_bytes(data[4..8].try_into().unwrap()) as usize, 0, data[8]),
        5 => (
            u16::from_le_bytes(data[8..10].try_into().unwrap()) as usize,
            u16::from_le_bytes(data[10..12].try_into().unwrap()) as usize,
            data[4],
        ),
        _ => return Err(ParseError::BadVersion),
    };
    if encoding > 2 {
        return Err(ParseError::BadEncoding);
    }

    // Include the sentinel entry, whose offset gives the last resource's end.
    // Checked arithmetic also handles malicious v4 counts on 32-bit targets.
    let resource_table_size = resource_count
        .checked_add(1)
        .and_then(|count| count.checked_mul(ENTRY_SIZE))
        .ok_or(ParseError::TooShort)?;
    let alias_table_offset =
        header_length.checked_add(resource_table_size).ok_or(ParseError::TooShort)?;
    let tables_end = alias_count
        .checked_mul(ALIAS_SIZE)
        .and_then(|size| alias_table_offset.checked_add(size))
        .ok_or(ParseError::TooShort)?;
    if tables_end > data.len() {
        return Err(ParseError::TooShort);
    }

    let entries = &data[header_length..alias_table_offset];
    // Remember ordering errors until all bounds have been checked: a later
    // out-of-bounds offset must take precedence. Combining the two checks
    // avoids rereading the table while preserving the legacy error precedence.
    // Reducing the maximum offset permits vectorization without per-entry
    // exits.
    let mut previous_offset = 0;
    let mut maximum_offset = 0;
    let mut misordered = false;
    for entry in entries.as_chunks::<ENTRY_SIZE>().0 {
        let offset = u32::from_le_bytes(entry[2..6].try_into().unwrap());
        maximum_offset = maximum_offset.max(offset);
        misordered |= offset < previous_offset;
        previous_offset = offset;
    }
    if maximum_offset as usize > data.len() {
        return Err(ParseError::BoundsExceeded);
    }
    if misordered {
        return Err(ParseError::OrderingViolation);
    }
    for alias in data[alias_table_offset..tables_end].as_chunks::<ALIAS_SIZE>().0 {
        let index = u16::from_le_bytes(alias[2..4].try_into().unwrap()) as usize;
        if index >= resource_count {
            return Err(ParseError::AliasTableCorrupt);
        }
    }

    // Resource-ID sorting and payload/header overlap were not checked by the
    // old validator. Tightening the format is a separate compatibility change.
    Ok(PackIndex {
        resource_count,
        alias_count,
        resource_table_offset: header_length,
        alias_table_offset,
        encoding,
    })
}
