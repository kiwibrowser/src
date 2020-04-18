// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/accessibility/accessibility_tree_formatter_browser.h"


namespace content {

std::unique_ptr<base::DictionaryValue>
AccessibilityTreeFormatterBrowser::BuildAccessibilityTree(
    BrowserAccessibility* root) {
  CHECK(root);
  std::unique_ptr<base::DictionaryValue> dict(new base::DictionaryValue);
  RecursiveBuildAccessibilityTree(*root, dict.get());
  return dict;
}

std::unique_ptr<base::DictionaryValue>
AccessibilityTreeFormatterBrowser::BuildAccessibilityTreeForProcess(
    base::ProcessId pid) {
  NOTREACHED();
  return nullptr;
}

std::unique_ptr<base::DictionaryValue>
AccessibilityTreeFormatterBrowser::BuildAccessibilityTreeForWindow(
    gfx::AcceleratedWidget widget) {
  NOTREACHED();
  return nullptr;
}

void AccessibilityTreeFormatterBrowser::RecursiveBuildAccessibilityTree(
    const BrowserAccessibility& node,
    base::DictionaryValue* dict) {
  AddProperties(node, dict);

  auto children = std::make_unique<base::ListValue>();

  for (size_t i = 0; i < ChildCount(node); ++i) {
    BrowserAccessibility* child_node = GetChild(node, i);
    std::unique_ptr<base::DictionaryValue> child_dict(
        new base::DictionaryValue);
    RecursiveBuildAccessibilityTree(*child_node, child_dict.get());
    children->Append(std::move(child_dict));
  }
  dict->Set(kChildrenDictAttr, std::move(children));
}

uint32_t AccessibilityTreeFormatterBrowser::ChildCount(
    const BrowserAccessibility& node) const {
  return node.PlatformChildCount();
}

BrowserAccessibility* AccessibilityTreeFormatterBrowser::GetChild(
    const BrowserAccessibility& node,
    uint32_t i) const {
  return node.PlatformGetChild(i);
}

}  // namespace content
