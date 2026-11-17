# Director integration proposal

This proposal changes no shared surface. The candidate currently exposes Rust slice APIs from `src/lib.rs` only.

Director-owned decisions required before integration:

1. Select the Rust crate/export location and GN target, and add only the needed `//base:base_unittests` or dedicated Rust test wiring.
2. Define the C++/Rust bridge for byte and UTF-16 borrowed slices, owned outputs, enums, and key/value partial results. Borrowed output lifetime must remain bounded by the input; no raw pointers or `unsafe` are needed inside this policy crate.
3. Decide how Chromium `std::string_view`/`std::u16string_view` map to Rust slices and how Windows `std::wstring` (`wchar_t`) overloads remain provided. The candidate does not claim Windows ABI compatibility.
4. Keep existing C++ implementations and callers authoritative until the bridge is reviewed and configured behavior tests pass. Retire C++ only after all callers, exported symbols, Windows overloads, and provider debt are accounted for.

Required integration validation: build the selected configured target; run the original full `string_split_unittest.cc` cases and Rust candidate tests; test supported Windows and non-Windows surfaces as applicable; review borrowed lifetime and exact empty/NUL/surrogate behavior. No build, test, benchmark, or cutover evidence is supplied by this candidate.
