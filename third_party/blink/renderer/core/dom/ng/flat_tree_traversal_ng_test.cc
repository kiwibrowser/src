// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/dom/ng/flat_tree_traversal_ng.h"

#include <memory>
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/bindings/core/v8/exception_state.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/element.h"
#include "third_party/blink/renderer/core/dom/node.h"
#include "third_party/blink/renderer/core/dom/node_traversal.h"
#include "third_party/blink/renderer/core/dom/shadow_root.h"
#include "third_party/blink/renderer/core/frame/local_frame_view.h"
#include "third_party/blink/renderer/core/html/html_element.h"
#include "third_party/blink/renderer/core/testing/page_test_base.h"
#include "third_party/blink/renderer/platform/geometry/int_size.h"
#include "third_party/blink/renderer/platform/runtime_enabled_features.h"
#include "third_party/blink/renderer/platform/testing/runtime_enabled_features_test_helpers.h"
#include "third_party/blink/renderer/platform/wtf/compiler.h"
#include "third_party/blink/renderer/platform/wtf/std_lib_extras.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"

namespace blink {
// To avoid symbol collisions in jumbo builds.
namespace flat_tree_traversal_ng_test {

class FlatTreeTraversalNgTest : public PageTestBase,
                                private ScopedSlotInFlatTreeForTest,
                                ScopedIncrementalShadowDOMForTest {
 public:
  FlatTreeTraversalNgTest()
      : ScopedSlotInFlatTreeForTest(true),
        ScopedIncrementalShadowDOMForTest(true) {}

 protected:
  // Sets |mainHTML| to BODY element with |innerHTML| property and attaches
  // shadow root to child with |shadowHTML|, then update distribution for
  // calling member functions in |FlatTreeTraversalNg|.
  void SetupSampleHTML(const char* main_html,
                       const char* shadow_html,
                       unsigned);

  void SetupDocumentTree(const char* main_html);

  void AttachV0ShadowRoot(Element& shadow_host, const char* shadow_inner_html);
  void AttachOpenShadowRoot(Element& shadow_host,
                            const char* shadow_inner_html);
};

void FlatTreeTraversalNgTest::SetupSampleHTML(const char* main_html,
                                              const char* shadow_html,
                                              unsigned index) {
  Element* body = GetDocument().body();
  body->SetInnerHTMLFromString(String::FromUTF8(main_html));
  Element* shadow_host = ToElement(NodeTraversal::ChildAt(*body, index));
  ShadowRoot& shadow_root = shadow_host->CreateV0ShadowRootForTesting();
  shadow_root.SetInnerHTMLFromString(String::FromUTF8(shadow_html));
  body->UpdateDistributionForFlatTreeTraversal();
}

void FlatTreeTraversalNgTest::SetupDocumentTree(const char* main_html) {
  Element* body = GetDocument().body();
  body->SetInnerHTMLFromString(String::FromUTF8(main_html));
}

void FlatTreeTraversalNgTest::AttachV0ShadowRoot(
    Element& shadow_host,
    const char* shadow_inner_html) {
  ShadowRoot& shadow_root = shadow_host.CreateV0ShadowRootForTesting();
  shadow_root.SetInnerHTMLFromString(String::FromUTF8(shadow_inner_html));
  GetDocument().body()->UpdateDistributionForFlatTreeTraversal();
}

void FlatTreeTraversalNgTest::AttachOpenShadowRoot(
    Element& shadow_host,
    const char* shadow_inner_html) {
  ShadowRoot& shadow_root =
      shadow_host.AttachShadowRootInternal(ShadowRootType::kOpen);
  shadow_root.SetInnerHTMLFromString(String::FromUTF8(shadow_inner_html));
  GetDocument().body()->UpdateDistributionForFlatTreeTraversal();
}

namespace {

void TestCommonAncestor(Node* expected_result,
                        const Node& node_a,
                        const Node& node_b) {
  Node* result1 = FlatTreeTraversalNg::CommonAncestor(node_a, node_b);
  EXPECT_EQ(expected_result, result1)
      << "commonAncestor(" << node_a.textContent() << ","
      << node_b.textContent() << ")";
  Node* result2 = FlatTreeTraversalNg::CommonAncestor(node_b, node_a);
  EXPECT_EQ(expected_result, result2)
      << "commonAncestor(" << node_b.textContent() << ","
      << node_a.textContent() << ")";
}

}  // namespace

// Test case for
//  - childAt
//  - countChildren
//  - hasChildren
//  - index
//  - isDescendantOf
TEST_F(FlatTreeTraversalNgTest, childAt) {
  const char* main_html =
      "<div id='m0'>"
      "<span id='m00'>m00</span>"
      "<span id='m01'>m01</span>"
      "</div>";
  const char* shadow_html =
      "<a id='s00'>s00</a>"
      "<content select='#m01'></content>"
      "<a id='s02'>s02</a>"
      "<a id='s03'><content select='#m00'></content></a>"
      "<a id='s04'>s04</a>";
  SetupSampleHTML(main_html, shadow_html, 0);

  Element* body = GetDocument().body();
  Element* m0 = body->QuerySelector("#m0");
  Element* m00 = m0->QuerySelector("#m00");
  Element* m01 = m0->QuerySelector("#m01");

  Element* shadow_host = m0;
  ShadowRoot* shadow_root = shadow_host->OpenShadowRoot();
  Element* s00 = shadow_root->QuerySelector("#s00");
  Element* s02 = shadow_root->QuerySelector("#s02");
  Element* s03 = shadow_root->QuerySelector("#s03");
  Element* s04 = shadow_root->QuerySelector("#s04");

  const unsigned kNumberOfChildNodes = 5;
  Node* expected_child_nodes[5] = {s00, m01, s02, s03, s04};

  ASSERT_EQ(kNumberOfChildNodes,
            FlatTreeTraversalNg::CountChildren(*shadow_host));
  EXPECT_TRUE(FlatTreeTraversalNg::HasChildren(*shadow_host));

  for (unsigned index = 0; index < kNumberOfChildNodes; ++index) {
    Node* child = FlatTreeTraversalNg::ChildAt(*shadow_host, index);
    EXPECT_EQ(expected_child_nodes[index], child)
        << "FlatTreeTraversalNg::childAt(*shadowHost, " << index << ")";
    EXPECT_EQ(index, FlatTreeTraversalNg::Index(*child))
        << "FlatTreeTraversalNg::index(FlatTreeTraversalNg(*shadowHost, "
        << index << "))";
    EXPECT_TRUE(FlatTreeTraversalNg::IsDescendantOf(*child, *shadow_host))
        << "FlatTreeTraversalNg::isDescendantOf(*FlatTreeTraversalNg(*"
           "shadowHost, "
        << index << "), *shadowHost)";
  }
  EXPECT_EQ(nullptr,
            FlatTreeTraversalNg::ChildAt(*shadow_host, kNumberOfChildNodes + 1))
      << "Out of bounds childAt() returns nullptr.";

  // Distribute node |m00| is child of node in shadow tree |s03|.
  EXPECT_EQ(m00, FlatTreeTraversalNg::ChildAt(*s03, 0));
}

TEST_F(FlatTreeTraversalNgTest, ChildrenOf) {
  SetupSampleHTML(
      "<p id=sample>ZERO<span slot=three>three</b><span "
      "slot=one>one</b>FOUR</p>",
      "zero<slot name=one></slot>two<slot name=three></slot>four", 0);
  Element* const sample = GetDocument().getElementById("sample");

  HeapVector<Member<Node>> expected_nodes;
  for (Node* runner = FlatTreeTraversalNg::FirstChild(*sample); runner;
       runner = FlatTreeTraversalNg::NextSibling(*runner)) {
    expected_nodes.push_back(runner);
  }

  HeapVector<Member<Node>> actual_nodes;
  for (Node& child : FlatTreeTraversalNg::ChildrenOf(*sample))
    actual_nodes.push_back(&child);

  EXPECT_EQ(expected_nodes, actual_nodes);
}

// Test case for
//  - commonAncestor
//  - isDescendantOf
TEST_F(FlatTreeTraversalNgTest, commonAncestor) {
  // We build following flat tree:
  //             ____BODY___
  //             |    |     |
  //            m0    m1    m2       m1 is shadow host having m10, m11, m12.
  //            _|_   |   __|__
  //           |   |  |   |    |
  //          m00 m01 |   m20 m21
  //             _____|_____________
  //             |  |   |    |     |
  //            s10 s11 s12 s13  s14
  //                         |
  //                       __|__
  //                |      |    |
  //                m12    m10 m11 <-- distributed
  // where: each symbol consists with prefix, child index, child-child index.
  //  prefix "m" means node in main tree,
  //  prefix "d" means node in main tree and distributed
  //  prefix "s" means node in shadow tree
  const char* main_html =
      "<a id='m0'><b id='m00'>m00</b><b id='m01'>m01</b></a>"
      "<a id='m1'>"
      "<b id='m10'>m10</b>"
      "<b id='m11'>m11</b>"
      "<b id='m12'>m12</b>"
      "</a>"
      "<a id='m2'><b id='m20'>m20</b><b id='m21'>m21</b></a>";
  const char* shadow_html =
      "<a id='s10'>s10</a>"
      "<a id='s11'><content select='#m12'></content></a>"
      "<a id='s12'>s12</a>"
      "<a id='s13'>"
      "<content select='#m10'></content>"
      "<content select='#m11'></content>"
      "</a>"
      "<a id='s14'>s14</a>";
  SetupSampleHTML(main_html, shadow_html, 1);
  Element* body = GetDocument().body();
  Element* m0 = body->QuerySelector("#m0");
  Element* m1 = body->QuerySelector("#m1");
  Element* m2 = body->QuerySelector("#m2");

  Element* m00 = body->QuerySelector("#m00");
  Element* m01 = body->QuerySelector("#m01");
  Element* m10 = body->QuerySelector("#m10");
  Element* m11 = body->QuerySelector("#m11");
  Element* m12 = body->QuerySelector("#m12");
  Element* m20 = body->QuerySelector("#m20");
  Element* m21 = body->QuerySelector("#m21");

  ShadowRoot* shadow_root = m1->OpenShadowRoot();
  Element* s10 = shadow_root->QuerySelector("#s10");
  Element* s11 = shadow_root->QuerySelector("#s11");
  Element* s12 = shadow_root->QuerySelector("#s12");
  Element* s13 = shadow_root->QuerySelector("#s13");
  Element* s14 = shadow_root->QuerySelector("#s14");

  TestCommonAncestor(body, *m0, *m1);
  TestCommonAncestor(body, *m1, *m2);
  TestCommonAncestor(body, *m1, *m20);
  TestCommonAncestor(body, *s14, *m21);

  TestCommonAncestor(m0, *m0, *m0);
  TestCommonAncestor(m0, *m00, *m01);

  TestCommonAncestor(m1, *m1, *m1);
  TestCommonAncestor(m1, *s10, *s14);
  TestCommonAncestor(m1, *s10, *m12);
  TestCommonAncestor(m1, *s12, *m12);
  TestCommonAncestor(m1, *m10, *m12);

  TestCommonAncestor(m01, *m01, *m01);
  TestCommonAncestor(s11, *s11, *m12);
  TestCommonAncestor(s13, *m10, *m11);

  s12->remove(ASSERT_NO_EXCEPTION);
  TestCommonAncestor(s12, *s12, *s12);
  TestCommonAncestor(nullptr, *s12, *s11);
  TestCommonAncestor(nullptr, *s12, *m01);
  TestCommonAncestor(nullptr, *s12, *m20);

  m20->remove(ASSERT_NO_EXCEPTION);
  TestCommonAncestor(m20, *m20, *m20);
  TestCommonAncestor(nullptr, *m20, *s12);
  TestCommonAncestor(nullptr, *m20, *m1);
}

// Test case for
//  - nextSkippingChildren
//  - previousSkippingChildren
TEST_F(FlatTreeTraversalNgTest, nextSkippingChildren) {
  const char* main_html =
      "<div id='m0'>m0</div>"
      "<div id='m1'>"
      "<span id='m10'>m10</span>"
      "<span id='m11'>m11</span>"
      "</div>"
      "<div id='m2'>m2</div>";
  const char* shadow_html =
      "<content select='#m11'></content>"
      "<a id='s11'>s11</a>"
      "<a id='s12'>"
      "<b id='s120'>s120</b>"
      "<content select='#m10'></content>"
      "</a>";
  SetupSampleHTML(main_html, shadow_html, 1);

  Element* body = GetDocument().body();
  Element* m0 = body->QuerySelector("#m0");
  Element* m1 = body->QuerySelector("#m1");
  Element* m2 = body->QuerySelector("#m2");

  Element* m10 = body->QuerySelector("#m10");
  Element* m11 = body->QuerySelector("#m11");

  ShadowRoot* shadow_root = m1->OpenShadowRoot();
  Element* s11 = shadow_root->QuerySelector("#s11");
  Element* s12 = shadow_root->QuerySelector("#s12");
  Element* s120 = shadow_root->QuerySelector("#s120");

  // Main tree node to main tree node
  EXPECT_EQ(*m1, FlatTreeTraversalNg::NextSkippingChildren(*m0));
  EXPECT_EQ(*m0, FlatTreeTraversalNg::PreviousSkippingChildren(*m1));

  // Distribute node to main tree node
  EXPECT_EQ(*m2, FlatTreeTraversalNg::NextSkippingChildren(*m10));
  EXPECT_EQ(*m1, FlatTreeTraversalNg::PreviousSkippingChildren(*m2));

  // Distribute node to node in shadow tree
  EXPECT_EQ(*s11, FlatTreeTraversalNg::NextSkippingChildren(*m11));
  EXPECT_EQ(*m11, FlatTreeTraversalNg::PreviousSkippingChildren(*s11));

  // Node in shadow tree to distributed node
  EXPECT_EQ(*s11, FlatTreeTraversalNg::NextSkippingChildren(*m11));
  EXPECT_EQ(*m11, FlatTreeTraversalNg::PreviousSkippingChildren(*s11));

  EXPECT_EQ(*m10, FlatTreeTraversalNg::NextSkippingChildren(*s120));
  EXPECT_EQ(*s120, FlatTreeTraversalNg::PreviousSkippingChildren(*m10));

  // Node in shadow tree to main tree
  EXPECT_EQ(*m2, FlatTreeTraversalNg::NextSkippingChildren(*s12));
  EXPECT_EQ(*m1, FlatTreeTraversalNg::PreviousSkippingChildren(*m2));
}

// Test case for
//  - lastWithin
//  - lastWithinOrSelf
TEST_F(FlatTreeTraversalNgTest, lastWithin) {
  const char* main_html =
      "<div id='m0'>m0</div>"
      "<div id='m1'>"
      "<span id='m10'>m10</span>"
      "<span id='m11'>m11</span>"
      "<span id='m12'>m12</span>"  // #m12 is not distributed.
      "</div>"
      "<div id='m2'></div>";
  const char* shadow_html =
      "<content select='#m11'></content>"
      "<a id='s11'>s11</a>"
      "<a id='s12'>"
      "<content select='#m10'></content>"
      "</a>";
  SetupSampleHTML(main_html, shadow_html, 1);

  Element* body = GetDocument().body();
  Element* m0 = body->QuerySelector("#m0");
  Element* m1 = body->QuerySelector("#m1");
  Element* m2 = body->QuerySelector("#m2");

  Element* m10 = body->QuerySelector("#m10");

  ShadowRoot* shadow_root = m1->OpenShadowRoot();
  Element* s11 = shadow_root->QuerySelector("#s11");
  Element* s12 = shadow_root->QuerySelector("#s12");

  EXPECT_EQ(m0->firstChild(), FlatTreeTraversalNg::LastWithin(*m0));
  EXPECT_EQ(*m0->firstChild(), FlatTreeTraversalNg::LastWithinOrSelf(*m0));

  EXPECT_EQ(m10->firstChild(), FlatTreeTraversalNg::LastWithin(*m1));
  EXPECT_EQ(*m10->firstChild(), FlatTreeTraversalNg::LastWithinOrSelf(*m1));

  EXPECT_EQ(nullptr, FlatTreeTraversalNg::LastWithin(*m2));
  EXPECT_EQ(*m2, FlatTreeTraversalNg::LastWithinOrSelf(*m2));

  EXPECT_EQ(s11->firstChild(), FlatTreeTraversalNg::LastWithin(*s11));
  EXPECT_EQ(*s11->firstChild(), FlatTreeTraversalNg::LastWithinOrSelf(*s11));

  EXPECT_EQ(m10->firstChild(), FlatTreeTraversalNg::LastWithin(*s12));
  EXPECT_EQ(*m10->firstChild(), FlatTreeTraversalNg::LastWithinOrSelf(*s12));
}

TEST_F(FlatTreeTraversalNgTest, previousPostOrder) {
  const char* main_html =
      "<div id='m0'>m0</div>"
      "<div id='m1'>"
      "<span id='m10'>m10</span>"
      "<span id='m11'>m11</span>"
      "</div>"
      "<div id='m2'>m2</div>";
  const char* shadow_html =
      "<content select='#m11'></content>"
      "<a id='s11'>s11</a>"
      "<a id='s12'>"
      "<b id='s120'>s120</b>"
      "<content select='#m10'></content>"
      "</a>";
  SetupSampleHTML(main_html, shadow_html, 1);

  Element* body = GetDocument().body();
  Element* m0 = body->QuerySelector("#m0");
  Element* m1 = body->QuerySelector("#m1");
  Element* m2 = body->QuerySelector("#m2");

  Element* m10 = body->QuerySelector("#m10");
  Element* m11 = body->QuerySelector("#m11");

  ShadowRoot* shadow_root = m1->OpenShadowRoot();
  Element* s11 = shadow_root->QuerySelector("#s11");
  Element* s12 = shadow_root->QuerySelector("#s12");
  Element* s120 = shadow_root->QuerySelector("#s120");

  EXPECT_EQ(*m0->firstChild(), FlatTreeTraversalNg::PreviousPostOrder(*m0));
  EXPECT_EQ(*s12, FlatTreeTraversalNg::PreviousPostOrder(*m1));
  EXPECT_EQ(*m10->firstChild(), FlatTreeTraversalNg::PreviousPostOrder(*m10));
  EXPECT_EQ(*s120, FlatTreeTraversalNg::PreviousPostOrder(*m10->firstChild()));
  EXPECT_EQ(*s120,
            FlatTreeTraversalNg::PreviousPostOrder(*m10->firstChild(), s12));
  EXPECT_EQ(*m11->firstChild(), FlatTreeTraversalNg::PreviousPostOrder(*m11));
  EXPECT_EQ(*m0, FlatTreeTraversalNg::PreviousPostOrder(*m11->firstChild()));
  EXPECT_EQ(nullptr,
            FlatTreeTraversalNg::PreviousPostOrder(*m11->firstChild(), m11));
  EXPECT_EQ(*m2->firstChild(), FlatTreeTraversalNg::PreviousPostOrder(*m2));

  EXPECT_EQ(*s11->firstChild(), FlatTreeTraversalNg::PreviousPostOrder(*s11));
  EXPECT_EQ(*m10, FlatTreeTraversalNg::PreviousPostOrder(*s12));
  EXPECT_EQ(*s120->firstChild(), FlatTreeTraversalNg::PreviousPostOrder(*s120));
  EXPECT_EQ(*s11, FlatTreeTraversalNg::PreviousPostOrder(*s120->firstChild()));
  EXPECT_EQ(nullptr,
            FlatTreeTraversalNg::PreviousPostOrder(*s120->firstChild(), s12));
}

TEST_F(FlatTreeTraversalNgTest, nextSiblingNotInDocumentFlatTree) {
  const char* main_html =
      "<div id='m0'>m0</div>"
      "<div id='m1'>"
      "<span id='m10'>m10</span>"
      "<span id='m11'>m11</span>"
      "</div>"
      "<div id='m2'>m2</div>";
  const char* shadow_html = "<content select='#m11'></content>";
  SetupSampleHTML(main_html, shadow_html, 1);

  Element* body = GetDocument().body();
  Element* m10 = body->QuerySelector("#m10");

  EXPECT_EQ(nullptr, FlatTreeTraversalNg::NextSibling(*m10));
  EXPECT_EQ(nullptr, FlatTreeTraversalNg::PreviousSibling(*m10));
}

TEST_F(FlatTreeTraversalNgTest, redistribution) {
  const char* main_html =
      "<div id='m0'>m0</div>"
      "<div id='m1'>"
      "<span id='m10'>m10</span>"
      "<span id='m11'>m11</span>"
      "</div>"
      "<div id='m2'>m2</div>";
  const char* shadow_html1 =
      "<div id='s1'>"
      "<content></content>"
      "</div>";

  SetupSampleHTML(main_html, shadow_html1, 1);

  const char* shadow_html2 =
      "<div id='s2'>"
      "<content select='#m10'></content>"
      "<span id='s21'>s21</span>"
      "</div>";

  Element* body = GetDocument().body();
  Element* m1 = body->QuerySelector("#m1");
  Element* m10 = body->QuerySelector("#m10");

  ShadowRoot* shadow_root1 = m1->OpenShadowRoot();
  Element* s1 = shadow_root1->QuerySelector("#s1");

  AttachV0ShadowRoot(*s1, shadow_html2);

  ShadowRoot* shadow_root2 = s1->OpenShadowRoot();
  Element* s21 = shadow_root2->QuerySelector("#s21");

  EXPECT_EQ(s21, FlatTreeTraversalNg::NextSibling(*m10));
  EXPECT_EQ(m10, FlatTreeTraversalNg::PreviousSibling(*s21));

  // FlatTreeTraversalNg::traverseSiblings does not work for a node which is not
  // in a document flat tree.
  // e.g. The following test fails. The result of
  // FlatTreeTraversalNg::previousSibling(*m11)) will be #m10, instead of
  // nullptr. Element* m11 = body->querySelector("#m11"); EXPECT_EQ(nullptr,
  // FlatTreeTraversalNg::previousSibling(*m11));
}

TEST_F(FlatTreeTraversalNgTest, v1Simple) {
  const char* main_html =
      "<div id='host'>"
      "<div id='child1' slot='slot1'></div>"
      "<div id='child2' slot='slot2'></div>"
      "</div>";
  const char* shadow_html =
      "<div id='shadow-child1'></div>"
      "<slot name='slot1'></slot>"
      "<slot name='slot2'></slot>"
      "<div id='shadow-child2'></div>";

  SetupDocumentTree(main_html);
  Element* body = GetDocument().body();
  Element* host = body->QuerySelector("#host");
  Element* child1 = body->QuerySelector("#child1");
  Element* child2 = body->QuerySelector("#child2");

  AttachOpenShadowRoot(*host, shadow_html);
  ShadowRoot* shadow_root = host->OpenShadowRoot();
  Element* slot1 = shadow_root->QuerySelector("[name=slot1]");
  Element* slot2 = shadow_root->QuerySelector("[name=slot2]");
  Element* shadow_child1 = shadow_root->QuerySelector("#shadow-child1");
  Element* shadow_child2 = shadow_root->QuerySelector("#shadow-child2");

  EXPECT_TRUE(slot1);
  EXPECT_TRUE(slot2);
  EXPECT_EQ(shadow_child1, FlatTreeTraversalNg::FirstChild(*host));
  EXPECT_EQ(slot1, FlatTreeTraversalNg::NextSibling(*shadow_child1));
  EXPECT_EQ(nullptr, FlatTreeTraversalNg::NextSibling(*child1));
  EXPECT_EQ(nullptr, FlatTreeTraversalNg::NextSibling(*child2));
  EXPECT_EQ(slot2, FlatTreeTraversalNg::NextSibling(*slot1));
  EXPECT_EQ(shadow_child2, FlatTreeTraversalNg::NextSibling(*slot2));
}

TEST_F(FlatTreeTraversalNgTest, v1Redistribution) {
  // composed tree:
  // d1
  // ├──/shadow-root
  // │   └── d1-1
  // │       ├──/shadow-root
  // │       │   ├── d1-1-1
  // │       │   ├── slot name=d1-1-s1
  // │       │   ├── slot name=d1-1-s2
  // │       │   └── d1-1-2
  // │       ├── d1-2
  // │       ├── slot id=d1-s0
  // │       ├── slot name=d1-s1 slot=d1-1-s1
  // │       ├── slot name=d1-s2
  // │       ├── d1-3
  // │       └── d1-4 slot=d1-1-s1
  // ├── d2 slot=d1-s1
  // ├── d3 slot=d1-s2
  // ├── d4 slot=nonexistent
  // └── d5

  // flat tree:
  // d1
  // └── d1-1
  //     ├── d1-1-1
  //     ├── slot name=d1-1-s1
  //     │   ├── slot name=d1-s1 slot=d1-1-s1
  //     │   │   └── d2 slot=d1-s1
  //     │   └── d1-4 slot=d1-1-s1
  //     ├── slot name=d1-1-s2
  //     └── d1-1-2
  const char* main_html =
      "<div id='d1'>"
      "<div id='d2' slot='d1-s1'></div>"
      "<div id='d3' slot='d1-s2'></div>"
      "<div id='d4' slot='nonexistent'></div>"
      "<div id='d5'></div>"
      "</div>"
      "<div id='d6'></div>";
  const char* shadow_html1 =
      "<div id='d1-1'>"
      "<div id='d1-2'></div>"
      "<slot id='d1-s0'></slot>"
      "<slot name='d1-s1' slot='d1-1-s1'></slot>"
      "<slot name='d1-s2'></slot>"
      "<div id='d1-3'></div>"
      "<div id='d1-4' slot='d1-1-s1'></div>"
      "</div>";
  const char* shadow_html2 =
      "<div id='d1-1-1'></div>"
      "<slot name='d1-1-s1'></slot>"
      "<slot name='d1-1-s2'></slot>"
      "<div id='d1-1-2'></div>";

  SetupDocumentTree(main_html);

  Element* body = GetDocument().body();
  Element* d1 = body->QuerySelector("#d1");
  Element* d2 = body->QuerySelector("#d2");
  Element* d3 = body->QuerySelector("#d3");
  Element* d4 = body->QuerySelector("#d4");
  Element* d5 = body->QuerySelector("#d5");
  Element* d6 = body->QuerySelector("#d6");

  AttachOpenShadowRoot(*d1, shadow_html1);
  ShadowRoot* shadow_root1 = d1->OpenShadowRoot();
  Element* d11 = shadow_root1->QuerySelector("#d1-1");
  Element* d12 = shadow_root1->QuerySelector("#d1-2");
  Element* d13 = shadow_root1->QuerySelector("#d1-3");
  Element* d14 = shadow_root1->QuerySelector("#d1-4");
  Element* d1s0 = shadow_root1->QuerySelector("#d1-s0");
  Element* d1s1 = shadow_root1->QuerySelector("[name=d1-s1]");
  Element* d1s2 = shadow_root1->QuerySelector("[name=d1-s2]");

  AttachOpenShadowRoot(*d11, shadow_html2);
  ShadowRoot* shadow_root2 = d11->OpenShadowRoot();
  Element* d111 = shadow_root2->QuerySelector("#d1-1-1");
  Element* d112 = shadow_root2->QuerySelector("#d1-1-2");
  Element* d11s1 = shadow_root2->QuerySelector("[name=d1-1-s1]");
  Element* d11s2 = shadow_root2->QuerySelector("[name=d1-1-s2]");

  EXPECT_TRUE(d5);
  EXPECT_TRUE(d12);
  EXPECT_TRUE(d13);
  EXPECT_TRUE(d1s0);
  EXPECT_TRUE(d1s1);
  EXPECT_TRUE(d1s2);
  EXPECT_TRUE(d11s1);
  EXPECT_TRUE(d11s2);

  EXPECT_EQ(d11, FlatTreeTraversalNg::Next(*d1));
  EXPECT_EQ(d111, FlatTreeTraversalNg::Next(*d11));
  EXPECT_EQ(d11s1, FlatTreeTraversalNg::Next(*d111));
  EXPECT_EQ(d1s1, FlatTreeTraversalNg::Next(*d11s1));
  EXPECT_EQ(d2, FlatTreeTraversalNg::Next(*d1s1));
  EXPECT_EQ(d14, FlatTreeTraversalNg::Next(*d2));
  EXPECT_EQ(d11s2, FlatTreeTraversalNg::Next(*d14));
  EXPECT_EQ(d112, FlatTreeTraversalNg::Next(*d11s2));
  EXPECT_EQ(d6, FlatTreeTraversalNg::Next(*d112));

  EXPECT_EQ(d112, FlatTreeTraversalNg::Previous(*d6));

  EXPECT_EQ(d11, FlatTreeTraversalNg::Parent(*d111));
  EXPECT_EQ(d11, FlatTreeTraversalNg::Parent(*d112));
  EXPECT_EQ(d1s1, FlatTreeTraversalNg::Parent(*d2));
  EXPECT_EQ(d11s1, FlatTreeTraversalNg::Parent(*d14));
  EXPECT_EQ(d1s2, FlatTreeTraversalNg::Parent(*d3));
  EXPECT_EQ(nullptr, FlatTreeTraversalNg::Parent(*d4));
}

TEST_F(FlatTreeTraversalNgTest, v1SlotInDocumentTree) {
  const char* main_html =
      "<div id='parent'>"
      "<slot>"
      "<div id='child1'></div>"
      "<div id='child2'></div>"
      "</slot>"
      "</div>";

  SetupDocumentTree(main_html);
  Element* body = GetDocument().body();
  Element* parent = body->QuerySelector("#parent");
  Element* slot = body->QuerySelector("slot");
  Element* child1 = body->QuerySelector("#child1");
  Element* child2 = body->QuerySelector("#child2");

  EXPECT_EQ(slot, FlatTreeTraversalNg::FirstChild(*parent));
  EXPECT_EQ(child1, FlatTreeTraversalNg::FirstChild(*slot));
  EXPECT_EQ(child2, FlatTreeTraversalNg::NextSibling(*child1));
  EXPECT_EQ(nullptr, FlatTreeTraversalNg::NextSibling(*child2));
  EXPECT_EQ(slot, FlatTreeTraversalNg::Parent(*child1));
  EXPECT_EQ(slot, FlatTreeTraversalNg::Parent(*child2));
  EXPECT_EQ(parent, FlatTreeTraversalNg::Parent(*slot));
}

TEST_F(FlatTreeTraversalNgTest, v1FallbackContent) {
  const char* main_html = "<div id='d1'></div>";
  const char* shadow_html =
      "<div id='before'></div>"
      "<slot><p>fallback content</p></slot>"
      "<div id='after'></div>";

  SetupDocumentTree(main_html);

  Element* body = GetDocument().body();
  Element* d1 = body->QuerySelector("#d1");

  AttachOpenShadowRoot(*d1, shadow_html);
  ShadowRoot* shadow_root = d1->OpenShadowRoot();
  Element* before = shadow_root->QuerySelector("#before");
  Element* after = shadow_root->QuerySelector("#after");
  Element* fallback_content = shadow_root->QuerySelector("p");
  Element* slot = shadow_root->QuerySelector("slot");

  EXPECT_EQ(before, FlatTreeTraversalNg::FirstChild(*d1));
  EXPECT_EQ(after, FlatTreeTraversalNg::LastChild(*d1));
  EXPECT_EQ(slot, FlatTreeTraversalNg::Parent(*fallback_content));

  EXPECT_EQ(slot, FlatTreeTraversalNg::NextSibling(*before));
  EXPECT_EQ(after, FlatTreeTraversalNg::NextSibling(*slot));
  EXPECT_EQ(nullptr, FlatTreeTraversalNg::NextSibling(*fallback_content));
  EXPECT_EQ(nullptr, FlatTreeTraversalNg::NextSibling(*after));

  EXPECT_EQ(slot, FlatTreeTraversalNg::PreviousSibling(*after));
  EXPECT_EQ(before, FlatTreeTraversalNg::PreviousSibling(*slot));
  EXPECT_EQ(nullptr, FlatTreeTraversalNg::PreviousSibling(*fallback_content));
  EXPECT_EQ(nullptr, FlatTreeTraversalNg::PreviousSibling(*before));
}

TEST_F(FlatTreeTraversalNgTest, v1FallbackContentSkippedInTraversal) {
  const char* main_html = "<div id='d1'><span></span></div>";
  const char* shadow_html =
      "<div id='before'></div>"
      "<slot><p>fallback content</p></slot>"
      "<div id='after'></div>";

  SetupDocumentTree(main_html);

  Element* body = GetDocument().body();
  Element* d1 = body->QuerySelector("#d1");
  Element* span = body->QuerySelector("span");

  AttachOpenShadowRoot(*d1, shadow_html);
  ShadowRoot* shadow_root = d1->OpenShadowRoot();
  Element* before = shadow_root->QuerySelector("#before");
  Element* after = shadow_root->QuerySelector("#after");
  Element* fallback_content = shadow_root->QuerySelector("p");
  Element* slot = shadow_root->QuerySelector("slot");

  EXPECT_EQ(before, FlatTreeTraversalNg::FirstChild(*d1));
  EXPECT_EQ(after, FlatTreeTraversalNg::LastChild(*d1));
  EXPECT_EQ(slot, FlatTreeTraversalNg::Parent(*span));
  EXPECT_EQ(d1, FlatTreeTraversalNg::Parent(*slot));

  EXPECT_EQ(slot, FlatTreeTraversalNg::NextSibling(*before));
  EXPECT_EQ(after, FlatTreeTraversalNg::NextSibling(*slot));
  EXPECT_EQ(nullptr, FlatTreeTraversalNg::NextSibling(*after));

  EXPECT_EQ(slot, FlatTreeTraversalNg::PreviousSibling(*after));
  EXPECT_EQ(before, FlatTreeTraversalNg::PreviousSibling(*slot));
  EXPECT_EQ(nullptr, FlatTreeTraversalNg::PreviousSibling(*before));

  EXPECT_EQ(nullptr, FlatTreeTraversalNg::Parent(*fallback_content));
  EXPECT_EQ(nullptr, FlatTreeTraversalNg::NextSibling(*fallback_content));
  EXPECT_EQ(nullptr, FlatTreeTraversalNg::PreviousSibling(*fallback_content));
}

TEST_F(FlatTreeTraversalNgTest, v1AllFallbackContent) {
  const char* main_html = "<div id='d1'></div>";
  const char* shadow_html =
      "<slot name='a'><p id='x'>fallback content X</p></slot>"
      "<slot name='b'><p id='y'>fallback content Y</p></slot>"
      "<slot name='c'><p id='z'>fallback content Z</p></slot>";

  SetupDocumentTree(main_html);

  Element* body = GetDocument().body();
  Element* d1 = body->QuerySelector("#d1");

  AttachOpenShadowRoot(*d1, shadow_html);
  ShadowRoot* shadow_root = d1->OpenShadowRoot();
  Element* slot_a = shadow_root->QuerySelector("slot[name=a]");
  Element* slot_b = shadow_root->QuerySelector("slot[name=b]");
  Element* slot_c = shadow_root->QuerySelector("slot[name=c]");
  Element* fallback_x = shadow_root->QuerySelector("#x");
  Element* fallback_y = shadow_root->QuerySelector("#y");
  Element* fallback_z = shadow_root->QuerySelector("#z");

  EXPECT_EQ(slot_a, FlatTreeTraversalNg::FirstChild(*d1));
  EXPECT_EQ(slot_c, FlatTreeTraversalNg::LastChild(*d1));

  EXPECT_EQ(fallback_x, FlatTreeTraversalNg::FirstChild(*slot_a));
  EXPECT_EQ(fallback_y, FlatTreeTraversalNg::FirstChild(*slot_b));
  EXPECT_EQ(fallback_z, FlatTreeTraversalNg::FirstChild(*slot_c));

  EXPECT_EQ(slot_a, FlatTreeTraversalNg::Parent(*fallback_x));
  EXPECT_EQ(slot_b, FlatTreeTraversalNg::Parent(*fallback_y));
  EXPECT_EQ(slot_c, FlatTreeTraversalNg::Parent(*fallback_z));
  EXPECT_EQ(d1, FlatTreeTraversalNg::Parent(*slot_a));

  EXPECT_EQ(nullptr, FlatTreeTraversalNg::NextSibling(*fallback_x));
  EXPECT_EQ(nullptr, FlatTreeTraversalNg::NextSibling(*fallback_y));
  EXPECT_EQ(nullptr, FlatTreeTraversalNg::NextSibling(*fallback_z));

  EXPECT_EQ(nullptr, FlatTreeTraversalNg::PreviousSibling(*fallback_z));
  EXPECT_EQ(nullptr, FlatTreeTraversalNg::PreviousSibling(*fallback_y));
  EXPECT_EQ(nullptr, FlatTreeTraversalNg::PreviousSibling(*fallback_x));
}

}  // namespace flat_tree_traversal_ng_test
}  // namespace blink
