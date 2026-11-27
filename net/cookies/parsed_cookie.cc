// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Portions of this code based on Mozilla:
//   (netwerk/cookie/src/nsCookieService.cpp)
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 2003
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Daniel Witte (dwitte@stanford.edu)
 *   Michiel van Leeuwen (mvl@exedo.nl)
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include "net/cookies/parsed_cookie.h"

#include <array>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

#include "base/check.h"
#include "net/cookies/cookie_inclusion_status.h"
#include "net/cookies/parsed_cookie_rust_ffi.h"

namespace net {
namespace {
constexpr std::array<std::string_view, 9> kAttributeNames = {
    "path",     "domain",   "expires",  "max-age",    "secure",
    "httponly", "samesite", "priority", "partitioned"};

void ApplyReason(size_t reason, CookieInclusionStatus* status) {
  if (!status) {
    return;
  }
  using Reason = CookieInclusionStatus::ExclusionReason;
  switch (reason) {
    case 0:
      return;
    case 1:
      status->AddExclusionReason(Reason::EXCLUDE_NO_COOKIE_CONTENT);
      return;
    case 2:
      status->AddExclusionReason(
          Reason::EXCLUDE_NAME_VALUE_PAIR_EXCEEDS_MAX_SIZE);
      return;
    case 3:
      status->AddExclusionReason(Reason::EXCLUDE_DISALLOWED_CHARACTER);
      return;
  }
}

// Pointer formation only: Rust guarantees offsets inside the borrowed input.
// Avoid even zero arithmetic on a null pointer for null-empty string_views.
std::string_view::iterator At(std::string_view input, size_t offset) {
  return offset == 0 ? input.begin() : UNSAFE_BUFFERS(input.begin() + offset);
}
std::string_view Remaining(std::string_view::iterator it,
                           std::string_view::iterator end) {
  return it == end ? std::string_view(std::to_address(it), 0)
                   : std::string_view(it, end);
}

// Only iterator-object identity crosses the bridge, never iterator addresses.
size_t IteratorAliases(std::string_view::iterator* it,
                       const std::string_view::iterator& end,
                       std::string_view::iterator* start,
                       std::string_view::iterator* finish) {
  return size_t(it == start) | (size_t(it == finish) << 1) |
         (size_t(start == finish) << 2) | (size_t(&end == it) << 3) |
         (size_t(&end == start) << 4) | (size_t(&end == finish) << 5);
}

// Canonical enums stay in C++. Fail compilation if their persisted values drift
// from the private Rust bridge representation.
static_assert(static_cast<int>(CookieSameSite::UNSPECIFIED) == -1);
static_assert(static_cast<int>(CookieSameSite::NO_RESTRICTION) == 0);
static_assert(static_cast<int>(CookieSameSite::LAX_MODE) == 1);
static_assert(static_cast<int>(CookieSameSite::STRICT_MODE) == 2);
static_assert(static_cast<int>(CookieSameSiteString::kUnspecified) == 0);
static_assert(static_cast<int>(CookieSameSiteString::kEmptyString) == 1);
static_assert(static_cast<int>(CookieSameSiteString::kUnrecognized) == 2);
static_assert(static_cast<int>(CookieSameSiteString::kLax) == 3);
static_assert(static_cast<int>(CookieSameSiteString::kStrict) == 4);
static_assert(static_cast<int>(CookieSameSiteString::kNone) == 5);
static_assert(static_cast<int>(CookieSameSiteString::kExtended) == 6);
static_assert(COOKIE_PRIORITY_LOW == 0 && COOKIE_PRIORITY_MEDIUM == 1 &&
              COOKIE_PRIORITY_HIGH == 2 && COOKIE_PRIORITY_DEFAULT == 1);
static_assert(static_cast<int>(NamelessCookieLineParseType::kBareToken) == 0);
static_assert(
    static_cast<int>(
        NamelessCookieLineParseType::kBareTokenMatchingAttributeName) == 1);
static_assert(
    static_cast<int>(NamelessCookieLineParseType::kEqualsPrecedingToken) == 2);
static_assert(static_cast<int>(
                  NamelessCookieLineParseType::kNamelessWithAmbiguousValue) ==
              3);
}  // namespace

ParsedCookie::ParsedCookie(std::string_view input,
                           CookieInclusionStatus* status_out) {
  CookieInclusionStatus blank;
  auto& status = status_out ? *status_out : blank;
  status = CookieInclusionStatus();
  size_t position = 0;
  bool first = true;
  for (;;) {
    const auto pair =
        rustium_cookie_next(input.data(), input.size(), position, first);
    if (first) {
      internal_htab_ = pair.internal_htab != 0;
      if (pair.nameless != 4) {
        nameless_parse_type_ =
            static_cast<NamelessCookieLineParseType>(pair.nameless);
      }
    }
    if (pair.action == 0) {
      break;
    }
    if (pair.action == 3) {
      ApplyReason(pair.reason, &status);
      pairs_.clear();
      // The original selects attributes only after the entire parse succeeds.
      for (size_t* index : AttributeIndexes()) {
        *index = 0;
      }
      break;
    }
    if (pair.action == 2) {
      status.AddWarningReason(CookieInclusionStatus::WarningReason::
                                  WARN_ATTRIBUTE_VALUE_EXCEEDS_MAX_SIZE);
    } else {
      pairs_.emplace_back(input.substr(pair.name_begin, pair.name_len),
                          input.substr(pair.value_begin, pair.value_len));
      if (!first) {
        auto& name = pairs_.back().first;
        rustium_cookie_lower(name.data(), name.size());
        if (pair.attribute) {
          *AttributeIndexes()[pair.attribute - 1] = pairs_.size() - 1;
        }
      }
    }
    position = pair.next;
    first = false;
  }
  if (!IsValid()) {
    CHECK(!status.IsInclude());
  }
}

ParsedCookie::~ParsedCookie() = default;
bool ParsedCookie::IsValid() const {
  return !pairs_.empty();
}

std::array<size_t*, 9> ParsedCookie::AttributeIndexes() {
  return {&path_index_,      &domain_index_,   &expires_index_,
          &maxage_index_,    &secure_index_,   &httponly_index_,
          &same_site_index_, &priority_index_, &partitioned_index_};
}

std::pair<CookieSameSite, CookieSameSiteString> ParsedCookie::SameSite() const {
  std::string_view value =
      same_site_index_ ? pairs_[same_site_index_].second : std::string_view();
  const auto result = rustium_cookie_same_site(value.data(), value.size(),
                                               same_site_index_ != 0);
  return {static_cast<CookieSameSite>(result.value),
          static_cast<CookieSameSiteString>(result.metric)};
}
CookiePriority ParsedCookie::Priority() const {
  std::string_view value =
      priority_index_ ? pairs_[priority_index_].second : std::string_view();
  return static_cast<CookiePriority>(
      rustium_cookie_query(5, value.data(), value.size()));
}

bool ParsedCookie::Mutate(size_t field, std::string_view input, bool enabled) {
  const std::string_view name = IsValid() ? Name() : std::string_view();
  const std::string_view value = IsValid() ? Value() : std::string_view();
  const auto plan =
      rustium_cookie_mutation(field, input.data(), input.size(), name.data(),
                              name.size(), value.data(), value.size(), enabled);
  if (plan.action == 0) {
    return false;
  }
  if (plan.action == 3) {
    const auto indexes = AttributeIndexes();
    const size_t removed = *indexes[field - 2];
    if (removed) {
      std::array<size_t, 9> updated;
      for (size_t i = 0; i < updated.size(); ++i) {
        updated[i] = *indexes[i];
      }
      rustium_cookie_clear(updated.data(), removed);
      for (size_t i = 0; i < updated.size(); ++i) {
        *indexes[i] = updated[i];
      }
      pairs_.erase(pairs_.begin() + removed);
    }
    return true;
  }
  // Rust has released all input borrows. Existing-string assignment supports
  // overlapping string_views, as in the original C++ implementation. Only an
  // insertion needs a staged copy before vector growth can invalidate input.
  const auto parsed = input.substr(plan.begin, plan.len);
  if (plan.action == 1 || plan.action == 2) {
    if (pairs_.empty()) {
      std::string staged(parsed);
      pairs_.emplace_back("", "");
      if (plan.action == 1) {
        pairs_[0].first = std::move(staged);
      } else {
        pairs_[0].second = std::move(staged);
      }
    } else if (plan.action == 1) {
      pairs_[0].first.assign(parsed);
    } else {
      pairs_[0].second.assign(parsed);
    }
  } else {
    size_t& index = *AttributeIndexes()[field - 2];
    if (index) {
      pairs_[index].second.assign(parsed);
    } else {
      std::string staged(parsed);
      pairs_.emplace_back(kAttributeNames[field - 2], std::move(staged));
      index = pairs_.size() - 1;
    }
  }
  return true;
}

bool ParsedCookie::SetName(std::string_view v) {
  return Mutate(0, v);
}
bool ParsedCookie::SetValue(std::string_view v) {
  return Mutate(1, v);
}
bool ParsedCookie::SetPath(std::string_view v) {
  return Mutate(2, v);
}
bool ParsedCookie::SetDomain(std::string_view v) {
  return Mutate(3, v);
}
bool ParsedCookie::SetExpires(std::string_view v) {
  return Mutate(4, v);
}
bool ParsedCookie::SetMaxAge(std::string_view v) {
  return Mutate(5, v);
}
bool ParsedCookie::SetIsSecure(bool v) {
  return Mutate(6, {}, v);
}
bool ParsedCookie::SetIsHttpOnly(bool v) {
  return Mutate(7, {}, v);
}
bool ParsedCookie::SetSameSite(std::string_view v) {
  return Mutate(8, v);
}
bool ParsedCookie::SetPriority(std::string_view v) {
  return Mutate(9, v);
}
bool ParsedCookie::SetIsPartitioned(bool v) {
  return Mutate(10, {}, v);
}

std::string ParsedCookie::ToCookieLine() const {
  std::string out;
  bool first = true;
  for (const auto& [name, value] : pairs_) {
    const auto flags = rustium_cookie_serialization(name.data(), name.size(),
                                                    first, out.empty());
    if (flags & 2) {
      out.append("; ");
    }
    out.append(name);
    if (flags & 1) {
      out.append("=");
      out.append(value);
    }
    first = false;
  }
  return out;
}

std::string_view::iterator ParsedCookie::FindFirstTerminator(
    std::string_view s) {
  return At(s, rustium_cookie_query(0, s.data(), s.size()));
}
bool ParsedCookie::ParseToken(std::string_view::iterator* it,
                              const std::string_view::iterator& end,
                              std::string_view::iterator* start,
                              std::string_view::iterator* finish) {
  DCHECK(it && start && finish);
  const auto input = Remaining(*it, end);
  const auto scan =
      rustium_cookie_scan(input.data(), input.size(), true, false,
                          IteratorAliases(it, end, start, finish));
  *it = At(input, scan.next);
  if (!scan.found) {
    return false;
  }
  *start = At(input, scan.begin);
  *finish = At(input, scan.begin + scan.len);
  return true;
}
void ParsedCookie::ParseValue(std::string_view::iterator* it,
                              const std::string_view::iterator& end,
                              std::string_view::iterator* start,
                              std::string_view::iterator* finish) {
  DCHECK(it && start && finish);
  const auto input = Remaining(*it, end);
  const auto scan =
      rustium_cookie_scan(input.data(), input.size(), false, false,
                          IteratorAliases(it, end, start, finish));
  *it = At(input, scan.next);
  *start = At(input, scan.begin);
  *finish = At(input, scan.begin + scan.len);
}
std::string_view ParsedCookie::ParseTokenString(std::string_view s) {
  const auto scan = rustium_cookie_scan(s.data(), s.size(), true, true, 0);
  return scan.found ? s.substr(scan.begin, scan.len) : std::string_view();
}
std::string_view ParsedCookie::ParseValueString(std::string_view s) {
  const auto scan = rustium_cookie_scan(s.data(), s.size(), false, true, 0);
  return s.substr(scan.begin, scan.len);
}
bool ParsedCookie::ValueMatchesParsedValue(std::string_view s) {
  return rustium_cookie_query(4, s.data(), s.size());
}
bool ParsedCookie::IsValidCookieName(std::string_view s) {
  return rustium_cookie_query(1, s.data(), s.size());
}
bool ParsedCookie::IsValidCookieValue(std::string_view s) {
  return rustium_cookie_query(2, s.data(), s.size());
}
bool ParsedCookie::CookieAttributeValueHasValidCharSet(std::string_view s) {
  return rustium_cookie_query(2, s.data(), s.size());
}
bool ParsedCookie::CookieAttributeValueHasValidSize(std::string_view s) {
  return rustium_cookie_query(3, s.data(), s.size());
}
bool ParsedCookie::IsValidCookieNameValuePair(std::string_view name,
                                              std::string_view value,
                                              CookieInclusionStatus* status) {
  const auto reason = rustium_cookie_valid_pair(name.data(), name.size(),
                                                value.data(), value.size());
  ApplyReason(reason, status);
  return reason == 0;
}
bool ParsedCookie::ForEachAttribute(
    base::FunctionRef<bool(std::string_view, std::string_view)> functor) const {
  // Callback invocation and C++ view lifetimes remain storage adapter seams.
  for (size_t i = 1; i < pairs_.size(); ++i) {
    if (!functor(pairs_[i].first, pairs_[i].second)) {
      return false;
    }
  }
  return true;
}
}  // namespace net
