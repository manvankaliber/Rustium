// Copyright 2026 The Rustium Authors
// Candidate validation adapter; not a production export or tokenizer bridge.
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include "double-conversion/double-conversion.h"

namespace {
unsigned Hex(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  throw std::runtime_error("invalid hex digit");
}
std::string Bytes(const std::string& hex, bool utf16) {
  const size_t width = utf16 ? 4 : 2;
  if (hex.size() % width) throw std::runtime_error("invalid hex width");
  std::string result;
  for (size_t i = 0; i < hex.size(); i += width) {
    unsigned unit = 0;
    for (size_t j = 0; j < width; ++j) unit = (unit << 4) | Hex(hex[i+j]);
    // WTF ParseDouble(UChar) narrows non-ASCII code units to zero.
    result.push_back(static_cast<char>(utf16 && unit > 127 ? 0 : unit));
  }
  return result;
}
bool AsciiSpace(unsigned char c) {
  return c == ' ' || (c >= '\t' && c <= '\r');
}
uint64_t Bits(double value) {
  uint64_t bits;
  static_assert(sizeof(bits) == sizeof(value));
  std::memcpy(&bits, &value, sizeof(bits));
  return bits;
}
std::string HexBits(uint64_t bits) {
  std::ostringstream s;
  s << std::hex << std::setfill('0') << std::setw(16) << bits;
  return s.str();
}
void Parse(const std::string& hex, bool utf16) {
  std::string input = Bytes(hex, utf16);
  if (input.size() > static_cast<size_t>(std::numeric_limits<int>::max()))
    throw std::runtime_error("input exceeds converter int length");
  size_t spaces = 0;
  while (spaces < input.size() && AsciiSpace(static_cast<unsigned char>(input[spaces]))) ++spaces;
  // Pinned WTF dtoa.cc flags; outer ToDoubleType requires full consumption.
  const double_conversion::StringToDoubleConverter converter(
      double_conversion::StringToDoubleConverter::ALLOW_LEADING_SPACES |
      double_conversion::StringToDoubleConverter::ALLOW_TRAILING_JUNK,
      0.0, 0.0, nullptr, nullptr);
  int consumed = 0;
  double value = converter.StringToDouble(input.data() + spaces,
      static_cast<int>(input.size() - spaces), &consumed);
  bool valid = consumed != 0 && static_cast<size_t>(consumed) + spaces == input.size();
  // Expose raw provider consumption as well as outer complete-consumption status.
  std::cout << "{\"op\":\"parse\",\"bits\":\"" << HexBits(Bits(value))
            << "\",\"consumed\":" << (consumed ? consumed + spaces : 0)
            << ",\"provider_consumed\":" << consumed
            << ",\"valid\":" << (valid ? "true" : "false") << "}\n";
}
std::string Format(double value, bool precision) {
  char buffer[128];
  double_conversion::StringBuilder builder(buffer, sizeof(buffer));
  const auto& converter = double_conversion::DoubleToStringConverter::EcmaScriptConverter();
  bool ok = precision ? converter.ToPrecision(value, 6, &builder)
                      : converter.ToShortest(value, &builder);
  if (!ok) throw std::runtime_error("formatter failed");
  const int size = builder.position();
  builder.Finalize();
  std::string result(buffer, size);
  // Pinned WTF FormatStringTruncatingTrailingZerosIfNeeded.
  if (precision && result.find('e') == std::string::npos) {
    const size_t dot = result.find('.');
    if (dot != std::string::npos) {
      size_t end = result.size();
      while (end > dot + 1 && result[end - 1] == '0') --end;
      if (end == dot + 1) --end;
      result.resize(end);
    }
  }
  return result;
}
void Formats(const std::string& hex) {
  if (hex.size() != 16) throw std::runtime_error("expected64bits");
  uint64_t bits = 0;
  for (char c : hex) bits = (bits << 4) | Hex(c);
  double value;
  std::memcpy(&value, &bits, sizeof(value));
  std::cout << "{\"op\":\"format\",\"bits\":\"" << HexBits(bits)
            << "\",\"shortest\":\"" << Format(value, false)
            << "\",\"six_significant\":\"" << Format(value, true) << "\"}\n";
}
}  // namespace
int main() {
  std::string line;
  try {
    while (std::getline(std::cin, line)) {
      if (!line.empty() && line.back() == '\r') line.pop_back();
      size_t tab = line.find('\t');
      if (tab == std::string::npos) throw std::runtime_error("expected op TAB hex");
      std::string op = line.substr(0, tab), value = line.substr(tab + 1);
      if (op == "P8") Parse(value, false);
      else if (op == "P16") Parse(value, true);
      else if (op == "F") Formats(value);
      else throw std::runtime_error("unknown operation");
    }
  } catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
  if (!std::cin.eof() || std::cin.bad()) {
    std::cerr << "input stream failed\n";
    return 1;
  }
  std::cout.flush();
  if (!std::cout) {
    std::cerr << "output stream failed\n";
    return 1;
  }
  return 0;
}
