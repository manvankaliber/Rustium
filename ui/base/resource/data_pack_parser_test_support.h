// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_RESOURCE_DATA_PACK_PARSER_TEST_SUPPORT_H_
#define UI_BASE_RESOURCE_DATA_PACK_PARSER_TEST_SUPPORT_H_

#include <stddef.h>
#include <stdint.h>

#include <cstdlib>
#include <cstring>
#include <span>
#include <vector>

#include "base/compiler_specific.h"
#include "third_party/rust/cxx/v1/cxx.h"
#include "ui/base/resource/data_pack_parser_ffi.rs.h"

namespace ui::data_pack_internal::test {

// Test/benchmark-only reference for the validator before the Rust migration.
// Keep its separate bounds/ordering/alias passes and error precedence frozen.
// memcpy avoids the original packed-struct aliasing; wide table-size arithmetic
// makes malformed v4 counts safe to compare on 32-bit platforms as well.
inline ParseResult ParseLegacy(std::span<const uint8_t> data) {
  ParseResult result{};
  auto fail = [&result](ParseStatus status) {
    result = {};
    result.status = status;
    return result;
  };
  uint32_t version = 0;
  if (data.size() > sizeof(version)) {
    // SAFETY: The input contains at least sizeof(version) bytes.
    UNSAFE_BUFFERS(std::memcpy(&version, data.data(), sizeof(version)));
  }
  const size_t header_length = version == 4 ? 9 : 12;
  if (version == 0 || data.size() < header_length) {
    return fail(ParseStatus::IncompleteHeader);
  }
  if (version == 4) {
    uint32_t count;
    // SAFETY: The v4 header length check guarantees bytes [4, 8) exist.
    UNSAFE_BUFFERS(std::memcpy(&count, data.subspan(4).data(), sizeof(count)));
    result.resource_count = count;
    result.encoding = data[8];
  } else if (version == 5) {
    uint16_t count;
    uint16_t aliases;
    // SAFETY: The v5 header length check guarantees bytes [8, 12) exist.
    UNSAFE_BUFFERS(std::memcpy(&count, data.subspan(8).data(), sizeof(count)));
    // SAFETY: The same checked header contains the two-byte alias count.
    UNSAFE_BUFFERS(
        std::memcpy(&aliases, data.subspan(10).data(), sizeof(aliases)));
    result.resource_count = count;
    result.alias_count = aliases;
    result.encoding = data[4];
  } else {
    return fail(ParseStatus::BadVersion);
  }
  if (result.encoding > 2) {
    return fail(ParseStatus::BadEncoding);
  }
  const uint64_t resource_table_size =
      (uint64_t{result.resource_count} + 1) * 6;
  const uint64_t alias_table_offset = header_length + resource_table_size;
  if (alias_table_offset + uint64_t{result.alias_count} * 4 > data.size()) {
    return fail(ParseStatus::TooShort);
  }
  result.resource_table_offset = header_length;
  result.alias_table_offset = static_cast<size_t>(alias_table_offset);
  auto offset_at = [&](size_t index) {
    uint32_t offset;
    // SAFETY: The complete table extent was checked above. Callers pass an
    // index no greater than resource_count, including its sentinel entry.
    UNSAFE_BUFFERS(
        std::memcpy(&offset, data.subspan(header_length + index * 6 + 2).data(),
                    sizeof(offset)));
    return offset;
  };
  for (size_t i = 0; i < result.resource_count + 1; ++i) {
    if (offset_at(i) > data.size()) {
      return fail(ParseStatus::BoundsExceeded);
    }
  }
  for (size_t i = 0; i < result.resource_count; ++i) {
    if (offset_at(i) > offset_at(i + 1)) {
      return fail(ParseStatus::OrderingViolation);
    }
  }
  for (size_t i = 0; i < result.alias_count; ++i) {
    uint16_t index;
    // SAFETY: The complete alias table extent was checked above and i is less
    // than alias_count, so this two-byte field is within its four-byte entry.
    UNSAFE_BUFFERS(std::memcpy(
        &index, data.subspan(result.alias_table_offset + i * 4 + 2).data(),
        sizeof(index)));
    if (index >= result.resource_count) {
      return fail(ParseStatus::AliasTableCorrupt);
    }
  }
  result.status = ParseStatus::Ok;
  return result;
}

// Frozen libc bsearch lookup, with guards around cases where the old reader
// could access beyond the input after corruption. Use cached metadata, just as
// DataPack does; changes to the on-disk header must not change the layout.
inline LookupResult LookupLegacy(std::span<const uint8_t> data,
                                 const ParseResult& layout,
                                 uint16_t id,
                                 bool check_range = true) {
  LookupResult result{};
  result.status = LookupStatus::CorruptIndex;
  const size_t start = layout.resource_table_offset;
  if (start > data.size() || (data.size() - start) / 6 == 0 ||
      layout.resource_count > (data.size() - start) / 6 - 1) {
    return result;
  }
  const size_t alias_start = start + (layout.resource_count + 1) * 6;
  if (layout.alias_count > (data.size() - alias_start) / 4) {
    return result;
  }
  auto compare = +[](const void* key, const void* entry) {
    uint16_t resource_id;
    // SAFETY: bsearch only passes entries in the checked tables below. Both
    // table entry sizes (six and four bytes) contain this two-byte field.
    UNSAFE_BUFFERS(std::memcpy(&resource_id, entry, sizeof(resource_id)));
    return static_cast<int>(*static_cast<const uint16_t*>(key)) - resource_id;
  };
  const auto entries = data.subspan(start);
  // SAFETY: The guards above verify the resource table and sentinel fit in
  // data. The key is a live uint16_t and compare reads its two-byte IDs.
  const void* entry = UNSAFE_BUFFERS(
      std::bsearch(&id, entries.data(), layout.resource_count, 6, compare));
  size_t index;
  if (entry) {
    index = (static_cast<const uint8_t*>(entry) - entries.data()) / 6;
  } else {
    // SAFETY: The alias table's complete byte extent was checked above, with
    // four bytes per entry; the key and comparator match the resource search.
    const void* alias = UNSAFE_BUFFERS(std::bsearch(
        &id, data.subspan(alias_start).data(), layout.alias_count, 4, compare));
    if (!alias) {
      result.status = LookupStatus::NotFound;
      return result;
    }
    uint16_t alias_index;
    // SAFETY: A successful bsearch returns a complete four-byte alias entry.
    UNSAFE_BUFFERS(std::memcpy(
        &alias_index,
        std::span(static_cast<const uint8_t*>(alias), 4).subspan(2).data(),
        sizeof(alias_index)));
    index = alias_index;
    if (index >= layout.resource_count) {
      return result;
    }
  }
  result.status = LookupStatus::Found;
  if (!check_range) {
    return result;
  }
  uint32_t begin;
  uint32_t end;
  // SAFETY: index is a resource entry (checked for aliases); the complete
  // resource table, including the following sentinel, was checked above.
  UNSAFE_BUFFERS(std::memcpy(&begin, entries.subspan(index * 6 + 2).data(),
                             sizeof(begin)));
  // SAFETY: The same table check covers the four-byte offset of index + 1.
  UNSAFE_BUFFERS(std::memcpy(&end, entries.subspan((index + 1) * 6 + 2).data(),
                             sizeof(end)));
  if (end > data.size()) {
    result.status = LookupStatus::BoundsExceeded;
  } else if (begin > end) {
    result.status = LookupStatus::OrderingViolation;
  } else {
    result.offset = begin;
    result.length = end - begin;
  }
  return result;
}

inline void Write16(std::vector<uint8_t>& data, size_t at, uint16_t value) {
  data[at] = static_cast<uint8_t>(value);
  data[at + 1] = static_cast<uint8_t>(value >> 8);
}

inline void Write32(std::vector<uint8_t>& data, size_t at, uint32_t value) {
  Write16(data, at, static_cast<uint16_t>(value));
  Write16(data, at + 2, static_cast<uint16_t>(value >> 16));
}

// Synthetic pack with one payload byte per resource and aliases to entry zero.
inline std::vector<uint8_t> MakePack(uint32_t version,
                                     uint16_t resources,
                                     uint16_t aliases = 0,
                                     uint8_t encoding = 1) {
  const size_t header = version == 4 ? 9 : 12;
  const size_t payload = header + (size_t{resources} + 1) * 6 + aliases * 4;
  std::vector<uint8_t> data(payload + resources, 0);
  Write32(data, 0, version);
  if (version == 4) {
    Write32(data, 4, resources);
    data[8] = encoding;
  } else {
    data[4] = encoding;
    Write16(data, 8, resources);
    Write16(data, 10, aliases);
  }
  for (size_t i = 0; i <= resources; ++i) {
    Write16(data, header + i * 6, static_cast<uint16_t>(i + 1));
    Write32(data, header + i * 6 + 2, static_cast<uint32_t>(payload + i));
  }
  for (size_t i = 0; i < aliases; ++i) {
    Write16(data, header + (size_t{resources} + 1) * 6 + i * 4,
            static_cast<uint16_t>(resources + i + 1));
  }
  return data;
}

}  // namespace ui::data_pack_internal::test

#endif  // UI_BASE_RESOURCE_DATA_PACK_PARSER_TEST_SUPPORT_H_
