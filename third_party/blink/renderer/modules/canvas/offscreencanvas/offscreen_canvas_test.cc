// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/mojom/page/page_visibility_state.mojom-blink.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/frame/local_frame_view.h"
#include "third_party/blink/renderer/core/frame/settings.h"
#include "third_party/blink/renderer/core/html/canvas/html_canvas_element.h"
#include "third_party/blink/renderer/core/offscreencanvas/offscreen_canvas.h"
#include "third_party/blink/renderer/core/testing/page_test_base.h"
#include "third_party/blink/renderer/modules/canvas/htmlcanvas/html_canvas_element_module.h"
#include "third_party/blink/renderer/modules/canvas/offscreencanvas2d/offscreen_canvas_rendering_context_2d.h"
#include "third_party/blink/renderer/platform/graphics/gpu/shared_gpu_context.h"
#include "third_party/blink/renderer/platform/graphics/test/fake_gles2_interface.h"
#include "third_party/blink/renderer/platform/graphics/test/fake_web_graphics_context_3d_provider.h"
#include "third_party/blink/renderer/platform/testing/testing_platform_support.h"

using testing::Mock;

namespace blink {

class OffscreenCanvasTest : public PageTestBase {
 protected:
  OffscreenCanvasTest();
  void SetUp() override;
  void TearDown() override;

  HTMLCanvasElement& CanvasElement() const { return *canvas_element_; }
  OffscreenCanvas& OSCanvas() const { return *offscreen_canvas_; }
  OffscreenCanvasFrameDispatcher* Dispatcher() const {
    return offscreen_canvas_->GetOrCreateFrameDispatcher();
  }
  OffscreenCanvasRenderingContext2D& Context() const { return *context_; }
  ScriptState* GetScriptState() const {
    return ToScriptStateForMainWorld(GetDocument().GetFrame());
  }
  ScopedTestingPlatformSupport<TestingPlatformSupport>& platform() {
    return platform_;
  }

 private:
  Persistent<HTMLCanvasElement> canvas_element_;
  Persistent<OffscreenCanvas> offscreen_canvas_;
  Persistent<OffscreenCanvasRenderingContext2D> context_;
  ScopedTestingPlatformSupport<TestingPlatformSupport> platform_;
  FakeGLES2Interface gl_;
};

OffscreenCanvasTest::OffscreenCanvasTest() = default;

void OffscreenCanvasTest::SetUp() {
  auto factory = [](FakeGLES2Interface* gl, bool* gpu_compositing_disabled)
      -> std::unique_ptr<WebGraphicsContext3DProvider> {
    *gpu_compositing_disabled = false;
    gl->SetIsContextLost(false);
    return std::make_unique<FakeWebGraphicsContext3DProvider>(gl);
  };
  SharedGpuContext::SetContextProviderFactoryForTesting(
      WTF::BindRepeating(factory, WTF::Unretained(&gl_)));
  PageTestBase::SetUp();
  SetHtmlInnerHTML("<body><canvas id='c'></canvas></body>");
  canvas_element_ = ToHTMLCanvasElement(GetElementById("c"));
  DummyExceptionStateForTesting exception_state;
  offscreen_canvas_ = HTMLCanvasElementModule::transferControlToOffscreen(
      *canvas_element_, exception_state);
  CanvasContextCreationAttributesCore attrs;
  context_ = static_cast<OffscreenCanvasRenderingContext2D*>(
      offscreen_canvas_->GetCanvasRenderingContext(&GetDocument(), String("2d"),
                                                   attrs));
}

void OffscreenCanvasTest::TearDown() {
  SharedGpuContext::ResetForTesting();
}

TEST_F(OffscreenCanvasTest, AnimationNotInitiallySuspended) {
  ScriptState::Scope scope(GetScriptState());
  EXPECT_FALSE(Dispatcher()->IsAnimationSuspended());
}

}  // namespace blink
