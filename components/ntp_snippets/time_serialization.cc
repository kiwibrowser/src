// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/ntp_snippets/time_serialization.h"

namespace ntp_snippets {

int64_t SerializeTime(const base::Time& time) {
  return SerializeTimeDelta(time - base::Time());
}

base::Time DeserializeTime(int64_t serialized_time) {
  return base::Time() + DeserializeTimeDelta(serialized_time);
}

int64_t SerializeTimeDelta(const base::TimeDelta& time_delta) {
  return time_delta.InMicroseconds();
}

base::TimeDelta DeserializeTimeDelta(int64_t serialized_time_delta) {
  return base::TimeDelta::FromMicroseconds(serialized_time_delta);
}

}  // namespace ntp_snippets
