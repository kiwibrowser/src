// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_COMMON_UTIL_H_
#define SERVICES_UI_COMMON_UTIL_H_

#include <stdint.h>

#include "services/ui/common/types.h"

namespace ui {

inline ClientSpecificId ClientIdFromTransportId(Id id) {
  return static_cast<ClientSpecificId>((id >> 32) & 0xFFFFFFFF);
}

inline ClientSpecificId ClientWindowIdFromTransportId(Id id) {
  return static_cast<ClientSpecificId>(id & 0xFFFFFFFF);
}

}  // namespace ui

#endif  // SERVICES_UI_COMMON_UTIL_H_
