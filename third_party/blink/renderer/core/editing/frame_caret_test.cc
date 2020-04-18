// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/editing/frame_caret.h"

#include "third_party/blink/renderer/core/editing/commands/typing_command.h"
#include "third_party/blink/renderer/core/editing/frame_selection.h"
#include "third_party/blink/renderer/core/editing/selection_template.h"
#include "third_party/blink/renderer/core/editing/testing/editing_test_base.h"
#include "third_party/blink/renderer/core/frame/local_frame_view.h"
#include "third_party/blink/renderer/core/layout/layout_theme.h"
#include "third_party/blink/renderer/core/page/focus_controller.h"
#include "third_party/blink/renderer/platform/layout_test_support.h"
#include "third_party/blink/renderer/platform/scheduler/test/fake_task_runner.h"

namespace blink {

class FrameCaretTest : public EditingTestBase {
 public:
  FrameCaretTest()
      : was_running_layout_test_(LayoutTestSupport::IsRunningLayoutTest()) {
    // The caret blink timer doesn't work if isRunningLayoutTest() because
    // LayoutTheme::caretBlinkInterval() returns 0.
    LayoutTestSupport::SetIsRunningLayoutTest(false);
  }
  ~FrameCaretTest() override {
    LayoutTestSupport::SetIsRunningLayoutTest(was_running_layout_test_);
  }

  static bool ShouldBlinkCaret(const FrameCaret& caret) {
    return caret.ShouldBlinkCaret();
  }

 private:
  const bool was_running_layout_test_;
};

TEST_F(FrameCaretTest, BlinkAfterTyping) {
  FrameCaret& caret = Selection().FrameCaretForTesting();
  scoped_refptr<scheduler::FakeTaskRunner> task_runner =
      base::MakeRefCounted<scheduler::FakeTaskRunner>();
  task_runner->SetTime(0);
  caret.RecreateCaretBlinkTimerForTesting(task_runner.get());
  const double kInterval = 10;
  LayoutTheme::GetTheme().SetCaretBlinkInterval(
      TimeDelta::FromSecondsD(kInterval));
  GetDocument().GetPage()->GetFocusController().SetActive(true);
  GetDocument().GetPage()->GetFocusController().SetFocused(true);
  GetDocument().body()->SetInnerHTMLFromString("<textarea>");
  Element* editor = ToElement(GetDocument().body()->firstChild());
  editor->focus();
  GetDocument().View()->UpdateAllLifecyclePhases();

  EXPECT_TRUE(caret.IsActive());
  EXPECT_FALSE(caret.ShouldShowBlockCursor());
  EXPECT_TRUE(caret.ShouldPaintCaretForTesting())
      << "Initially a caret should be in visible cycle.";

  task_runner->AdvanceTimeAndRun(kInterval);
  EXPECT_FALSE(caret.ShouldPaintCaretForTesting())
      << "The caret blinks normally.";

  TypingCommand::InsertLineBreak(GetDocument());
  GetDocument().View()->UpdateAllLifecyclePhases();
  EXPECT_TRUE(caret.ShouldPaintCaretForTesting())
      << "The caret should be in visible cycle just after a typing command.";

  task_runner->AdvanceTimeAndRun(kInterval - 1);
  GetDocument().View()->UpdateAllLifecyclePhases();
  EXPECT_TRUE(caret.ShouldPaintCaretForTesting())
      << "The typing command reset the timer. The caret is still visible.";

  task_runner->AdvanceTimeAndRun(1);
  GetDocument().View()->UpdateAllLifecyclePhases();
  EXPECT_FALSE(caret.ShouldPaintCaretForTesting())
      << "The caret should blink after the typing command.";
}

TEST_F(FrameCaretTest, ShouldNotBlinkWhenSelectionLooseFocus) {
  FrameCaret& caret = Selection().FrameCaretForTesting();
  GetDocument().GetPage()->GetFocusController().SetActive(true);
  GetDocument().GetPage()->GetFocusController().SetFocused(true);
  GetDocument().body()->SetInnerHTMLFromString(
      "<div id='outer' tabindex='-1'>"
      "<div id='input' contenteditable>foo</div>"
      "</div>");
  Element* input = GetDocument().QuerySelector("#input");
  input->focus();
  Element* outer = GetDocument().QuerySelector("#outer");
  outer->focus();
  GetDocument().View()->UpdateAllLifecyclePhases();
  const SelectionInDOMTree& selection = Selection().GetSelectionInDOMTree();
  EXPECT_EQ(selection.Base(),
            Position(input, PositionAnchorType::kBeforeChildren));
  EXPECT_FALSE(ShouldBlinkCaret(caret));
}

}  // namespace blink
