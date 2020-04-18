// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/idle/idle_query_x11.h"

#include "ui/gfx/x/x11.h"
#include "ui/gfx/x/x11_types.h"

namespace ui {

class IdleData {
 public:
  IdleData() {
    int event_base;
    int error_base;
    if (XScreenSaverQueryExtension(gfx::GetXDisplay(), &event_base,
                                   &error_base)) {
      mit_info.reset(XScreenSaverAllocInfo());
    }
  }

  ~IdleData() {
  }

  gfx::XScopedPtr<XScreenSaverInfo> mit_info;
};

IdleQueryX11::IdleQueryX11() : idle_data_(new IdleData()) {}

IdleQueryX11::~IdleQueryX11() {}

int IdleQueryX11::IdleTime() {
  if (!idle_data_->mit_info)
    return 0;

  if (XScreenSaverQueryInfo(gfx::GetXDisplay(),
                            XRootWindow(gfx::GetXDisplay(), 0),
                            idle_data_->mit_info.get())) {
    return (idle_data_->mit_info->idle) / 1000;
  } else {
    return 0;
  }
}

}  // namespace ui
