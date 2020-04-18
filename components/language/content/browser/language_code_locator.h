// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_LANGUAGE_CONTENT_BROWSER_LANGUAGE_CODE_LOCATOR_H_
#define COMPONENTS_LANGUAGE_CONTENT_BROWSER_LANGUAGE_CODE_LOCATOR_H_

#include <string>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/macros.h"

namespace language {

class LanguageCodeLocator {
 public:
  LanguageCodeLocator();
  ~LanguageCodeLocator();

  // Find the language code given a coordinate.
  // If the latitude, longitude pair is not found, will return an empty vector.
  std::vector<std::string> GetLanguageCode(double latitude,
                                           double longitude) const;

 private:
  // Map from s2 cellid to ';' delimited list of language codes enum.
  base::flat_map<uint32_t, char> district_languages_;

  DISALLOW_COPY_AND_ASSIGN(LanguageCodeLocator);
};

}  // namespace language

#endif  // COMPONENTS_LANGUAGE_CONTENT_BROWSER_LANGUAGE_CODE_LOCATOR_H_
