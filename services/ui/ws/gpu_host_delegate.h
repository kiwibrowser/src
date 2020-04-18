// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_WS_GPU_SERVICE_PROXY_DELEGATEH_
#define SERVICES_UI_WS_GPU_SERVICE_PROXY_DELEGATEH_

#include "base/memory/ref_counted.h"

namespace ui {
namespace ws {

class GpuHostDelegate {
 public:
  virtual ~GpuHostDelegate() {}

  virtual void OnGpuServiceInitialized() = 0;
};

}  // namespace ws
}  // namespace ui

#endif  // SERVICES_UI_WS_GPU_SERVICE_PROXY_DELEGATEH_
