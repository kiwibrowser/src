// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/public/cpp/raster_thread_helper.h"

#include "base/logging.h"
#include "base/threading/simple_thread.h"
#include "cc/raster/single_thread_task_graph_runner.h"

namespace ui {

RasterThreadHelper::RasterThreadHelper()
    : task_graph_runner_(new cc::SingleThreadTaskGraphRunner) {
  task_graph_runner_->Start("CompositorTileWorker1",
                            base::SimpleThread::Options());
}

RasterThreadHelper::~RasterThreadHelper() {
  task_graph_runner_->Shutdown();
}

cc::TaskGraphRunner* RasterThreadHelper::task_graph_runner() {
  return task_graph_runner_.get();
}

}  // namespace ui
