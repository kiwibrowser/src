// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>
#include <stdint.h>

#include <string>
#include <vector>

#include "base/macros.h"
#include "base/strings/utf_string_conversions.h"
#include "build/build_config.h"
#include "content/browser/renderer_host/render_view_host_impl.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/notification_types.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/public/test/content_browser_test.h"
#include "content/public/test/content_browser_test_utils.h"
#include "content/shell/browser/shell.h"
#include "content/test/accessibility_browser_test_utils.h"
#include "ui/accessibility/ax_node.h"
#include "ui/accessibility/ax_tree.h"

#if defined(OS_WIN)
#include <atlbase.h>
#include <atlcom.h>
#include "base/win/scoped_com_initializer.h"
#include "ui/base/win/atl_module.h"
#endif

// TODO(dmazzoni): Disabled accessibility tests on Win64. crbug.com/179717
#if defined(OS_WIN) && defined(ARCH_CPU_X86_64)
#define MAYBE_TableSpan DISABLED_TableSpan
#else
#define MAYBE_TableSpan TableSpan
#endif

namespace content {

class CrossPlatformAccessibilityBrowserTest : public ContentBrowserTest {
 public:
  CrossPlatformAccessibilityBrowserTest() {}

  // Tell the renderer to send an accessibility tree, then wait for the
  // notification that it's been received.
  const ui::AXTree& GetAXTree(
      ui::AXMode accessibility_mode = ui::kAXModeComplete) {
    AccessibilityNotificationWaiter waiter(shell()->web_contents(),
                                           accessibility_mode,
                                           ax::mojom::Event::kLayoutComplete);
    waiter.WaitForNotification();
    return waiter.GetAXTree();
  }

  // Make sure each node in the tree has a unique id.
  void RecursiveAssertUniqueIds(
      const ui::AXNode* node, base::hash_set<int>* ids) {
    ASSERT_TRUE(ids->find(node->id()) == ids->end());
    ids->insert(node->id());
    for (int i = 0; i < node->child_count(); i++)
      RecursiveAssertUniqueIds(node->ChildAtIndex(i), ids);
  }

  // ContentBrowserTest
  void SetUpOnMainThread() override;
  void TearDownOnMainThread() override;

 protected:
  std::string GetAttr(const ui::AXNode* node,
                      const ax::mojom::StringAttribute attr);
  int GetIntAttr(const ui::AXNode* node, const ax::mojom::IntAttribute attr);
  bool GetBoolAttr(const ui::AXNode* node, const ax::mojom::BoolAttribute attr);

 private:
#if defined(OS_WIN)
  std::unique_ptr<base::win::ScopedCOMInitializer> com_initializer_;
#endif

