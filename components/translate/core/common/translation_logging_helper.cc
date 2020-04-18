// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/translate/core/common/translation_logging_helper.h"

#include "base/logging.h"
#include "base/time/time.h"
#include "components/sync/protocol/user_event_specifics.pb.h"
#include "third_party/metrics_proto/translate_event.pb.h"

using Translation = sync_pb::UserEventSpecifics::Translation;

namespace translate {
namespace {
using metrics::TranslateEventProto;
}  // namespace

bool ConstructTranslateEvent(const int64_t navigation_id,
                             const TranslateEventProto& translate_event,
                             sync_pb::UserEventSpecifics* const specifics) {
  specifics->set_event_time_usec(base::Time::Now().ToInternalValue());

  specifics->set_navigation_id(navigation_id);
  auto* const translation = specifics->mutable_translation_event();
  translation->set_from_language_code(translate_event.source_language());
  translation->set_to_language_code(translate_event.target_language());
  switch (translate_event.event_type()) {
    case TranslateEventProto::UNKNOWN:
      translation->set_interaction(Translation::UNKNOWN);
      break;
    case TranslateEventProto::USER_ACCEPT:
      if (translate_event.has_modified_source_language() ||
          translate_event.has_modified_target_language()) {
        // Special case, since we don't have event enum telling us it's actually
        // modified by user, we do this by check whether this event has modified
        // source or target language.
        if (translate_event.has_modified_source_language()) {
          translation->set_from_language_code(
              translate_event.modified_source_language());
        }
        if (translate_event.has_modified_target_language()) {
          translation->set_to_language_code(
              translate_event.modified_target_language());
        }
        translation->set_interaction(Translation::MANUAL);
      } else {
        translation->set_interaction(Translation::ACCEPT);
      }
      break;
    case TranslateEventProto::USER_DECLINE:
      translation->set_interaction(Translation::DECLINE);
      break;
    case TranslateEventProto::USER_IGNORE:
      translation->set_interaction(Translation::IGNORED);
      break;
    case TranslateEventProto::USER_DISMISS:
      translation->set_interaction(Translation::DISMISSED);
      break;
    case TranslateEventProto::USER_REVERT:
      translation->set_interaction(Translation::TRANSLATION_REVERTED);
      break;
    case TranslateEventProto::AUTO_TRANSLATION_BY_PREF:
      translation->set_interaction(Translation::AUTO_TRANSLATION_BY_PREF);
      break;
    case TranslateEventProto::AUTO_TRANSLATION_BY_LINK:
      translation->set_interaction(Translation::AUTO_TRANSLATION_BY_LINK);
      break;
    case TranslateEventProto::INITIALIZATION_ERROR:
      translation->set_interaction(Translation::INITIALIZATION_ERROR);
      break;
    default:  // We don't care about other events.
      return false;
  }
  return true;
}
}  // namespace translate
