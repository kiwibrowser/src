// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/translate/core/common/language_detection_details.h"

namespace translate {

LanguageDetectionDetails::LanguageDetectionDetails()
    : is_cld_reliable(false), has_notranslate(false) {
}

LanguageDetectionDetails::LanguageDetectionDetails(
    const LanguageDetectionDetails& other) = default;

LanguageDetectionDetails::~LanguageDetectionDetails() {}

}  // namespace translate
