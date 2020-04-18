// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_PUBLIC_CPP_RASTER_THREAD_HELPER_H_
#define SERVICES_UI_PUBLIC_CPP_RASTER_THREAD_HELPER_H_

#include <memory>

#include "base/macros.h"

namespace cc {
class TaskGraphRunner;
class SingleThreadTaskGraphRunner;
}

namespace ui {

class RasterThreadHelper {
 public:
  RasterThreadHelper();
  ~RasterThreadHelper();

  cc::TaskGraphRunner* task_graph_runner();

 private:
  std::unique_ptr<cc::SingleThreadTaskGraphRunner> task_graph_runner_;

  DISALLOW_COPY_AND_ASSIGN(RasterThreadHelper);
};

}  // namespace ui

#endif  // SERVICES_UI_PUBLIC_CPP_RASTER_THREAD_HELPER_H_
