// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/api/event_waiter.h"

namespace openscreen {
namespace platform {

Events::Events() = default;
Events::~Events() = default;
Events::Events(Events&& o) = default;
Events& Events::operator=(Events&& o) = default;

}  // namespace platform
}  // namespace openscreen
