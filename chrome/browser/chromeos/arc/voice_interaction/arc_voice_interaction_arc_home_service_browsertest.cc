// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/arc/voice_interaction/arc_voice_interaction_arc_home_service.h"

#include <string>
#include <vector>

#include "base/callback.h"
#include "base/macros.h"
#include "base/run_loop.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/interactive_test_utils.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/test_utils.h"
#include "ui/accessibility/ax_assistant_structure.h"
#include "ui/accessibility/ax_tree_update.h"

namespace arc {

namespace {

class AXTreeSnapshotWaiter {
 public:
  AXTreeSnapshotWaiter() = default;

  void Wait() { loop_.Run(); }

  const ui::AXTreeUpdate& snapshot() const { return snapshot_; }

  void ReceiveSnapshot(const ui::AXTreeUpdate& snapshot) {
    snapshot_ = snapshot;
    loop_.Quit();
  }

 private:
  ui::AXTreeUpdate snapshot_;
  base::RunLoop loop_;

  DISALLOW_COPY_AND_ASSIGN(AXTreeSnapshotWaiter);
};

}  // namespace

class ArcVoiceInteractionArcHomeServiceTest : public InProcessBrowserTest {
 public:
  ArcVoiceInteractionArcHomeServiceTest() = default;
  ~ArcVoiceInteractionArcHomeServiceTest() override = default;

 protected:
  mojom::VoiceInteractionStructurePtr GetVoiceInteractionStructure(
      const std::string& html) {
    GURL url("data:text/html," + html);
    ui_test_utils::NavigateToURL(browser(), url);
    auto* web_contents = browser()->tab_strip_model()->GetActiveWebContents();
    AXTreeSnapshotWaiter waiter;
    web_contents->RequestAXTreeSnapshot(
        base::BindOnce(&AXTreeSnapshotWaiter::ReceiveSnapshot,
                       base::Unretained(&waiter)),
        ui::kAXModeComplete);
    waiter.Wait();
    std::unique_ptr<ui::AssistantTree> tree =
        ui::CreateAssistantTree(waiter.snapshot(), false);

    return ArcVoiceInteractionArcHomeService::
        CreateVoiceInteractionStructureForTesting(*tree, *tree->nodes.front());
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(ArcVoiceInteractionArcHomeServiceTest);
};

IN_PROC_BROWSER_TEST_F(ArcVoiceInteractionArcHomeServiceTest,
                       VoiceInteractionStructurePositionTest) {
  auto result = GetVoiceInteractionStructure(
      "<div style='position:absolute;width:200px;height:200px'>"
      "<p style='position:absolute;top:20px;left:20px;margin:0'>Hello</p>"
      "</div>");
  ASSERT_FALSE(result.is_null());
  ASSERT_EQ(result->children.size(), 1ul);
  ASSERT_EQ(result->children[0]->children.size(), 1ul);

  auto& child = result->children[0]->children[0];
  ASSERT_EQ(base::UTF16ToUTF8(child->text), "Hello");
  ASSERT_EQ(child->rect.x(), 20);
  ASSERT_EQ(child->rect.y(), 20);
}

IN_PROC_BROWSER_TEST_F(ArcVoiceInteractionArcHomeServiceTest,
                       VoiceInteractionStructureInputSelectionTest) {
  auto result = GetVoiceInteractionStructure(
      "<html>"
      "  <body>"
      "    <input id='input' value='Hello, world'/>"
      "    <script type='text/javascript'>"
      "      var input = document.getElementById('input');"
      "      input.select();"
      "      input.selectionStart = 0;"
      "      input.selectionEnd = 5;"
      "    </script>"
      "  </body>"
      "</html>");
  auto& content_root = result->children[0];
  ASSERT_EQ(content_root->children.size(), 1ul);
  auto& child = content_root->children[0];
  ASSERT_FALSE(child.is_null());
  ASSERT_EQ(base::UTF16ToUTF8(child->text), "Hello, world");
  EXPECT_EQ(gfx::Range(0, 5), child->selection);
}

IN_PROC_BROWSER_TEST_F(ArcVoiceInteractionArcHomeServiceTest,
                       VoiceInteractionStructureSectionTest) {
  // Help ensure accessibility states are tested correctly.
  // When the states are not tested correctly (bit shifted), the div appears to
  // be focusable, causing the child text to be aggregated for the div.
  auto result = GetVoiceInteractionStructure(
      "<div role='section' aria-expanded='false'>"
      "Hello<img>"
      "</div>");
  ASSERT_FALSE(result.is_null());

  auto& child = result->children[0];
  ASSERT_EQ(base::UTF16ToUTF8(child->text), "");
}

IN_PROC_BROWSER_TEST_F(ArcVoiceInteractionArcHomeServiceTest,
                       VoiceInteractionStructureSelectTest) {
  // Help ensure accessibility states are tested correctly.
  // When the states are not tested correctly (bit shifted), the option appears
  // to have ax::mojom::State::kProtected, and text is incorrectly set as
  // password dots.
  auto result = GetVoiceInteractionStructure(
      "<div><select><option>1</option></select></div>");
  ASSERT_FALSE(result.is_null());

  auto& child = result->children[0];
  ASSERT_EQ(base::UTF16ToUTF8(child->text), "");
  ASSERT_EQ(base::UTF16ToUTF8(child->children[0]->text), "1");
}

IN_PROC_BROWSER_TEST_F(ArcVoiceInteractionArcHomeServiceTest,
                       VoiceInteractionStructureMultipleSelectionTest) {
  auto result = GetVoiceInteractionStructure(
      "<html>"
      "  <body>"
      "    <b id='node1'>foo</b><b>middle</b><b id='node2'>bar</b>"
      "    <script type='text/javascript'>"
      "      var element1 = document.getElementById('node1');"
      "      var node1 = element1.childNodes.item(0);"
      "      var range=document.createRange();"
      "      range.setStart(node1, 1);"
      "      var element2 = document.getElementById('node2');"
      "      var node2 = element2.childNodes.item(0);"
      "      range.setEnd(node2, 1);"
      "      var selection=window.getSelection();"
      "      selection.removeAllRanges();"
      "      selection.addRange(range);"
      "    </script>"
      "  </body>"
      "</html>");
  ASSERT_EQ(result->children.size(), 1ul);
  auto& content_root = result->children[0];
  ASSERT_EQ(content_root->children.size(), 3ul);

  auto& grand_child1 = content_root->children[0];
  EXPECT_EQ(gfx::Range(1, 3), grand_child1->selection);

  auto& grand_child2 = content_root->children[1];
  EXPECT_EQ(gfx::Range(0, 6), grand_child2->selection);

  auto& grand_child3 = content_root->children[2];
  EXPECT_EQ(gfx::Range(0, 1), grand_child3->selection);
}

}  // namespace arc
