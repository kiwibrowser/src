// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/translate/core/common/language_detection_logging_helper.h"

#include <string>

#include "components/sync/protocol/user_event_specifics.pb.h"
#include "components/translate/core/common/language_detection_details.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace translate {

// Tests that sync_pb::UserEventSpecifics is correctly build.
TEST(LanguageDetectionLoggingHelperTest, ConstructUserEventSpecifics) {
  LanguageDetectionDetails details;
  details.cld_language = "en";
  details.is_cld_reliable = false;
  details.adopted_language = "ja";
  // Expected language detection.
  sync_pb::UserEventSpecifics::LanguageDetection lang_detection;
  auto* const lang = lang_detection.add_detected_languages();
  lang->set_language_code(details.cld_language);
  lang->set_is_reliable(details.is_cld_reliable);
  lang_detection.set_adopted_language_code(details.adopted_language);
  const int64_t navigation_id = 1000000000000000LL;
  const std::unique_ptr<sync_pb::UserEventSpecifics> user_event =
      ConstructLanguageDetectionEvent(navigation_id, details);
  // Expect the navigation id is correctly set.
  EXPECT_EQ(user_event->navigation_id(), navigation_id);
  EXPECT_EQ(user_event->language_detection_event().SerializeAsString(),
            lang_detection.SerializeAsString());
}

// Tests that sync_pb::UserEventSpecifics is correctly build.
// If adopted_language is the same as cld_language, we don't set it.
TEST(LanguageDetectionLoggingHelperTest, DontSetAdoptedLanguage) {
  LanguageDetectionDetails details;
  details.cld_language = "en";
  details.is_cld_reliable = true;
  details.adopted_language = "en";
  // Expected language detection.
  sync_pb::UserEventSpecifics::LanguageDetection lang_detection;
  auto* const lang = lang_detection.add_detected_languages();
  lang->set_language_code(details.cld_language);
  lang->set_is_reliable(details.is_cld_reliable);
  const std::unique_ptr<sync_pb::UserEventSpecifics> user_event =
      ConstructLanguageDetectionEvent(100, details);
  // Expect the navigation id is correctly set.
  EXPECT_EQ(user_event->navigation_id(), 100);
  EXPECT_EQ(user_event->language_detection_event().SerializeAsString(),
            lang_detection.SerializeAsString());
}

}  // namespace translate
