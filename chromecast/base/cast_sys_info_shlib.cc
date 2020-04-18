// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromecast/public/cast_sys_info_shlib.h"

#include "chromecast/base/cast_sys_info_dummy.h"

namespace chromecast {

// static
CastSysInfo* CastSysInfoShlib::Create(const std::vector<std::string>& argv) {
  return new CastSysInfoDummy();
}

}  // namespace chromecast
