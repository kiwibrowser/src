// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/canvas/htmlcanvas/html_canvas_element_module.h"

#include <memory>
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/dom_node_ids.h"
#include "third_party/blink/renderer/core/frame/local_frame_view.h"
#include "third_party/blink/renderer/core/html/canvas/html_canvas_element.h"
#include "third_party/blink/renderer/core/offscreencanvas/offscreen_canvas.h"
#include "third_party/blink/renderer/core/testing/page_test_base.h"

namespace blink {

class HTMLCanvasElementModuleTest : public PageTestBase {
 protected:
  void SetUp() override {
    PageTestBase::SetUp();
    SetHtmlInnerHTML("<body><canvas id='c'></canvas></body>");
    canvas_element_ = ToHTMLCanvasElement(GetElementById("c"));
  }

  HTMLCanvasElement& CanvasElement() const { return *canvas_element_; }
  OffscreenCanvas* TransferControlToOffscreen(ExceptionState&);

 private:
  Persistent<HTMLCanvasElement> canvas_element_;
};

OffscreenCanvas* HTMLCanvasElementModuleTest::TransferControlToOffscreen(
    ExceptionState& exception_state) {
  // This unit test only tests if the Canvas Id is associated correctly, so we
  // exclude the part that creates surface layer bridge because a mojo message
  // pipe cannot be tested using webkit unit tests.
  return HTMLCanvasElementModule::TransferControlToOffscreenInternal(
      CanvasElement(), exception_state);
}

TEST_F(HTMLCanvasElementModuleTest, TransferControlToOffscreen) {
  NonThrowableExceptionState exception_state;
  OffscreenCanvas* offscreen_canvas =
      TransferControlToOffscreen(exception_state);
  DOMNodeId canvas_id = offscreen_canvas->PlaceholderCanvasId();
  EXPECT_EQ(canvas_id, DOMNodeIds::IdForNode(&(CanvasElement())));
}

}  // namespace blink