  DISALLOW_COPY_AND_ASSIGN(CrossPlatformAccessibilityBrowserTest);
};

void CrossPlatformAccessibilityBrowserTest::SetUpOnMainThread() {
#if defined(OS_WIN)
  ui::win::CreateATLModuleIfNeeded();
  com_initializer_.reset(new base::win::ScopedCOMInitializer());
#endif
}

void CrossPlatformAccessibilityBrowserTest::TearDownOnMainThread() {
#if defined(OS_WIN)
  com_initializer_.reset();
#endif
}

// Convenience method to get the value of a particular AXNode
// attribute as a UTF-8 string.
std::string CrossPlatformAccessibilityBrowserTest::GetAttr(
    const ui::AXNode* node,
    const ax::mojom::StringAttribute attr) {
  const ui::AXNodeData& data = node->data();
  for (size_t i = 0; i < data.string_attributes.size(); ++i) {
    if (data.string_attributes[i].first == attr)
      return data.string_attributes[i].second;
  }
  return std::string();
}

// Convenience method to get the value of a particular AXNode
// integer attribute.
int CrossPlatformAccessibilityBrowserTest::GetIntAttr(
    const ui::AXNode* node,
    const ax::mojom::IntAttribute attr) {
  const ui::AXNodeData& data = node->data();
  for (size_t i = 0; i < data.int_attributes.size(); ++i) {
    if (data.int_attributes[i].first == attr)
      return data.int_attributes[i].second;
  }
  return -1;
}

// Convenience method to get the value of a particular AXNode
// boolean attribute.
bool CrossPlatformAccessibilityBrowserTest::GetBoolAttr(
    const ui::AXNode* node,
    const ax::mojom::BoolAttribute attr) {
  const ui::AXNodeData& data = node->data();
  for (size_t i = 0; i < data.bool_attributes.size(); ++i) {
    if (data.bool_attributes[i].first == attr)
      return data.bool_attributes[i].second;
  }
  return false;
}

// Marked flaky per http://crbug.com/101984
IN_PROC_BROWSER_TEST_F(CrossPlatformAccessibilityBrowserTest,
                       DISABLED_WebpageAccessibility) {
  // Create a data url and load it.
  const char url_str[] =
      "data:text/html,"
      "<!doctype html>"
      "<html><head><title>Accessibility Test</title></head>"
      "<body><input type='button' value='push' /><input type='checkbox' />"
      "</body></html>";
  GURL url(url_str);
  NavigateToURL(shell(), url);
  const ui::AXTree& tree = GetAXTree();
  const ui::AXNode* root = tree.root();

  // Check properties of thet tree.
  EXPECT_STREQ(url_str, tree.data().url.c_str());
  EXPECT_STREQ("Accessibility Test", tree.data().title.c_str());
  EXPECT_STREQ("html", tree.data().doctype.c_str());
  EXPECT_STREQ("text/html", tree.data().mimetype.c_str());

  // Check properties of the root element of the tree.
  EXPECT_STREQ("Accessibility Test",
               GetAttr(root, ax::mojom::StringAttribute::kName).c_str());
  EXPECT_EQ(ax::mojom::Role::kRootWebArea, root->data().role);

  // Check properties of the BODY element.
  ASSERT_EQ(1, root->child_count());
  const ui::AXNode* body = root->ChildAtIndex(0);
  EXPECT_EQ(ax::mojom::Role::kGroup, body->data().role);
  EXPECT_STREQ("body",
               GetAttr(body, ax::mojom::StringAttribute::kHtmlTag).c_str());
  EXPECT_STREQ("block",
               GetAttr(body, ax::mojom::StringAttribute::kDisplay).c_str());

  // Check properties of the two children of the BODY element.
  ASSERT_EQ(2, body->child_count());

  const ui::AXNode* button = body->ChildAtIndex(0);
  EXPECT_EQ(ax::mojom::Role::kButton, button->data().role);
  EXPECT_STREQ("input",
               GetAttr(button, ax::mojom::StringAttribute::kHtmlTag).c_str());
  EXPECT_STREQ("push",
               GetAttr(button, ax::mojom::StringAttribute::kName).c_str());
  EXPECT_STREQ("inline-block",
               GetAttr(button, ax::mojom::StringAttribute::kDisplay).c_str());
  ASSERT_EQ(2U, button->data().html_attributes.size());
  EXPECT_STREQ("type", button->data().html_attributes[0].first.c_str());
  EXPECT_STREQ("button", button->data().html_attributes[0].second.c_str());
  EXPECT_STREQ("value", button->data().html_attributes[1].first.c_str());
  EXPECT_STREQ("push", button->data().html_attributes[1].second.c_str());

  const ui::AXNode* checkbox = body->ChildAtIndex(1);
  EXPECT_EQ(ax::mojom::Role::kCheckBox, checkbox->data().role);
  EXPECT_STREQ("input",
               GetAttr(checkbox, ax::mojom::StringAttribute::kHtmlTag).c_str());
  EXPECT_STREQ("inline-block",
               GetAttr(checkbox, ax::mojom::StringAttribute::kDisplay).c_str());
  ASSERT_EQ(1U, checkbox->data().html_attributes.size());
  EXPECT_STREQ("type", checkbox->data().html_attributes[0].first.c_str());
  EXPECT_STREQ("checkbox", checkbox->data().html_attributes[0].second.c_str());
}

IN_PROC_BROWSER_TEST_F(CrossPlatformAccessibilityBrowserTest,
                       UnselectedEditableTextAccessibility) {
  // Create a data url and load it.
  const char url_str[] =
      "data:text/html,"
      "<!doctype html>"
      "<body>"
      "<input value=\"Hello, world.\"/>"
      "</body></html>";
  GURL url(url_str);
  NavigateToURL(shell(), url);

  const ui::AXTree& tree = GetAXTree();
  const ui::AXNode* root = tree.root();
  ASSERT_EQ(1, root->child_count());
  const ui::AXNode* body = root->ChildAtIndex(0);
  ASSERT_EQ(1, body->child_count());
  const ui::AXNode* text = body->ChildAtIndex(0);
  EXPECT_EQ(ax::mojom::Role::kTextField, text->data().role);
  EXPECT_STREQ("input",
               GetAttr(text, ax::mojom::StringAttribute::kHtmlTag).c_str());
  EXPECT_EQ(0, GetIntAttr(text, ax::mojom::IntAttribute::kTextSelStart));
  EXPECT_EQ(0, GetIntAttr(text, ax::mojom::IntAttribute::kTextSelEnd));
  EXPECT_STREQ("Hello, world.",
               GetAttr(text, ax::mojom::StringAttribute::kValue).c_str());

  // TODO(dmazzoni): as soon as more accessibility code is cross-platform,
  // this code should test that the accessible info is dynamically updated
  // if the selection or value changes.
}

IN_PROC_BROWSER_TEST_F(CrossPlatformAccessibilityBrowserTest,
                       SelectedEditableTextAccessibility) {
  // Create a data url and load it.
  const char url_str[] =
      "data:text/html,"
      "<!doctype html>"
      "<body onload=\"document.body.children[0].select();\">"
      "<input value=\"Hello, world.\"/>"
      "</body></html>";
  GURL url(url_str);
  NavigateToURL(shell(), url);

  const ui::AXTree& tree = GetAXTree();
  const ui::AXNode* root = tree.root();
  ASSERT_EQ(1, root->child_count());
  const ui::AXNode* body = root->ChildAtIndex(0);
  ASSERT_EQ(1, body->child_count());
  const ui::AXNode* text = body->ChildAtIndex(0);
  EXPECT_EQ(ax::mojom::Role::kTextField, text->data().role);
  EXPECT_STREQ("input",
               GetAttr(text, ax::mojom::StringAttribute::kHtmlTag).c_str());
  EXPECT_EQ(0, GetIntAttr(text, ax::mojom::IntAttribute::kTextSelStart));
  EXPECT_EQ(13, GetIntAttr(text, ax::mojom::IntAttribute::kTextSelEnd));
  EXPECT_STREQ("Hello, world.",
               GetAttr(text, ax::mojom::StringAttribute::kValue).c_str());
}

IN_PROC_BROWSER_TEST_F(CrossPlatformAccessibilityBrowserTest,
                       MultipleInheritanceAccessibility) {
  // In a WebKit accessibility render tree for a table, each cell is a
  // child of both a row and a column, so it appears to use multiple
  // inheritance. Make sure that the ui::AXNodeDataObject tree only
  // keeps one copy of each cell, and uses an indirect child id for the
  // additional reference to it.
  const char url_str[] =
      "data:text/html,"
      "<!doctype html>"
      "<table border=1><tr><td>1</td><td>2</td></tr></table>";
  GURL url(url_str);
  NavigateToURL(shell(), url);

  const ui::AXTree& tree = GetAXTree();
  const ui::AXNode* root = tree.root();
  ASSERT_EQ(1, root->child_count());
  const ui::AXNode* table = root->ChildAtIndex(0);
  EXPECT_EQ(ax::mojom::Role::kTable, table->data().role);
  const ui::AXNode* row = table->ChildAtIndex(0);
  EXPECT_EQ(ax::mojom::Role::kRow, row->data().role);
  const ui::AXNode* cell1 = row->ChildAtIndex(0);
  EXPECT_EQ(ax::mojom::Role::kCell, cell1->data().role);
  const ui::AXNode* cell2 = row->ChildAtIndex(1);
  EXPECT_EQ(ax::mojom::Role::kCell, cell2->data().role);
  const ui::AXNode* column1 = table->ChildAtIndex(1);
  EXPECT_EQ(ax::mojom::Role::kColumn, column1->data().role);
  EXPECT_EQ(0, column1->child_count());
  EXPECT_EQ(1U, column1->data().intlist_attributes.size());
  EXPECT_EQ(ax::mojom::IntListAttribute::kIndirectChildIds,
            column1->data().intlist_attributes[0].first);
  const std::vector<int32_t> column1_indirect_child_ids =
      column1->data().intlist_attributes[0].second;
  EXPECT_EQ(1U, column1_indirect_child_ids.size());
  EXPECT_EQ(cell1->id(), column1_indirect_child_ids[0]);
  const ui::AXNode* column2 = table->ChildAtIndex(2);
  EXPECT_EQ(ax::mojom::Role::kColumn, column2->data().role);
  EXPECT_EQ(0, column2->child_count());
  EXPECT_EQ(ax::mojom::IntListAttribute::kIndirectChildIds,
            column2->data().intlist_attributes[0].first);
  const std::vector<int32_t> column2_indirect_child_ids =
      column2->data().intlist_attributes[0].second;
  EXPECT_EQ(1U, column2_indirect_child_ids.size());
  EXPECT_EQ(cell2->id(), column2_indirect_child_ids[0]);
}

IN_PROC_BROWSER_TEST_F(CrossPlatformAccessibilityBrowserTest,
                       MultipleInheritanceAccessibility2) {
  // Here's another html snippet where WebKit puts the same node as a child
  // of two different parents. Instead of checking the exact output, just
  // make sure that no id is reused in the resulting tree.
  const char url_str[] =
      "data:text/html,"
      "<!doctype html>"
      "<script>\n"
      "  document.writeln('<q><section></section></q><q><li>');\n"
      "  setTimeout(function() {\n"
      "    document.close();\n"
      "  }, 1);\n"
      "</script>";
  GURL url(url_str);
  NavigateToURL(shell(), url);

  const ui::AXTree& tree = GetAXTree();
  const ui::AXNode* root = tree.root();
  base::hash_set<int> ids;
  RecursiveAssertUniqueIds(root, &ids);
}

// TODO(dmazzoni): Needs to be rebaselined. http://crbug.com/347464
IN_PROC_BROWSER_TEST_F(CrossPlatformAccessibilityBrowserTest,
                       DISABLED_IframeAccessibility) {
  // Create a data url and load it.
  const char url_str[] =
      "data:text/html,"
      "<!doctype html><html><body>"
      "<button>Button 1</button>"
      "<iframe src='data:text/html,"
      "<!doctype html><html><body><button>Button 2</button></body></html>"
      "'></iframe>"
      "<button>Button 3</button>"
      "</body></html>";
  GURL url(url_str);
  NavigateToURL(shell(), url);

  const ui::AXTree& tree = GetAXTree();
  const ui::AXNode* root = tree.root();
  ASSERT_EQ(1, root->child_count());
  const ui::AXNode* body = root->ChildAtIndex(0);
  ASSERT_EQ(3, body->child_count());

  const ui::AXNode* button1 = body->ChildAtIndex(0);
  EXPECT_EQ(ax::mojom::Role::kButton, button1->data().role);
  EXPECT_STREQ("Button 1",
               GetAttr(button1, ax::mojom::StringAttribute::kName).c_str());

  const ui::AXNode* iframe = body->ChildAtIndex(1);
  EXPECT_STREQ("iframe",
               GetAttr(iframe, ax::mojom::StringAttribute::kHtmlTag).c_str());
  ASSERT_EQ(1, iframe->child_count());

  const ui::AXNode* sub_document = iframe->ChildAtIndex(0);
  EXPECT_EQ(ax::mojom::Role::kWebArea, sub_document->data().role);
  ASSERT_EQ(1, sub_document->child_count());

  const ui::AXNode* sub_body = sub_document->ChildAtIndex(0);
  ASSERT_EQ(1, sub_body->child_count());

  const ui::AXNode* button2 = sub_body->ChildAtIndex(0);
  EXPECT_EQ(ax::mojom::Role::kButton, button2->data().role);
  EXPECT_STREQ("Button 2",
               GetAttr(button2, ax::mojom::StringAttribute::kName).c_str());

  const ui::AXNode* button3 = body->ChildAtIndex(2);
  EXPECT_EQ(ax::mojom::Role::kButton, button3->data().role);
  EXPECT_STREQ("Button 3",
               GetAttr(button3, ax::mojom::StringAttribute::kName).c_str());
}

IN_PROC_BROWSER_TEST_F(CrossPlatformAccessibilityBrowserTest,
                       DuplicateChildrenAccessibility) {
  // Here's another html snippet where WebKit has a parent node containing
  // two duplicate child nodes. Instead of checking the exact output, just
  // make sure that no id is reused in the resulting tree.
  const char url_str[] =
      "data:text/html,"
      "<!doctype html>"
      "<em><code ><h4 ></em>";
  GURL url(url_str);
  NavigateToURL(shell(), url);

  const ui::AXTree& tree = GetAXTree();
  const ui::AXNode* root = tree.root();
  base::hash_set<int> ids;
  RecursiveAssertUniqueIds(root, &ids);
}

IN_PROC_BROWSER_TEST_F(CrossPlatformAccessibilityBrowserTest,
                       MAYBE_TableSpan) {
  // +---+---+---+
  // |   1   | 2 |
  // +---+---+---+
  // | 3 |   4   |
  // +---+---+---+

  const char url_str[] =
      "data:text/html,"
      "<!doctype html>"
      "<table border=1>"
      " <tr>"
      "  <td colspan=2>1</td><td>2</td>"
      " </tr>"
      " <tr>"
      "  <td>3</td><td colspan=2>4</td>"
      " </tr>"
      "</table>";
  GURL url(url_str);
  NavigateToURL(shell(), url);

  const ui::AXTree& tree = GetAXTree();
  const ui::AXNode* root = tree.root();
  const ui::AXNode* table = root->ChildAtIndex(0);
  EXPECT_EQ(ax::mojom::Role::kTable, table->data().role);
  ASSERT_GE(table->child_count(), 5);
  EXPECT_EQ(ax::mojom::Role::kRow, table->ChildAtIndex(0)->data().role);
  EXPECT_EQ(ax::mojom::Role::kRow, table->ChildAtIndex(1)->data().role);
  EXPECT_EQ(ax::mojom::Role::kColumn, table->ChildAtIndex(2)->data().role);
  EXPECT_EQ(ax::mojom::Role::kColumn, table->ChildAtIndex(3)->data().role);
  EXPECT_EQ(ax::mojom::Role::kColumn, table->ChildAtIndex(4)->data().role);
  EXPECT_EQ(3, GetIntAttr(table, ax::mojom::IntAttribute::kTableColumnCount));
  EXPECT_EQ(2, GetIntAttr(table, ax::mojom::IntAttribute::kTableRowCount));

  const ui::AXNode* cell1 = table->ChildAtIndex(0)->ChildAtIndex(0);
  const ui::AXNode* cell2 = table->ChildAtIndex(0)->ChildAtIndex(1);
  const ui::AXNode* cell3 = table->ChildAtIndex(1)->ChildAtIndex(0);
  const ui::AXNode* cell4 = table->ChildAtIndex(1)->ChildAtIndex(1);

  EXPECT_EQ(0,
            GetIntAttr(cell1, ax::mojom::IntAttribute::kTableCellColumnIndex));
  EXPECT_EQ(0, GetIntAttr(cell1, ax::mojom::IntAttribute::kTableCellRowIndex));
  EXPECT_EQ(2,
            GetIntAttr(cell1, ax::mojom::IntAttribute::kTableCellColumnSpan));
  EXPECT_EQ(1, GetIntAttr(cell1, ax::mojom::IntAttribute::kTableCellRowSpan));
  EXPECT_EQ(2,
            GetIntAttr(cell2, ax::mojom::IntAttribute::kTableCellColumnIndex));
  EXPECT_EQ(1,
            GetIntAttr(cell2, ax::mojom::IntAttribute::kTableCellColumnSpan));
  EXPECT_EQ(0,
            GetIntAttr(cell3, ax::mojom::IntAttribute::kTableCellColumnIndex));
  EXPECT_EQ(1,
            GetIntAttr(cell3, ax::mojom::IntAttribute::kTableCellColumnSpan));
  EXPECT_EQ(1,
            GetIntAttr(cell4, ax::mojom::IntAttribute::kTableCellColumnIndex));
  EXPECT_EQ(2,
            GetIntAttr(cell4, ax::mojom::IntAttribute::kTableCellColumnSpan));
}

IN_PROC_BROWSER_TEST_F(CrossPlatformAccessibilityBrowserTest,
                       WritableElement) {
  const char url_str[] =
      "data:text/html,"
      "<!doctype html>"
      "<div role='textbox' tabindex=0>"
      " Some text"
      "</div>";
  GURL url(url_str);
  NavigateToURL(shell(), url);
  const ui::AXTree& tree = GetAXTree();
  const ui::AXNode* root = tree.root();
  ASSERT_EQ(1, root->child_count());
  const ui::AXNode* textbox = root->ChildAtIndex(0);
  EXPECT_TRUE(textbox->data().HasAction(ax::mojom::Action::kSetValue));
}

}  // namespace content
