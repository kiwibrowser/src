// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/service_manager/public/cpp/standalone_service/switches.h"

namespace service_manager {
namespace switches {

// The path where ICU initialization data can be found. If not provided, the
// service binary's directory is assumed.
const char kIcuDataDir[] = "icu-data-dir";

}  // namespace switches
}  // namespace service_manager
