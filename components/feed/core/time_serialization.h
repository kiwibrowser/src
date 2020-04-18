// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_FEED_CORE_TIME_SERIALIZATION_H_
#define COMPONENTS_FEED_CORE_TIME_SERIALIZATION_H_

#include <stdint.h>

#include "base/time/time.h"

namespace feed {

int64_t ToDatabaseTime(base::Time time);
base::Time FromDatabaseTime(int64_t serialized_time);

}  // namespace feed

#endif  // COMPONENTS_FEED_CORE_TIME_SERIALIZATION_H_
