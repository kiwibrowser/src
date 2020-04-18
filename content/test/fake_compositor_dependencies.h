// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_TEST_FAKE_COMPOSITOR_DEPENDENCIES_H_
#define CONTENT_TEST_FAKE_COMPOSITOR_DEPENDENCIES_H_

#include "base/macros.h"
#include "base/single_thread_task_runner.h"
#include "cc/test/test_task_graph_runner.h"
#include "content/renderer/gpu/compositor_dependencies.h"
#include "third_party/blink/public/platform/scheduler/test/fake_renderer_scheduler.h"

namespace content {

class FakeCompositorDependencies : public CompositorDependencies {
 public:
  FakeCompositorDependencies();
  ~FakeCompositorDependencies() override;

  // CompositorDependencies implementation.
  bool IsGpuRasterizationForced() override;
  int GetGpuRasterizationMSAASampleCount() override;
  bool IsLcdTextEnabled() override;
  bool IsZeroCopyEnabled() override;
  bool IsPartialRasterEnabled() override;
  bool IsGpuMemoryBufferCompositorResourcesEnabled() override;
  bool IsElasticOverscrollEnabled() override;
  scoped_refptr<base::SingleThreadTaskRunner>
  GetCompositorMainThreadTaskRunner() override;
  scoped_refptr<base::SingleThreadTaskRunner>
  GetCompositorImplThreadTaskRunner() override;
  blink::scheduler::WebMainThreadScheduler* GetWebMainThreadScheduler()
      override;
  cc::TaskGraphRunner* GetTaskGraphRunner() override;
  bool IsThreadedAnimationEnabled() override;
  bool IsScrollAnimatorEnabled() override;
  std::unique_ptr<cc::UkmRecorderFactory> CreateUkmRecorderFactory() override;

 private:
  cc::TestTaskGraphRunner task_graph_runner_;
  blink::scheduler::FakeRendererScheduler renderer_scheduler_;

  DISALLOW_COPY_AND_ASSIGN(FakeCompositorDependencies);
};

}  // namespace content

#endif  // CONTENT_TEST_FAKE_COMPOSITOR_DEPENDENCIES_H_
