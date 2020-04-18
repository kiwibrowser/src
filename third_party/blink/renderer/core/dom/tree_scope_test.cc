// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/dom/tree_scope.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/element.h"
#include "third_party/blink/renderer/core/dom/shadow_root.h"

namespace blink {

TEST(TreeScopeTest, CommonAncestorOfSameTrees) {
  Document* document = Document::CreateForTest();
  EXPECT_EQ(document, document->CommonAncestorTreeScope(*document));

  Element* html = document->CreateRawElement(HTMLNames::htmlTag);
  document->AppendChild(html);
  ShadowRoot& shadow_root = html->CreateV0ShadowRootForTesting();
  EXPECT_EQ(shadow_root, shadow_root.CommonAncestorTreeScope(shadow_root));
}

TEST(TreeScopeTest, CommonAncestorOfInclusiveTrees) {
  //  document
  //     |      : Common ancestor is document.
  // shadowRoot

  Document* document = Document::CreateForTest();
  Element* html = document->CreateRawElement(HTMLNames::htmlTag);
  document->AppendChild(html);
  ShadowRoot& shadow_root = html->CreateV0ShadowRootForTesting();

  EXPECT_EQ(document, document->CommonAncestorTreeScope(shadow_root));
  EXPECT_EQ(document, shadow_root.CommonAncestorTreeScope(*document));
}

TEST(TreeScopeTest, CommonAncestorOfSiblingTrees) {
  //  document
  //   /    \  : Common ancestor is document.
  //  A      B

  Document* document = Document::CreateForTest();
  Element* html = document->CreateRawElement(HTMLNames::htmlTag);
  document->AppendChild(html);
  Element* head = document->CreateRawElement(HTMLNames::headTag);
  html->AppendChild(head);
  Element* body = document->CreateRawElement(HTMLNames::bodyTag);
  html->AppendChild(body);

  ShadowRoot& shadow_root_a = head->CreateV0ShadowRootForTesting();
  ShadowRoot& shadow_root_b = body->CreateV0ShadowRootForTesting();

  EXPECT_EQ(document, shadow_root_a.CommonAncestorTreeScope(shadow_root_b));
  EXPECT_EQ(document, shadow_root_b.CommonAncestorTreeScope(shadow_root_a));
}

TEST(TreeScopeTest, CommonAncestorOfTreesAtDifferentDepths) {
  //  document
  //    / \    : Common ancestor is document.
  //   Y   B
  //  /
  // A

  Document* document = Document::CreateForTest();
  Element* html = document->CreateRawElement(HTMLNames::htmlTag);
  document->AppendChild(html);
  Element* head = document->CreateRawElement(HTMLNames::headTag);
  html->AppendChild(head);
  Element* body = document->CreateRawElement(HTMLNames::bodyTag);
  html->AppendChild(body);

  ShadowRoot& shadow_root_y = head->CreateV0ShadowRootForTesting();
  ShadowRoot& shadow_root_b = body->CreateV0ShadowRootForTesting();

  Element* div_in_y = document->CreateRawElement(HTMLNames::divTag);
  shadow_root_y.AppendChild(div_in_y);
  ShadowRoot& shadow_root_a = div_in_y->CreateV0ShadowRootForTesting();

  EXPECT_EQ(document, shadow_root_a.CommonAncestorTreeScope(shadow_root_b));
  EXPECT_EQ(document, shadow_root_b.CommonAncestorTreeScope(shadow_root_a));
}

TEST(TreeScopeTest, CommonAncestorOfTreesInDifferentDocuments) {
  Document* document1 = Document::CreateForTest();
  Document* document2 = Document::CreateForTest();
  EXPECT_EQ(nullptr, document1->CommonAncestorTreeScope(*document2));
  EXPECT_EQ(nullptr, document2->CommonAncestorTreeScope(*document1));
}

}  // namespace blink
