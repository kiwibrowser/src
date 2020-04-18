// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_TRANSLATE_CORE_COMMON_LANGUAGE_DETECTION_LOGGING_HELPER_H_
#define COMPONENTS_TRANSLATE_CORE_COMMON_LANGUAGE_DETECTION_LOGGING_HELPER_H_

#include <memory>

namespace sync_pb {
class UserEventSpecifics;
}

namespace translate {

struct LanguageDetectionDetails;

// Construct language detection based on navigation_id and language detection
// details.
std::unique_ptr<sync_pb::UserEventSpecifics> ConstructLanguageDetectionEvent(
    int64_t navigation_id,
    const LanguageDetectionDetails& details);

}  // namespace translate

#endif  // COMPONENTS_TRANSLATE_CORE_COMMON_LANGUAGE_DETECTION_LOGGING_HELPER_H_
