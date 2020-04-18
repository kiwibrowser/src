// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_RASTER_TASK_CATEGORY_H_
#define CC_RASTER_TASK_CATEGORY_H_

#include <cstdint>

namespace cc {

// This enum provides values for TaskGraph::Node::category, which is a uint16_t.
// We don't use an enum class here, as we want to keep TaskGraph::Node::category
// generic, allowing other consumers to provide their own of values.
enum TaskCategory : uint16_t {
  TASK_CATEGORY_NONCONCURRENT_FOREGROUND,
  TASK_CATEGORY_FOREGROUND,
  TASK_CATEGORY_BACKGROUND,
};

}  // namespace cc

#endif  // CC_RASTER_TASK_CATEGORY_H_
