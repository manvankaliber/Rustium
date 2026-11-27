// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>
#include <stdint.h>

#include <cstdlib>
#include <span>

#include "ui/base/resource/data_pack_parser_test_support.h"

namespace {

void CheckLookup(std::span<const uint8_t> data,
                 const ui::data_pack_internal::ParseResult& layout,
                 uint16_t id) {
  using namespace ui::data_pack_internal;
  const auto actual = lookup_data_pack_resource(
      {data.data(), data.size()}, layout.resource_count, layout.alias_count,
      layout.resource_table_offset, id);
  const auto expected = test::LookupLegacy(data, layout, id);
  const bool has = has_data_pack_resource(
      {data.data(), data.size()}, layout.resource_count, layout.alias_count,
      layout.resource_table_offset, id);
  if (actual.status != expected.status || actual.offset != expected.offset ||
      actual.length != expected.length ||
      has != (test::LookupLegacy(data, layout, id, false).status ==
              LookupStatus::Found)) {
    std::abort();
  }
}

}  // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  using namespace ui::data_pack_internal;
  const auto actual = parse_data_pack({data, size});
  const auto expected = test::ParseLegacy(std::span(data, size));
  if (actual.status != expected.status ||
      actual.resource_count != expected.resource_count ||
      actual.alias_count != expected.alias_count ||
      actual.resource_table_offset != expected.resource_table_offset ||
      actual.alias_table_offset != expected.alias_table_offset ||
      actual.encoding != expected.encoding) {
    std::abort();
  }
  const uint16_t id =
      size >= 2 ? static_cast<uint16_t>(data[0] | data[1] << 8) : 0;
  if (actual.status == ParseStatus::Ok) {
    for (uint16_t query : {uint16_t{0}, uint16_t{1}, id, uint16_t{65535}}) {
      CheckLookup(std::span(data, size), actual, query);
    }
  }

  // Model a pack that was valid at load time, then had arbitrary bytes changed
  // without replacing its cached layout. Cap mutations so giant inputs cannot
  // force unbounded work in this part of the harness.
  auto changed = test::MakePack(5, 16, 4);
  const auto layout = test::ParseLegacy(changed);
  for (size_t i = 0; i + 1 < size && i < 64; i += 2) {
    changed[data[i] % changed.size()] = data[i + 1];
  }
  for (uint16_t query : {uint16_t{0}, uint16_t{1}, uint16_t{16}, uint16_t{17},
                         uint16_t{20}, id, uint16_t{65535}}) {
    CheckLookup(changed, layout, query);
  }
  return 0;
}
