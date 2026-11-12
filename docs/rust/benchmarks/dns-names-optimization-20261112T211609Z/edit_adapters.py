from pathlib import Path
p=Path('net/dns/dns_names_util.cc');s=p.read_text();a=s.index('CanonicalHostResult host_canonicalize(');b=s.index('\n}  // namespace net::dns_names_internal',a)
s=s[:a]+'''bool host_canonicalize(rust::Slice<const uint8_t> input, CanonicalName& context) {
  // URL/ICU writes directly into the eventual C++ result. Rust owns the
  // decision to keep this result or request original-byte fallback.
  url::StdStringCanonOutput output(&*context.output);
  url::CanonHostInfo host_info;
  url::CanonicalizeHostVerbose(View(input), url::Component(0, input.size()),
                               &output, &host_info);
  const bool broken = host_info.family == url::CanonHostInfo::Family::BROKEN;
  if (!broken) { output.Complete(); }
  return broken;
}
void canonical_use_original(rust::Slice<const uint8_t> input,
                             CanonicalName& context) {
  context.output->assign(View(input));
}'''+s[b:]
a=s.index('  auto result = dns_names_internal::dotted_name(');b=s.index('\nnamespace {\nbool ReadLengthPrefixed',a)
s=s[:a]+'''  dns_names_internal::NameBuffer output{};
  const auto result = dns_names_internal::dotted_name(
      Bytes(input), require_valid_internet_hostname, output);
  if (!result.valid) { return std::nullopt; }
  auto bytes = base::span(output.bytes).first(result.len);
  return std::vector<uint8_t>(bytes.begin(), bytes.end());
}
std::optional<std::string> NetworkToDottedName(base::span<const uint8_t> input,
                                               bool require_complete) {
  dns_names_internal::NameBuffer output{};
  const auto result = dns_names_internal::network_name(
      base::SpanToRustSlice(input), require_complete, output);
  if (!result.valid) { return std::nullopt; }
  return std::string(base::as_string_view(base::span(output.bytes).first(result.len)));
}
std::optional<std::string> NetworkToDottedName(
    base::SpanReader<const uint8_t>& reader, bool require_complete) {
  dns_names_internal::NameBuffer output{};
  dns_names_internal::NameResult result;
  const auto input = reader.remaining_span();
  const auto start = reinterpret_cast<uintptr_t>(input.data());
  const auto cursor = reinterpret_cast<uintptr_t>(&reader);
  // Only an ownership overlap check lives here, never DNS parsing. Subtraction
  // avoids endpoint overflow. Rust never sees the private SpanReader layout.
  const bool aliases_cursor = !input.empty() &&
      (start <= cursor ? cursor - start < input.size() : start - cursor < sizeof(reader));
  if (aliases_cursor) {
    // Every input borrow ends before the cursor callback mutates its storage;
    // payload bytes are copied after that commit, matching the original.
    dns_names_internal::NameReader context{raw_ref(reader)};
    result = dns_names_internal::network_name_reader(context, require_complete, output);
  } else {
    result = dns_names_internal::network_name(base::SpanToRustSlice(input),
                                               require_complete, output);
    CHECK(reader.Skip(result.consumed));
  }
  if (!result.valid) { return std::nullopt; }
  return std::string(base::as_string_view(base::span(output.bytes).first(result.len)));
}'''+s[b:]
a=s.index('  auto result = dns_names_internal::canonical_name(');b=s.index('\n}  // namespace net::dns_names_util',a)
s=s[:a]+'''  std::string result;
  dns_names_internal::CanonicalName context{raw_ref(result)};
  dns_names_internal::canonical_name(Bytes(input), context);
  return result;
}
bool DomainNameLess(std::string_view lhs, std::string_view rhs) {
  return dns_names_internal::net_dns_names_less(
      base::as_byte_span(lhs).data(), lhs.size(),
      base::as_byte_span(rhs).data(), rhs.size());
}'''+s[b:];p.write_text(s)
