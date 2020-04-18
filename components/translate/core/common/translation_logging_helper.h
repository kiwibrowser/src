// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_TRANSLATE_CORE_COMMON_TRANSLATION_LOGGING_HELPER_H_
#define COMPONENTS_TRANSLATE_CORE_COMMON_TRANSLATION_LOGGING_HELPER_H_

#include <stdint.h>

namespace metrics {
class TranslateEventProto;
}  // namespace metrics

namespace sync_pb {
class UserEventSpecifics;
}  // namespace sync_pb

namespace translate {

// Construct the sync_pb::Translation proto.
// If the event is the event that we care about, will return true, otherwise,
// will return false.
bool ConstructTranslateEvent(
    int64_t navigation_id,
    const metrics::TranslateEventProto& translate_event,
    sync_pb::UserEventSpecifics* const specifics);
}  // namespace translate

#endif  // COMPONENTS_TRANSLATE_CORE_COMMON_TRANSLATION_LOGGING_HELPER_H_