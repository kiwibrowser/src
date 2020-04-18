// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_TRACING_BACKGROUND_MEMORY_TRACING_OBSERVER_H_
#define CONTENT_BROWSER_TRACING_BACKGROUND_MEMORY_TRACING_OBSERVER_H_

#include "content/browser/tracing/background_tracing_manager_impl.h"

namespace content {

class CONTENT_EXPORT BackgroundMemoryTracingObserver
    : public BackgroundTracingManagerImpl::EnabledStateObserver {
 public:
  static BackgroundMemoryTracingObserver* GetInstance();

  void OnScenarioActivated(const BackgroundTracingConfigImpl* config) override;
  void OnScenarioAborted() override;
  void OnTracingEnabled(
      BackgroundTracingConfigImpl::CategoryPreset preset) override;

  bool heap_profiling_enabled_for_testing() const {
    return heap_profiling_enabled_;
  }

 private:
  BackgroundMemoryTracingObserver();
  ~BackgroundMemoryTracingObserver() override;

  bool heap_profiling_enabled_ = false;

  DISALLOW_COPY_AND_ASSIGN(BackgroundMemoryTracingObserver);
};

}  // namespace content

#endif  // CONTENT_BROWSER_TRACING_BACKGROUND_MEMORY_TRACING_OBSERVER_H_
