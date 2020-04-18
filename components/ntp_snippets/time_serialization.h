// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_NTP_SNIPPETS_TIME_SERIALIZATION_H_
#define COMPONENTS_NTP_SNIPPETS_TIME_SERIALIZATION_H_

#include "base/time/time.h"

namespace ntp_snippets {

// Backward compatible replacements for deprecated
// base::Time::To/FromInternalValue. Only for serialization. Do not change them,
// because the values based on them are persisted in multiple places (e.g.
// prefs, on-disk database). The value repesents number of microseconds since
// 1st of January 1601 (aka Windows epoch).
int64_t SerializeTime(const base::Time& time);
base::Time DeserializeTime(int64_t serialized_time);

// Same as above, but for base::TimeDelta.
int64_t SerializeTimeDelta(const base::TimeDelta& time_delta);
base::TimeDelta DeserializeTimeDelta(int64_t serialized_time_delta);

}  // namespace ntp_snippets

#endif  // COMPONENTS_NTP_SNIPPETS_TIME_SERIALIZATION_H_
