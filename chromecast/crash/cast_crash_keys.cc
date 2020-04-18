// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromecast/crash/cast_crash_keys.h"

namespace chromecast {
namespace crash_keys {

crash_reporter::CrashKeyString<64> last_app("last_app");

crash_reporter::CrashKeyString<64> previous_app("previous_app");

}  // namespace crash_keys
}  // namespace chromecast
