// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/bindings/core/v8/script_controller.h"
#include "third_party/blink/renderer/core/testing/core_unit_test_helper.h"
#include "third_party/blink/renderer/platform/testing/histogram_tester.h"

namespace blink {

enum PassiveForcedListenerResultType {
  kPreventDefaultNotCalled,
  kDocumentLevelTouchPreventDefaultCalled
};

class EventTargetTest : public RenderingTest {
 public:
  EventTargetTest() = default;
  ~EventTargetTest() override = default;
};

TEST_F(EventTargetTest, PreventDefaultNotCalled) {
  GetDocument().GetSettings()->SetScriptEnabled(true);
  HistogramTester histogram_tester;
  GetDocument().GetFrame()->GetScriptController().ExecuteScriptInMainWorld(
      "window.addEventListener('touchstart', function(e) {}, {});"
      "window.dispatchEvent(new TouchEvent('touchstart', {cancelable: "
      "false}));");

  histogram_tester.ExpectTotalCount("Event.PassiveForcedEventDispatchCancelled",
                                    1);
  histogram_tester.ExpectUniqueSample(
      "Event.PassiveForcedEventDispatchCancelled", kPreventDefaultNotCalled, 1);
}

TEST_F(EventTargetTest, PreventDefaultCalled) {
  GetDocument().GetSettings()->SetScriptEnabled(true);
  HistogramTester histogram_tester;
  GetDocument().GetFrame()->GetScriptController().ExecuteScriptInMainWorld(
      "window.addEventListener('touchstart', function(e) "
      "{e.preventDefault();}, {});"
      "window.dispatchEvent(new TouchEvent('touchstart', {cancelable: "
      "false}));");

  histogram_tester.ExpectTotalCount("Event.PassiveForcedEventDispatchCancelled",
                                    1);
  histogram_tester.ExpectUniqueSample(
      "Event.PassiveForcedEventDispatchCancelled",
      kDocumentLevelTouchPreventDefaultCalled, 1);
}

}  // namespace blink
