// Frozen bodies from original/ snapshot. Namespace/symbol changes only.
#include "net/http/content_disposition_services_reference.h"

#include <algorithm>

#include "base/check_op.h"
#include "base/i18n/icu_string_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/string_view_util.h"
#include "net/http/content_disposition_modp_reference.h"
#include "third_party/icu/source/common/unicode/ucnv.h"
namespace net::content_disposition_reference {
using namespace base;
ModpDecodePolicy GetModpPolicy(Base64DecodePolicy policy) {
  switch (policy) {
    case Base64DecodePolicy::kStrict:
      return ModpDecodePolicy::kStrict;
    case Base64DecodePolicy::kForgiving:
      return ModpDecodePolicy::kForgiving;
  }
}
bool Base64Decode(std::string_view input,
                  std::string* output,
                  Base64DecodePolicy policy) {
  std::string decode_buf;
  decode_buf.resize(content_disposition_modp_b64_decode_len(input.size()));

  // Does not NUL-terminate result since result is binary data!
  size_t written_size = content_disposition_modp_b64_decode(
      decode_buf.data(), input.data(), input.size(), GetModpPolicy(policy));

  // Forgiving mode requires whitespace to be stripped prior to decoding.
  // We don't do that in the above code to ensure that the "happy path" of
  // input without whitespace is as fast as possible. Since whitespace in input
  // will always cause `content_disposition_modp_b64_decode` to fail, just
  // handle whitespace stripping on failure. This is not much slower than just
  // scanning for whitespace first, even for input with whitespace.
  if (written_size == MODP_B64_ERROR &&
      policy == Base64DecodePolicy::kForgiving) {
    // We could use `output` here to avoid an allocation when decoding is done
    // in-place, but it violates the API contract that `output` is only modified
    // on success.
    std::string input_without_whitespace;
    RemoveChars(input, base::as_string_view(kInfraAsciiWhitespace),
                &input_without_whitespace);
    // This means that the required size to decode is at most what was needed
    // above, which means `decode_buf` will fit the decoded bytes at its current
    // size and we don't need to call
    // `content_disposition_modp_b64_decode_len()` again.
    CHECK_LE(input_without_whitespace.size(), input.size());
    written_size = content_disposition_modp_b64_decode(
        decode_buf.data(), input_without_whitespace.data(),
        input_without_whitespace.size(), GetModpPolicy(policy));
  }

  if (written_size == MODP_B64_ERROR) {
    return false;
  }

  // If this failed it would indicate we wrote OOB. It's possible for this to be
  // elided by the compiler, since writing OOB is UB.
  CHECK_LE(written_size, decode_buf.size());

  // Shrinks the buffer and makes it NUL-terminated.
  decode_buf.resize(written_size);
  *output = std::move(decode_buf);
  return true;
}
bool UnescapeUnsignedByteAtIndex(std::string_view escaped_text,
                                 size_t index,
                                 unsigned char* value) {
  if ((index + 2) >= escaped_text.size()) {
    return false;
  }
  if (escaped_text[index] != '%') {
    return false;
  }
  char most_sig_digit(escaped_text[index + 1]);
  char least_sig_digit(escaped_text[index + 2]);
  if (IsHexDigit(most_sig_digit) && IsHexDigit(least_sig_digit)) {
    *value = static_cast<unsigned char>(HexDigitToInt(most_sig_digit) * 16 +
                                        HexDigitToInt(least_sig_digit));
    return true;
  }
  return false;
}
std::string UnescapeBinaryURLComponent(std::string_view escaped_text,
                                       UnescapeRule::Type rules) {
  // Only NORMAL and REPLACE_PLUS_WITH_SPACE are supported.
  DCHECK(rules != UnescapeRule::NONE);
  DCHECK(!(rules &
           ~(UnescapeRule::NORMAL | UnescapeRule::REPLACE_PLUS_WITH_SPACE)));

  // If there are no '%' characters in the string, there will be nothing to
  // unescape, so we can take the fast path.
  if (escaped_text.find('%') == std::string_view::npos) {
    std::string unescaped_text(escaped_text);
    if (rules & UnescapeRule::REPLACE_PLUS_WITH_SPACE) {
      std::ranges::replace(unescaped_text, '+', ' ');
    }
    return unescaped_text;
  }

  std::string unescaped_text;

  // The output of the unescaping is always smaller than the input, so we can
  // reserve the input size to make sure we have enough buffer and don't have
  // to allocate in the loop below.
  // Increase capacity before size, as just resizing can grow capacity
  // needlessly beyond our requested size.
  unescaped_text.reserve(escaped_text.size());
  unescaped_text.resize(escaped_text.size());

  size_t output_index = 0;

  for (size_t i = 0, max = escaped_text.size(); i < max;) {
    unsigned char byte;
    // UnescapeUnsignedByteAtIndex does bounds checking, so this is always safe
    // to call.
    if (UnescapeUnsignedByteAtIndex(escaped_text, i, &byte)) {
      unescaped_text[output_index++] = static_cast<char>(byte);
      i += 3;
      continue;
    }

    if ((rules & UnescapeRule::REPLACE_PLUS_WITH_SPACE) &&
        escaped_text[i] == '+') {
      unescaped_text[output_index++] = ' ';
      ++i;
      continue;
    }

    unescaped_text[output_index++] = escaped_text[i++];
  }

  DCHECK_LE(output_index, unescaped_text.size());
  unescaped_text.resize(output_index);
  return unescaped_text;
}
bool ConvertToUtf8(std::string_view text,
                   const char* charset,
                   std::string* output) {
  output->clear();

  UErrorCode err = U_ZERO_ERROR;
  UConverter* converter(ucnv_open(charset, &err));
  if (U_FAILURE(err)) {
    return false;
  }

  // A single byte in a legacy encoding can be expanded to 3 bytes in UTF-8.
  // A 'two-byte character' in a legacy encoding can be expanded to 4 bytes
  // in UTF-8. Therefore, the expansion ratio is 3 at most.
  output->resize(text.length() * 3);
  size_t output_length =
      ucnv_toAlgorithmic(UCNV_UTF8, converter, output->data(), output->length(),
                         text.data(), text.length(), &err);
  ucnv_close(converter);
  if (U_FAILURE(err)) {
    output->clear();
    return false;
  }

  output->resize(output_length);
  return true;
}
bool ConvertToUtf8AndNormalize(std::string_view text,
                               const char* charset,
                               std::string* output) {
  return base::ConvertToUtf8AndNormalize(text, charset, output);
}
bool ConvertToUTF16(std::string_view text,
                    const char* charset,
                    std::u16string* output) {
  return base::CodepageToUTF16(text, charset,
                               base::OnStringConversionError::FAIL, output);
}
}  // namespace net::content_disposition_reference
