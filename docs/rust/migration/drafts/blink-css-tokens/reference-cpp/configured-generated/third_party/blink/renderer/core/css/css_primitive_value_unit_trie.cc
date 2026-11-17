// Copyright 2016 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Generated from template:
//   core/css/templates/css_primitive_value_unit_trie.cc.tmpl
// and input files:
//   ../../third_party/blink/renderer/core/css/css_primitive_value_units.json5


#include "third_party/blink/renderer/core/css/css_primitive_value.h"

namespace blink {

namespace {

template<typename CharacterType>
CSSPrimitiveValue::UnitType CssPrimitiveValueUnitFromTrie(
    base::span<const CharacterType> chars) {
  const CharacterType* data = chars.data();
  unsigned length = static_cast<unsigned>(chars.size());
  DCHECK(data);
  DCHECK(length);
  switch (length) {
  case 1:
    switch (ToAsciiLower(data[0])) {
    case 'q':
      return CSSPrimitiveValue::UnitType::kQuarterMillimeters;
    case 's':
      return CSSPrimitiveValue::UnitType::kSeconds;
    case 'x':
      return CSSPrimitiveValue::UnitType::kX;
    }
    break;
  case 2:
    switch (ToAsciiLower(data[0])) {
    case 'c':
      switch (ToAsciiLower(data[1])) {
      case 'h':
        return CSSPrimitiveValue::UnitType::kChs;
      case 'm':
        return CSSPrimitiveValue::UnitType::kCentimeters;
      }
      break;
    case 'e':
      switch (ToAsciiLower(data[1])) {
      case 'm':
        return CSSPrimitiveValue::UnitType::kEms;
      case 'x':
        return CSSPrimitiveValue::UnitType::kExs;
      }
      break;
    case 'f':
      if (ToAsciiLower(data[1]) == 'r') {
        return CSSPrimitiveValue::UnitType::kFlex;
      }
      break;
    case 'h':
      if (ToAsciiLower(data[1]) == 'z') {
        return CSSPrimitiveValue::UnitType::kHertz;
      }
      break;
    case 'i':
      switch (ToAsciiLower(data[1])) {
      case 'c':
        return CSSPrimitiveValue::UnitType::kIcs;
      case 'n':
        return CSSPrimitiveValue::UnitType::kInches;
      }
      break;
    case 'l':
      if (ToAsciiLower(data[1]) == 'h') {
        return CSSPrimitiveValue::UnitType::kLhs;
      }
      break;
    case 'm':
      switch (ToAsciiLower(data[1])) {
      case 'm':
        return CSSPrimitiveValue::UnitType::kMillimeters;
      case 's':
        return CSSPrimitiveValue::UnitType::kMilliseconds;
      }
      break;
    case 'p':
      switch (ToAsciiLower(data[1])) {
      case 'c':
        return CSSPrimitiveValue::UnitType::kPicas;
      case 't':
        return CSSPrimitiveValue::UnitType::kPoints;
      case 'x':
        return CSSPrimitiveValue::UnitType::kPixels;
      }
      break;
    case 'v':
      switch (ToAsciiLower(data[1])) {
      case 'b':
        return CSSPrimitiveValue::UnitType::kViewportBlockSize;
      case 'h':
        return CSSPrimitiveValue::UnitType::kViewportHeight;
      case 'i':
        return CSSPrimitiveValue::UnitType::kViewportInlineSize;
      case 'w':
        return CSSPrimitiveValue::UnitType::kViewportWidth;
      }
      break;
    }
    break;
  case 3:
    switch (ToAsciiLower(data[0])) {
    case 'c':
      switch (ToAsciiLower(data[1])) {
      case 'a':
        if (ToAsciiLower(data[2]) == 'p') {
          return CSSPrimitiveValue::UnitType::kCaps;
        }
        break;
      case 'q':
        switch (ToAsciiLower(data[2])) {
        case 'b':
          return CSSPrimitiveValue::UnitType::kContainerBlockSize;
        case 'h':
          return CSSPrimitiveValue::UnitType::kContainerHeight;
        case 'i':
          return CSSPrimitiveValue::UnitType::kContainerInlineSize;
        case 'w':
          return CSSPrimitiveValue::UnitType::kContainerWidth;
        }
        break;
      }
      break;
    case 'd':
      switch (ToAsciiLower(data[1])) {
      case 'e':
        if (ToAsciiLower(data[2]) == 'g') {
          return CSSPrimitiveValue::UnitType::kDegrees;
        }
        break;
      case 'p':
        if (ToAsciiLower(data[2]) == 'i') {
          return CSSPrimitiveValue::UnitType::kDotsPerInch;
        }
        break;
      case 'v':
        switch (ToAsciiLower(data[2])) {
        case 'b':
          return CSSPrimitiveValue::UnitType::kDynamicViewportBlockSize;
        case 'h':
          return CSSPrimitiveValue::UnitType::kDynamicViewportHeight;
        case 'i':
          return CSSPrimitiveValue::UnitType::kDynamicViewportInlineSize;
        case 'w':
          return CSSPrimitiveValue::UnitType::kDynamicViewportWidth;
        }
        break;
      }
      break;
    case 'k':
      if (ToAsciiLower(data[1]) == 'h' && ToAsciiLower(data[2]) == 'z') {
        return CSSPrimitiveValue::UnitType::kKilohertz;
      }
      break;
    case 'l':
      switch (ToAsciiLower(data[1])) {
      case 'v':
        switch (ToAsciiLower(data[2])) {
        case 'b':
          return CSSPrimitiveValue::UnitType::kLargeViewportBlockSize;
        case 'h':
          return CSSPrimitiveValue::UnitType::kLargeViewportHeight;
        case 'i':
          return CSSPrimitiveValue::UnitType::kLargeViewportInlineSize;
        case 'w':
          return CSSPrimitiveValue::UnitType::kLargeViewportWidth;
        }
        break;
      }
      break;
    case 'r':
      switch (ToAsciiLower(data[1])) {
      case 'a':
        if (ToAsciiLower(data[2]) == 'd') {
          return CSSPrimitiveValue::UnitType::kRadians;
        }
        break;
      case 'c':
        if (ToAsciiLower(data[2]) == 'h') {
          return CSSPrimitiveValue::UnitType::kRchs;
        }
        break;
      case 'e':
        switch (ToAsciiLower(data[2])) {
        case 'm':
          return CSSPrimitiveValue::UnitType::kRems;
        case 'x':
          return CSSPrimitiveValue::UnitType::kRexs;
        }
        break;
      case 'i':
        if (ToAsciiLower(data[2]) == 'c') {
          return CSSPrimitiveValue::UnitType::kRics;
        }
        break;
      case 'l':
        if (ToAsciiLower(data[2]) == 'h') {
          return CSSPrimitiveValue::UnitType::kRlhs;
        }
        break;
      }
      break;
    case 's':
      switch (ToAsciiLower(data[1])) {
      case 'v':
        switch (ToAsciiLower(data[2])) {
        case 'b':
          return CSSPrimitiveValue::UnitType::kSmallViewportBlockSize;
        case 'h':
          return CSSPrimitiveValue::UnitType::kSmallViewportHeight;
        case 'i':
          return CSSPrimitiveValue::UnitType::kSmallViewportInlineSize;
        case 'w':
          return CSSPrimitiveValue::UnitType::kSmallViewportWidth;
        }
        break;
      }
      break;
    }
    break;
  case 4:
    switch (ToAsciiLower(data[0])) {
    case 'd':
      switch (ToAsciiLower(data[1])) {
      case 'p':
        switch (ToAsciiLower(data[2])) {
        case 'c':
          if (ToAsciiLower(data[3]) == 'm') {
            return CSSPrimitiveValue::UnitType::kDotsPerCentimeter;
          }
          break;
        case 'p':
          if (ToAsciiLower(data[3]) == 'x') {
            return CSSPrimitiveValue::UnitType::kDotsPerPixel;
          }
          break;
        }
        break;
      }
      break;
    case 'g':
      if (ToAsciiLower(data[1]) == 'r' && ToAsciiLower(data[2]) == 'a' && ToAsciiLower(data[3]) == 'd') {
        return CSSPrimitiveValue::UnitType::kGradians;
      }
      break;
    case 'r':
      if (ToAsciiLower(data[1]) == 'c' && ToAsciiLower(data[2]) == 'a' && ToAsciiLower(data[3]) == 'p') {
        return CSSPrimitiveValue::UnitType::kRcaps;
      }
      break;
    case 't':
      if (ToAsciiLower(data[1]) == 'u' && ToAsciiLower(data[2]) == 'r' && ToAsciiLower(data[3]) == 'n') {
        return CSSPrimitiveValue::UnitType::kTurns;
      }
      break;
    case 'v':
      switch (ToAsciiLower(data[1])) {
      case 'm':
        switch (ToAsciiLower(data[2])) {
        case 'a':
          if (ToAsciiLower(data[3]) == 'x') {
            return CSSPrimitiveValue::UnitType::kViewportMax;
          }
          break;
        case 'i':
          if (ToAsciiLower(data[3]) == 'n') {
            return CSSPrimitiveValue::UnitType::kViewportMin;
          }
          break;
        }
        break;
      }
      break;
    }
    break;
  case 5:
    switch (ToAsciiLower(data[0])) {
    case '_':
      if (ToAsciiLower(data[1]) == '_' && ToAsciiLower(data[2]) == 'q' && ToAsciiLower(data[3]) == 'e' && ToAsciiLower(data[4]) == 'm') {
        return CSSPrimitiveValue::UnitType::kQuirkyEms;
      }
      break;
    case 'c':
      switch (ToAsciiLower(data[1])) {
      case 'q':
        switch (ToAsciiLower(data[2])) {
        case 'm':
          switch (ToAsciiLower(data[3])) {
          case 'a':
            if (ToAsciiLower(data[4]) == 'x') {
              return CSSPrimitiveValue::UnitType::kContainerMax;
            }
            break;
          case 'i':
            if (ToAsciiLower(data[4]) == 'n') {
              return CSSPrimitiveValue::UnitType::kContainerMin;
            }
            break;
          }
          break;
        }
        break;
      }
      break;
    case 'd':
      switch (ToAsciiLower(data[1])) {
      case 'v':
        switch (ToAsciiLower(data[2])) {
        case 'm':
          switch (ToAsciiLower(data[3])) {
          case 'a':
            if (ToAsciiLower(data[4]) == 'x') {
              return CSSPrimitiveValue::UnitType::kDynamicViewportMax;
            }
            break;
          case 'i':
            if (ToAsciiLower(data[4]) == 'n') {
              return CSSPrimitiveValue::UnitType::kDynamicViewportMin;
            }
            break;
          }
          break;
        }
        break;
      }
      break;
    case 'l':
      switch (ToAsciiLower(data[1])) {
      case 'v':
        switch (ToAsciiLower(data[2])) {
        case 'm':
          switch (ToAsciiLower(data[3])) {
          case 'a':
            if (ToAsciiLower(data[4]) == 'x') {
              return CSSPrimitiveValue::UnitType::kLargeViewportMax;
            }
            break;
          case 'i':
            if (ToAsciiLower(data[4]) == 'n') {
              return CSSPrimitiveValue::UnitType::kLargeViewportMin;
            }
            break;
          }
          break;
        }
        break;
      }
      break;
    case 's':
      switch (ToAsciiLower(data[1])) {
      case 'v':
        switch (ToAsciiLower(data[2])) {
        case 'm':
          switch (ToAsciiLower(data[3])) {
          case 'a':
            if (ToAsciiLower(data[4]) == 'x') {
              return CSSPrimitiveValue::UnitType::kSmallViewportMax;
            }
            break;
          case 'i':
            if (ToAsciiLower(data[4]) == 'n') {
              return CSSPrimitiveValue::UnitType::kSmallViewportMin;
            }
            break;
          }
          break;
        }
        break;
      }
      break;
    }
    break;
  }

  return CSSPrimitiveValue::UnitType::kUnknown;
}

} // namespace

CSSPrimitiveValue::UnitType CSSPrimitiveValue::StringToUnitType(
    base::span<const LChar> chars) {
  return CssPrimitiveValueUnitFromTrie(chars);
}

CSSPrimitiveValue::UnitType CSSPrimitiveValue::StringToUnitType(
    base::span<const UChar> chars) {
  return CssPrimitiveValueUnitFromTrie(chars);
}

} // namespace blink

