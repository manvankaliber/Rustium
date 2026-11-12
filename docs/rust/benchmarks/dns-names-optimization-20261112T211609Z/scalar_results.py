from pathlib import Path
p=Path('net/dns/dns_names_ffi.rs');s=p.read_text();a=s.index('    struct NameResult {');b=s.index('    struct LabelChunk',a);s=s[:a]+s[b:];s=s.replace('-> NameResult','-> u64').replace('-> ReadResult','-> u32').replace('-> ffi::NameResult','-> u64').replace('-> ffi::ReadResult','-> u32');a=s.index('fn result(');b=s.index('\nfn dotted_name',a);s=s[:a]+'''// Scalar return metadata avoids an owned aggregate or hidden result pointer.
// The low word is length + 1 (zero means invalid); the high word is consumed.
// DNS parsing stops after at most 255 valid octets plus one <=192-byte read,
// so consumed always fits the high word. Output lengths are at most 255.
fn result(consumed: usize, len: Option<usize>) -> u64 {
    ((consumed as u64) << 32) | len.map_or(0, |n| n as u64 + 1)
}'''+s[b:];a=s.index('fn read_length(' , s.index('struct Reader'));b=s.index('\nstruct Canonical',a);s=s[:a]+'''fn read_length(input: &[u8], wide: bool) -> u32 {
    // Prefix width is 1 or 2; payload lengths fit 16 bits. Zero is failure.
    dns_names_core::length_prefixed(input, wide)
        .map_or(0, |(offset, len)| ((len as u32) << 2) | offset as u32)
}'''+s[b:];p.write_text(s)
p=Path('net/dns/dns_names_util.cc');s=p.read_text();mark='rust::Slice<const uint8_t> Bytes(std::string_view input) {';a=s.index(mark);b=s.index('\n}  // namespace',a);s=s[:b]+'''
struct NameResult {
  size_t consumed;
  size_t len;
  bool valid;
};
NameResult DecodeResult(uint64_t value) {
  const uint32_t length_plus_one = static_cast<uint32_t>(value);
  return {static_cast<size_t>(value >> 32),
          length_plus_one ? length_plus_one - 1u : 0u,
          length_plus_one != 0};
}'''+s[b:];s=s.replace('const auto result = dns_names_internal::dotted_name(\n      Bytes(input), require_valid_internet_hostname, output);', 'const auto result = DecodeResult(dns_names_internal::dotted_name(\n      Bytes(input), require_valid_internet_hostname, output));');s=s.replace('const auto result = dns_names_internal::network_name(\n      base::SpanToRustSlice(input), require_complete, output);','const auto result = DecodeResult(dns_names_internal::network_name(\n      base::SpanToRustSlice(input), require_complete, output));');s=s.replace('dns_names_internal::NameResult result;', 'NameResult result;');s=s.replace('result = dns_names_internal::network_name_reader(context, require_complete,\n                                                    output);', 'result = DecodeResult(dns_names_internal::network_name_reader(\n        context, require_complete, output));');s=s.replace('result = dns_names_internal::network_name(base::SpanToRustSlice(input),\n                                            require_complete, output);','result = DecodeResult(dns_names_internal::network_name(base::SpanToRustSlice(input),\n                                            require_complete, output));');s=s.replace('  if (!result.valid) {\n    return false;\n  }', '  if (!result) {\n    return false;\n  }\n  const size_t offset = result & 3u;\n  const size_t len = result >> 2;');s=s.replace('result.offset + result.len','offset + len').replace('input.subspan(result.offset, result.len)','input.subspan(offset, len)');p.write_text(s)
