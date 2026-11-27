// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_HTTP_CONTENT_DISPOSITION_HTTP_UTIL_REFERENCE_H_
#define NET_HTTP_CONTENT_DISPOSITION_HTTP_UTIL_REFERENCE_H_
#include <string>
#include <string_view>

#include "net/http/content_disposition_tokenizer_reference.h"
namespace net::content_disposition_reference {
// Frozen helper API/layout from HttpUtil. Its implementation stays in a
// separate translation unit, preserving the original non-LTO call boundary.
class LegacyHttpUtil {
 public:
  ALWAYS_INLINE static bool IsLWS(char c) { return c == ' ' || c == '\t'; }
  static std::string_view TrimLWS(std::string_view string);
  static void TrimLWS(std::string_view string, size_t& begin, size_t& end);
  static bool IsTokenChar(char c);
  static bool IsToken(std::string_view string);
  static std::string Unquote(std::string_view str);
  static bool StrictUnquote(std::string_view str, std::string* out);
  class ValuesIterator {
   public:
    ValuesIterator(std::string_view values,
                   char delimiter,
                   bool ignore_empty_values = true);

    ValuesIterator(const ValuesIterator& other);
    ~ValuesIterator();

    // Advances the iterator to the next value, if any.  Returns true if there
    // is a next value.  Use value* methods to access the resultant value.
    bool GetNext();

    std::string_view value() const {
      return values_.substr(value_begin_, value_end_ - value_begin_);
    }

    // The begin/end offsets of the current value, relative to the start of
    // `values`.
    size_t value_begin() const { return value_begin_; }
    size_t value_end() const { return value_end_; }

   private:
    // The original input value.
    std::string_view values_;

    bool ignore_empty_values_;

    StringViewTokenizer tokenizer_;

    // These internally track the range of the current value withint `values_`,
    // to can provide begin/end indices for the current value for
    // HttpResponseHeaders, the only consumer that needs them.
    size_t value_begin_ = 0u;
    size_t value_end_ = 0u;
  };

  // Iterates over a delimited sequence of name-value pairs in an HTTP header.
  // Each pair consists of a token (the name), an equals sign, and either a
  // token or quoted-string (the value). Arbitrary HTTP LWS is permitted outside
  // of and between names, values, and delimiters.
  //
  // String iterators returned from this class' methods may be invalidated upon
  // calls to GetNext() or after the NameValuePairsIterator is destroyed.
  class NameValuePairsIterator {
   public:
    // Whether or not values are optional. Values::NOT_REQUIRED allows
    // e.g. name1=value1;name2;name3=value3, whereas Vaues::REQUIRED
    // will treat it as a parse error because name2 does not have a
    // corresponding equals sign.
    enum class Values { NOT_REQUIRED, REQUIRED };

    // Whether or not unmatched quotes should be considered a failure. By
    // default this class is pretty lenient and does a best effort to parse
    // values with mismatched quotes. When set to STRICT_QUOTES a value with
    // mismatched or otherwise invalid quotes is considered a parse error.
    enum class Quotes { STRICT_QUOTES, NOT_STRICT };

    NameValuePairsIterator(std::string_view value,
                           char delimiter,
                           Values optional_values = Values::REQUIRED,
                           Quotes strict_quotes = Quotes::NOT_STRICT);

    NameValuePairsIterator(const NameValuePairsIterator& other);

    ~NameValuePairsIterator();

    // Advances the iterator to the next pair, if any.  Returns true if there
    // is a next pair. Returns false on completion or on error. In the error
    // case, `valid()` will return false. Once GetNext() returns false, whether
    // due to error or completion, it should not be called again. Use name() and
    // value() methods to access the resultant value.
    //
    // Each call will invalidate the string views obtained through the previous
    // GetNext() call, as they may point to temporary buffers.
    bool GetNext();

    // Returns false if there was a parse error.
    bool valid() const { return valid_; }

    // The name of the current name-value pair.
    std::string_view name() const { return name_; }

    // The value of the current name-value pair. Note that the returned
    // string_view will be invalidated by the next GetNext() call.
    std::string_view value() const LIFETIME_BOUND {
      return value_is_quoted_ ? unquoted_value_ : value_;
    }

    bool value_is_quoted() const { return value_is_quoted_; }

    // The value before unquoting (if any).
    std::string_view raw_value() const LIFETIME_BOUND { return value_; }

   private:
    // Attempts to parse `name_value_pair`, populating `name_`, `value_`, and
    // `unquoted_value_`. returns false on failure. On failure, the caller
    // should clear those values, to ensure consistent behavior.
    bool ParseNameValuePair(std::string_view name_value_pair);

    LegacyHttpUtil::ValuesIterator props_;
    bool valid_ = true;

    std::string_view name_;
    std::string_view value_;

    // Do not store iterators into this string. The NameValuePairsIterator
    // is copyable/assignable, and if copied the copy's iterators would point
    // into the original's unquoted_value_ member.
    std::string unquoted_value_;

    bool value_is_quoted_ = false;

    // True if values are required for each name/value pair; false if a
    // name is permitted to appear without a corresponding value.
    bool values_optional_;

    // True if quotes values are required to be properly quoted; false if
    // mismatched quotes and other problems with quoted values should be more
    // or less gracefully treated as valid.
    bool strict_quotes_;
  };
};
}  // namespace net::content_disposition_reference
#endif  // NET_HTTP_CONTENT_DISPOSITION_HTTP_UTIL_REFERENCE_H_
