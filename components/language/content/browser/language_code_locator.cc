// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/language/content/browser/language_code_locator.h"

#include "base/strings/string_split.h"
#include "third_party/s2cellid/src/s2/s2cellid.h"
#include "third_party/s2cellid/src/s2/s2latlng.h"

namespace language {
namespace {
#include "components/language/content/browser/language_code_locator_helper.h"
}  // namespace

LanguageCodeLocator::LanguageCodeLocator() {
  int pos = 0;
  int index = 0;
  std::vector<std::pair<uint32_t, char>> items(arraysize(kDistrictPositions));
  for (const uint16_t language_count : kLanguageCodeCounts) {
    for (int i = 0; i < language_count; ++i) {
      items[pos] = std::make_pair(kDistrictPositions[pos], index);
      ++pos;
    }
    ++index;
  }
  district_languages_ = base::flat_map<uint32_t, char>(items);
}

LanguageCodeLocator::~LanguageCodeLocator() {}

std::vector<std::string> LanguageCodeLocator::GetLanguageCode(
    double latitude,
    double longitude) const {
  S2CellId current_cell(S2LatLng::FromDegrees(latitude, longitude));
  while (current_cell.level() > 0) {
    auto search = district_languages_.find(current_cell.id() >> 32);
    if (search != district_languages_.end()) {
      return base::SplitString(
          kLanguageEnumCodeMapping[static_cast<uint8_t>(search->second)], ";",
          base::KEEP_WHITESPACE, base::SPLIT_WANT_ALL);
    }
    current_cell = current_cell.parent();
  }
  return {};
}

}  // namespace language
