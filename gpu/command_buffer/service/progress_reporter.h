// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GPU_COMMAND_BUFFER_SERVICE_PROGRESS_REPORTER_H_
#define GPU_COMMAND_BUFFER_SERVICE_PROGRESS_REPORTER_H_

#include "gpu/gpu_gles2_export.h"

namespace gpu {
namespace gles2 {

// ProgressReporter is used by ContextGroup to report when it is making forward
// progress in execution, delaying activation of the watchdog timeout.
class GPU_GLES2_EXPORT ProgressReporter {
 public:
  virtual ~ProgressReporter() = default;

  virtual void ReportProgress() = 0;
};

}  // namespace gles2
}  // namespace gpu

#endif  // GPU_COMMAND_BUFFER_SERVICE_PROGRESS_REPORTER_H_
