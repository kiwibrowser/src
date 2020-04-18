// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CONTENT_SETTINGS_CORE_COMMON_CONTENT_SETTINGS_PATTERN_PARSER_H_
#define COMPONENTS_CONTENT_SETTINGS_CORE_COMMON_CONTENT_SETTINGS_PATTERN_PARSER_H_

#include <string>

#include "base/macros.h"
#include "components/content_settings/core/common/content_settings_pattern.h"

namespace content_settings {

class PatternParser {
 public:
  static void Parse(const std::string& pattern_spec,
                    ContentSettingsPattern::BuilderInterface* builder);

  static std::string ToString(
      const ContentSettingsPattern::PatternParts& parts);

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(PatternParser);
};

}  // namespace content_settings

#endif  // COMPONENTS_CONTENT_SETTINGS_CORE_COMMON_CONTENT_SETTINGS_PATTERN_PARSER_H_
