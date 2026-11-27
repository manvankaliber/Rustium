// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/html/forms/chooser_resource_loader.h"

#include "base/notreached.h"
#include "build/build_config.h"
#include "third_party/blink/public/resources/grit/blink_resources.h"
#include "third_party/blink/renderer/platform/data_resource_helper.h"

namespace blink {

Vector<char> ChooserResourceLoader::GetSuggestionPickerStyleSheet() {
  return UncompressResourceAsBinary(IDR_SUGGESTION_PICKER_CSS);
}

Vector<char> ChooserResourceLoader::GetSuggestionPickerJS() {
  return UncompressResourceAsBinary(IDR_SUGGESTION_PICKER_JS);
}

Vector<char> ChooserResourceLoader::GetPickerCommonStyleSheet() {
  return UncompressResourceAsBinary(IDR_PICKER_COMMON_CSS);
}

Vector<char> ChooserResourceLoader::GetPickerCommonJS() {
  return UncompressResourceAsBinary(IDR_PICKER_COMMON_JS);
}

Vector<char> ChooserResourceLoader::GetCalendarPickerStyleSheet() {
  return UncompressResourceAsBinary(IDR_CALENDAR_PICKER_CSS);
}

Vector<char> ChooserResourceLoader::GetCalendarPickerJS() {
  return UncompressResourceAsBinary(IDR_CALENDAR_PICKER_JS);
}

Vector<char> ChooserResourceLoader::GetMonthPickerJS() {
  return UncompressResourceAsBinary(IDR_MONTH_PICKER_JS);
}

Vector<char> ChooserResourceLoader::GetTimePickerStyleSheet() {
  return UncompressResourceAsBinary(IDR_TIME_PICKER_CSS);
}

Vector<char> ChooserResourceLoader::GetTimePickerJS() {
  return UncompressResourceAsBinary(IDR_TIME_PICKER_JS);
}

Vector<char> ChooserResourceLoader::GetDateTimeLocalPickerJS() {
  return UncompressResourceAsBinary(IDR_DATETIMELOCAL_PICKER_JS);
}

Vector<char> ChooserResourceLoader::GetColorSuggestionPickerStyleSheet() {
  return UncompressResourceAsBinary(IDR_COLOR_SUGGESTION_PICKER_CSS);
}

Vector<char> ChooserResourceLoader::GetColorSuggestionPickerJS() {
  return UncompressResourceAsBinary(IDR_COLOR_SUGGESTION_PICKER_JS);
}

Vector<char> ChooserResourceLoader::GetColorPickerStyleSheet() {
  return UncompressResourceAsBinary(IDR_COLOR_PICKER_CSS);
}

Vector<char> ChooserResourceLoader::GetColorPickerJS() {
  return UncompressResourceAsBinary(IDR_COLOR_PICKER_JS);
}

Vector<char> ChooserResourceLoader::GetColorPickerCommonJS() {
  return UncompressResourceAsBinary(IDR_COLOR_PICKER_COMMON_JS);
}

Vector<char> ChooserResourceLoader::GetListPickerStyleSheet() {
  return UncompressResourceAsBinary(IDR_LIST_PICKER_CSS);
}

Vector<char> ChooserResourceLoader::GetListPickerJS() {
  return UncompressResourceAsBinary(IDR_LIST_PICKER_JS);
}

}  // namespace blink
