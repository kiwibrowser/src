// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/translate/core/common/language_detection_logging_helper.h"

#include <memory>

#include "base/logging.h"
#include "components/sync/protocol/user_event_specifics.pb.h"
#include "components/translate/core/common/language_detection_details.h"

namespace translate {

std::unique_ptr<sync_pb::UserEventSpecifics> ConstructLanguageDetectionEvent(
    const int64_t navigation_id,
    const LanguageDetectionDetails& details) {
  auto specifics = std::make_unique<sync_pb::UserEventSpecifics>();
  specifics->set_event_time_usec(base::Time::Now().ToInternalValue());

  specifics->set_navigation_id(navigation_id);

  sync_pb::UserEventSpecifics::LanguageDetection lang_detection;
  auto* const lang = lang_detection.add_detected_languages();
  lang->set_language_code(details.cld_language);
  lang->set_is_reliable(details.is_cld_reliable);
  // Only set adopted_language when it's different from cld_language.
  if (details.adopted_language != details.cld_language) {
    lang_detection.set_adopted_language_code(details.adopted_language);
  }
  *specifics->mutable_language_detection_event() = lang_detection;
  return specifics;
}

}  // namespace translate
