// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_RASTER_TILE_TASK_H_
#define CC_RASTER_TILE_TASK_H_

#include <vector>

#include "cc/raster/task.h"

namespace cc {

class CC_EXPORT TileTask : public Task {
 public:
  typedef std::vector<scoped_refptr<TileTask>> Vector;

  const TileTask::Vector& dependencies() const { return dependencies_; }

  // Indicates whether this TileTask can be run at the same time as other tasks
  // in the task graph. If false, this task will be scheduled with
  // TASK_CATEGORY_NONCONCURRENT_FOREGROUND.
  bool supports_concurrent_execution() const {
    return supports_concurrent_execution_;
  }

  // This function should be called from origin thread to process the completion
  // of the task.
  virtual void OnTaskCompleted() = 0;

  void DidComplete();
  bool HasCompleted() const;

 protected:
  explicit TileTask(bool supports_concurrent_execution);
  TileTask(bool supports_concurrent_execution, TileTask::Vector* dependencies);
  ~TileTask() override;

  const bool supports_concurrent_execution_;
  TileTask::Vector dependencies_;
  bool did_complete_;
};

}  // namespace cc

#endif  // CC_RASTER_TILE_TASK_H_
