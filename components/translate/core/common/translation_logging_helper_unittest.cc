// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/translate/core/common/translation_logging_helper.h"

#include <string>

#include "base/logging.h"
#include "components/sync/protocol/user_event_specifics.pb.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/metrics_proto/translate_event.pb.h"

using metrics::TranslateEventProto;
using Translation = sync_pb::UserEventSpecifics::Translation;

void EqualTranslationProto(const Translation& first,
                           const Translation& second) {
  EXPECT_EQ(first.from_language_code(), second.from_language_code());
  EXPECT_EQ(first.to_language_code(), second.to_language_code());
  EXPECT_EQ(first.interaction(), second.interaction());
}

namespace translate {

// Tests that UserEventSpecifics is correctly built.
TEST(TranslationLoggingHelperTest, ConstructUserEventSpecifics) {
  // The event we have.
  TranslateEventProto translation_event;
  translation_event.set_source_language("ja");
  translation_event.set_target_language("en");
  translation_event.set_event_type(TranslateEventProto::USER_DECLINE);
  // Expected user_event.
  Translation user_translation_event;
  user_translation_event.set_from_language_code("ja");
  user_translation_event.set_to_language_code("en");
  user_translation_event.set_interaction(Translation::DECLINE);
  // The user event.
  sync_pb::UserEventSpecifics user_specifics;
  const int64_t navigation_id = 1000000000000000LL;
  const bool needs_logging = ConstructTranslateEvent(
      navigation_id, translation_event, &user_specifics);
  EXPECT_TRUE(needs_logging);
  EXPECT_EQ(user_specifics.navigation_id(), navigation_id);
  EqualTranslationProto(user_translation_event,
                        user_specifics.translation_event());
}

// Tests that if user change the target language, the event is MANUAL.
TEST(TranslationLoggingHelperTest, UserManualEvent) {
  // The event we have.
  TranslateEventProto translation_event;
  translation_event.set_source_language("ja");
  translation_event.set_target_language("en");
  translation_event.set_modified_target_language("fr");
  translation_event.set_event_type(TranslateEventProto::USER_ACCEPT);
  // Expected user_event.
  Translation user_translation_event;
  user_translation_event.set_from_language_code("ja");
  user_translation_event.set_to_language_code("fr");
  user_translation_event.set_interaction(Translation::MANUAL);
  // The user event.
  sync_pb::UserEventSpecifics user_specifics;
  const int64_t navigation_id = 100;
  const bool needs_logging = ConstructTranslateEvent(
      navigation_id, translation_event, &user_specifics);
  EXPECT_TRUE(needs_logging);
  EXPECT_EQ(user_specifics.navigation_id(), navigation_id);
  EqualTranslationProto(user_translation_event,
                        user_specifics.translation_event());
}

// Tests that we don't build unnecessary events.
TEST(TranslationLoggingHelperTest, DontBuildUnnecessaryEvent) {
  // The event we have.
  TranslateEventProto translation_event;
  translation_event.set_source_language("ja");
  translation_event.set_target_language("en");
  // The event we don't care.
  translation_event.set_event_type(TranslateEventProto::DISABLED_BY_RANKER);
  // The user event.
  sync_pb::UserEventSpecifics user_specifics;
  const bool needs_logging =
      ConstructTranslateEvent(100, translation_event, &user_specifics);
  // We don't expect the event to be logged.
  EXPECT_FALSE(needs_logging);
}

}  // namespace translate
