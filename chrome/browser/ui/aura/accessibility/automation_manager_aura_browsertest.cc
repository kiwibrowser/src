// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/aura/accessibility/automation_manager_aura.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/browser_accessibility_state.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test_utils.h"
#include "ui/accessibility/ax_node_data.h"

namespace {

// Given an AXTreeSourceAura and a node within that tree, recursively search
// for all nodes who have a child tree id of |target_ax_tree_id|, meaning
// that they're a parent of a particular web contents.
void FindAllHostsOfWebContentsWithAXTreeID(
    AXTreeSourceAura* tree,
    views::AXAuraObjWrapper* node,
    int target_ax_tree_id,
    std::vector<views::AXAuraObjWrapper*>* web_hosts) {
  ui::AXNodeData node_data;
  tree->SerializeNode(node, &node_data);
  if (node_data.GetIntAttribute(ax::mojom::IntAttribute::kChildTreeId) ==
      target_ax_tree_id) {
    web_hosts->push_back(node);
  }

  std::vector<views::AXAuraObjWrapper*> children;
  tree->GetChildren(node, &children);
  for (auto* child : children) {
    FindAllHostsOfWebContentsWithAXTreeID(tree, child, target_ax_tree_id,
                                          web_hosts);
  }
}

}  // namespace

typedef InProcessBrowserTest AutomationManagerAuraBrowserTest;

// A WebContents can be "hooked up" to the Chrome OS Desktop accessibility
// tree two different ways: via its aura::Window, and via a views::WebView.
// This test makes sure that we don't hook up both simultaneously, leading
// to the same web page appearing in the overall tree twice.
IN_PROC_BROWSER_TEST_F(AutomationManagerAuraBrowserTest, WebAppearsOnce) {
  content::BrowserAccessibilityState::GetInstance()->EnableAccessibility();

  AutomationManagerAura* manager = AutomationManagerAura::GetInstance();
  manager->Enable(browser()->profile());
  auto* tree = manager->current_tree_.get();

  ui_test_utils::NavigateToURL(
      browser(),
      GURL("data:text/html;charset=utf-8,<button autofocus>Click me</button>"));

  auto* web_contents = browser()->tab_strip_model()->GetActiveWebContents();

  WaitForAccessibilityTreeToContainNodeWithName(web_contents, "Click me");

  auto* frame_host = web_contents->GetMainFrame();
  int ax_tree_id = frame_host->GetAXTreeID();
  ASSERT_GT(ax_tree_id, 0);

  std::vector<views::AXAuraObjWrapper*> web_hosts;
  FindAllHostsOfWebContentsWithAXTreeID(tree, tree->GetRoot(), ax_tree_id,
                                        &web_hosts);

  EXPECT_EQ(1U, web_hosts.size());
  if (web_hosts.size() == 1) {
    ui::AXNodeData node_data;
    tree->SerializeNode(web_hosts[0], &node_data);
    EXPECT_EQ(ax::mojom::Role::kWebView, node_data.role);
  } else {
    for (size_t i = 0; i < web_hosts.size(); i++) {
      ui::AXNodeData node_data;
      tree->SerializeNode(web_hosts[i], &node_data);
      LOG(ERROR) << i << ": " << node_data.ToString();
    }
  }
}
