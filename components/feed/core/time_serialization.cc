// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/feed/core/time_serialization.h"

namespace feed {

int64_t ToDatabaseTime(base::Time time) {
  return time.since_origin().InMicroseconds();
}

base::Time FromDatabaseTime(int64_t serialized_time) {
  return base::Time() + base::TimeDelta::FromMicroseconds(serialized_time);
}

}  // namespace feed
