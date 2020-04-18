// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_SHELL_RENDERER_LAYOUT_TEST_LAYOUT_TEST_RENDER_FRAME_OBSERVER_H_
#define CONTENT_SHELL_RENDERER_LAYOUT_TEST_LAYOUT_TEST_RENDER_FRAME_OBSERVER_H_

#include "base/macros.h"
#include "content/public/renderer/render_frame_observer.h"
#include "content/shell/common/layout_test.mojom.h"
#include "mojo/public/cpp/bindings/associated_binding.h"

namespace content {

class LayoutTestRenderFrameObserver : public RenderFrameObserver,
                                      public mojom::LayoutTestControl {
 public:
  explicit LayoutTestRenderFrameObserver(RenderFrame* render_frame);
  ~LayoutTestRenderFrameObserver() override;

 private:
  // RenderFrameObserver implementation.
  void OnDestruct() override;

  void CaptureDump(CaptureDumpCallback callback) override;
  void CompositeWithRaster(CompositeWithRasterCallback callback) override;
  void DumpFrameLayout(DumpFrameLayoutCallback callback) override;
  void SetTestConfiguration(mojom::ShellTestConfigurationPtr config) override;
  void ReplicateTestConfiguration(
      mojom::ShellTestConfigurationPtr config) override;
  void SetupSecondaryRenderer() override;
  void BindRequest(mojom::LayoutTestControlAssociatedRequest request);

  mojo::AssociatedBinding<mojom::LayoutTestControl> binding_;
  DISALLOW_COPY_AND_ASSIGN(LayoutTestRenderFrameObserver);
};

}  // namespace content

#endif  // CONTENT_SHELL_RENDERER_LAYOUT_TEST_LAYOUT_TEST_RENDER_FRAME_OBSERVER_H_
