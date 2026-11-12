from pathlib import Path
p=Path('net/http/auth_challenge_reference_tokenizer.h');s=Path('.scratch/net-wave1/auth-challenges/original/base/strings/string_tokenizer.h').read_text();s=s.replace('BASE_STRINGS_STRING_TOKENIZER_H_', 'NET_HTTP_AUTH_CHALLENGE_REFERENCE_TOKENIZER_H_').replace('#include "base/strings/string_util.h"','').replace('namespace base {','namespace net::auth_challenge_reference::frozen {\n\ntemplate <typename C> constexpr bool IsAsciiWhitespace(C c) {\n  return c == \' \' || c == \'\\t\' || c == \'\\n\' || c == \'\\r\' || c == \'\\f\' || c == \'\\v\';\n}',1).replace('}  // namespace base','}  // namespace net::auth_challenge_reference::frozen');p.write_text(s)
p=Path('net/http/auth_challenge_reference.cc');s=p.read_text().replace('#include "net/http/auth_challenge_reference_modp.h"','#include "net/http/auth_challenge_reference_modp.h"\n#include "net/http/auth_challenge_reference_tokenizer.h"')
start=s.index('Tokenizer::Tokenizer(');end=s.index('std::string_view Tokenizer::base64_param()',start)
s=s[:start]+'''// Frozen copies of StringViewTokenizer's original algorithm and the original
// ToLowerASCIIImpl/TrimLWS helpers (see archived source snapshots).
[[clang::noinline]] std::string FrozenLower(std::string_view str) {
  std::string ret;
  ret.reserve(str.size());
  for (size_t i = 0; i < str.size(); i++)
    ret.push_back((str[i] >= 'A' && str[i] <= 'Z') ? (str[i] + ('a' - 'A')) : str[i]);
  return ret;
}
[[clang::noinline]] std::string_view FrozenTrim(std::string_view string) {
  size_t begin_offset = 0;
  size_t end_offset = string.size();
  while (begin_offset < end_offset && IsLws(string[begin_offset])) ++begin_offset;
  while (begin_offset < end_offset && IsLws(string[end_offset - 1])) --end_offset;
  return string.substr(begin_offset, end_offset - begin_offset);
}
Tokenizer::Tokenizer(std::string_view challenge) : challenge_(challenge) {
  frozen::StringViewTokenizer tok(challenge, " \\t");
  if (!tok.GetNext()) return;
  scheme_ = FrozenLower(tok.token_piece());
  params_ = FrozenTrim(std::string_view(tok.token_end(), challenge.end()));
}
''' +s[end:];p.write_text(s)
