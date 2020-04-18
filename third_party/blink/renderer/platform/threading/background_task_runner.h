// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_THREADING_BACKGROUND_TASK_RUNNER_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_THREADING_BACKGROUND_TASK_RUNNER_H_

#include "base/location.h"
#include "third_party/blink/renderer/platform/platform_export.h"
#include "third_party/blink/renderer/platform/wtf/functional.h"

namespace blink {

namespace BackgroundTaskRunner {

PLATFORM_EXPORT void PostOnBackgroundThread(const base::Location&,
                                            CrossThreadClosure);

}  // BackgroundTaskRunner

}  // namespace blink

#endif
