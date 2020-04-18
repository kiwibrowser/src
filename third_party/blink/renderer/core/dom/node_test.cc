// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/dom/node.h"

#include "third_party/blink/renderer/bindings/core/v8/v8_binding_for_core.h"
#include "third_party/blink/renderer/core/css/resolver/style_resolver.h"
#include "third_party/blink/renderer/core/dom/element.h"
#include "third_party/blink/renderer/core/dom/flat_tree_traversal.h"
#include "third_party/blink/renderer/core/dom/shadow_root.h"
#include "third_party/blink/renderer/core/dom/shadow_root_init.h"
#include "third_party/blink/renderer/core/editing/testing/editing_test_base.h"
#include "third_party/blink/renderer/core/html/html_div_element.h"

namespace blink {

class FakeMediaControlElement : public HTMLDivElement {
 public:
  FakeMediaControlElement(Document& document) : HTMLDivElement(document) {}

  bool IsMediaControlElement() const override { return true; }
};

class FakeMediaControls : public HTMLDivElement {
 public:
  FakeMediaControls(Document& document) : HTMLDivElement(document) {}

  bool IsMediaControls() const override { return true; }
};

class NodeTest : public EditingTestBase {
 protected:
  LayoutObject* ReattachLayoutTreeForNode(Node& node) {
    node.LazyReattachIfAttached();
    GetDocument().Lifecycle().AdvanceTo(DocumentLifecycle::kInStyleRecalc);
    GetDocument().documentElement()->RecalcStyle(kNoChange);
    PushSelectorFilterAncestors(
        GetDocument().EnsureStyleResolver().GetSelectorFilter(), node);
    Node::AttachContext context;
    node.ReattachLayoutTree(context);
    return context.previous_in_flow;
  }

  // Generate the following DOM structure and return the innermost <div>.
  //  + div#root
  //    + #shadow
  //      + test node
  //      |  + #shadow
  //      |    + div class="test"
  Node* InitializeUserAgentShadowTree(Element* test_node) {
    SetBodyContent("<div id=\"root\"></div>");
    Element* root = GetDocument().getElementById("root");
    ShadowRoot& first_shadow = root->CreateUserAgentShadowRoot();

    first_shadow.AppendChild(test_node);
    ShadowRoot& second_shadow = test_node->CreateUserAgentShadowRoot();

    HTMLDivElement* class_div = HTMLDivElement::Create(GetDocument());
    class_div->setAttribute("class", "test");
    second_shadow.AppendChild(class_div);
    return class_div;
  }

 private:
  void PushSelectorFilterAncestors(SelectorFilter& filter, Node& node) {
    if (Element* parent = FlatTreeTraversal::ParentElement(node)) {
      PushSelectorFilterAncestors(filter, *parent);
      filter.PushParent(*parent);
    }
  }
};

TEST_F(NodeTest, canStartSelection) {
  const char* body_content =
      "<a id=one href='http://www.msn.com'>one</a><b id=two>two</b>";
  SetBodyContent(body_content);
  Node* one = GetDocument().getElementById("one");
  Node* two = GetDocument().getElementById("two");

  EXPECT_FALSE(one->CanStartSelection());
  EXPECT_FALSE(one->firstChild()->CanStartSelection());
  EXPECT_TRUE(two->CanStartSelection());
  EXPECT_TRUE(two->firstChild()->CanStartSelection());
}

TEST_F(NodeTest, canStartSelectionWithShadowDOM) {
  const char* body_content = "<div id=host><span id=one>one</span></div>";
  const char* shadow_content =
      "<a href='http://www.msn.com'><content></content></a>";
  SetBodyContent(body_content);
  SetShadowContent(shadow_content, "host");
  Node* one = GetDocument().getElementById("one");

  EXPECT_FALSE(one->CanStartSelection());
  EXPECT_FALSE(one->firstChild()->CanStartSelection());
}

TEST_F(NodeTest, customElementState) {
  const char* body_content = "<div id=div></div>";
  SetBodyContent(body_content);
  Element* div = GetDocument().getElementById("div");
  EXPECT_EQ(CustomElementState::kUncustomized, div->GetCustomElementState());
  EXPECT_TRUE(div->IsDefined());
  EXPECT_EQ(Node::kV0NotCustomElement, div->GetV0CustomElementState());

  div->SetCustomElementState(CustomElementState::kUndefined);
  EXPECT_EQ(CustomElementState::kUndefined, div->GetCustomElementState());
  EXPECT_FALSE(div->IsDefined());
  EXPECT_EQ(Node::kV0NotCustomElement, div->GetV0CustomElementState());

  div->SetCustomElementState(CustomElementState::kCustom);
  EXPECT_EQ(CustomElementState::kCustom, div->GetCustomElementState());
  EXPECT_TRUE(div->IsDefined());
  EXPECT_EQ(Node::kV0NotCustomElement, div->GetV0CustomElementState());
}

TEST_F(NodeTest, AttachContext_PreviousInFlow_TextRoot) {
  SetBodyContent("Text");
  Node* root = GetDocument().body()->firstChild();
  LayoutObject* previous_in_flow = ReattachLayoutTreeForNode(*root);

  EXPECT_TRUE(previous_in_flow);
  EXPECT_EQ(root->GetLayoutObject(), previous_in_flow);
}

TEST_F(NodeTest, AttachContext_PreviousInFlow_InlineRoot) {
  SetBodyContent("<span id=root>Text <span></span></span>");
  Element* root = GetDocument().getElementById("root");
  LayoutObject* previous_in_flow = ReattachLayoutTreeForNode(*root);

  EXPECT_TRUE(previous_in_flow);
  EXPECT_EQ(root->GetLayoutObject(), previous_in_flow);
}

TEST_F(NodeTest, AttachContext_PreviousInFlow_BlockRoot) {
  SetBodyContent("<div id=root>Text <span></span></div>");
  Element* root = GetDocument().getElementById("root");
  LayoutObject* previous_in_flow = ReattachLayoutTreeForNode(*root);

  EXPECT_TRUE(previous_in_flow);
  EXPECT_EQ(root->GetLayoutObject(), previous_in_flow);
}

TEST_F(NodeTest, AttachContext_PreviousInFlow_FloatRoot) {
  SetBodyContent("<div id=root style='float:left'><span></span></div>");
  Element* root = GetDocument().getElementById("root");
  LayoutObject* previous_in_flow = ReattachLayoutTreeForNode(*root);

  EXPECT_FALSE(previous_in_flow);
}

TEST_F(NodeTest, AttachContext_PreviousInFlow_AbsoluteRoot) {
  SetBodyContent("<div id=root style='position:absolute'><span></span></div>");
  Element* root = GetDocument().getElementById("root");
  LayoutObject* previous_in_flow = ReattachLayoutTreeForNode(*root);

  EXPECT_FALSE(previous_in_flow);
}

TEST_F(NodeTest, AttachContext_PreviousInFlow_Text) {
  SetBodyContent("<div id=root style='display:contents'>Text</div>");
  Element* root = GetDocument().getElementById("root");
  LayoutObject* previous_in_flow = ReattachLayoutTreeForNode(*root);

  EXPECT_TRUE(previous_in_flow);
  EXPECT_EQ(root->firstChild()->GetLayoutObject(), previous_in_flow);
}

TEST_F(NodeTest, AttachContext_PreviousInFlow_Inline) {
  SetBodyContent("<div id=root style='display:contents'><span></span></div>");
  Element* root = GetDocument().getElementById("root");
  LayoutObject* previous_in_flow = ReattachLayoutTreeForNode(*root);

  EXPECT_TRUE(previous_in_flow);
  EXPECT_EQ(root->firstChild()->GetLayoutObject(), previous_in_flow);
}

TEST_F(NodeTest, AttachContext_PreviousInFlow_Block) {
  SetBodyContent("<div id=root style='display:contents'><div></div></div>");
  Element* root = GetDocument().getElementById("root");
  LayoutObject* previous_in_flow = ReattachLayoutTreeForNode(*root);

  EXPECT_TRUE(previous_in_flow);
  EXPECT_EQ(root->firstChild()->GetLayoutObject(), previous_in_flow);
}

TEST_F(NodeTest, AttachContext_PreviousInFlow_Float) {
  SetBodyContent(
      "<style>"
      "  #root { display:contents }"
      "  .float { float:left }"
      "</style>"
      "<div id=root><div class=float></div></div>");
  Element* root = GetDocument().getElementById("root");
  LayoutObject* previous_in_flow = ReattachLayoutTreeForNode(*root);

  EXPECT_FALSE(previous_in_flow);
}

TEST_F(NodeTest, AttachContext_PreviousInFlow_AbsolutePositioned) {
  SetBodyContent(
      "<style>"
      "  #root { display:contents }"
      "  .abs { position:absolute }"
      "</style>"
      "<div id=root><div class=abs></div></div>");
  Element* root = GetDocument().getElementById("root");
  LayoutObject* previous_in_flow = ReattachLayoutTreeForNode(*root);

  EXPECT_FALSE(previous_in_flow);
}

TEST_F(NodeTest, AttachContext_PreviousInFlow_SkipAbsolute) {
  SetBodyContent(
      "<style>"
      "  #root { display:contents }"
      "  .abs { position:absolute }"
      "</style>"
      "<div id=root>"
      "<div class=abs></div><span id=inline></span><div class=abs></div>"
      "</div>");
  Element* root = GetDocument().getElementById("root");
  Element* span = GetDocument().getElementById("inline");
  LayoutObject* previous_in_flow = ReattachLayoutTreeForNode(*root);

  EXPECT_TRUE(previous_in_flow);
  EXPECT_EQ(span->GetLayoutObject(), previous_in_flow);
}

TEST_F(NodeTest, AttachContext_PreviousInFlow_SkipFloats) {
  SetBodyContent(
      "<style>"
      "  #root { display:contents }"
      "  .float { float:left }"
      "</style>"
      "<div id=root>"
      "<div class=float></div>"
      "<span id=inline></span>"
      "<div class=float></div>"
      "</div>");
  Element* root = GetDocument().getElementById("root");
  Element* span = GetDocument().getElementById("inline");
  LayoutObject* previous_in_flow = ReattachLayoutTreeForNode(*root);

  EXPECT_TRUE(previous_in_flow);
  EXPECT_EQ(span->GetLayoutObject(), previous_in_flow);
}

TEST_F(NodeTest, AttachContext_PreviousInFlow_InsideDisplayContents) {
  SetBodyContent(
      "<style>"
      "  #root, .contents { display:contents }"
      "  .float { float:left }"
      "</style>"
      "<div id=root>"
      "<span></span><div class=contents><span id=inline></span></div>"
      "</div>");
  Element* root = GetDocument().getElementById("root");
  Element* span = GetDocument().getElementById("inline");
  LayoutObject* previous_in_flow = ReattachLayoutTreeForNode(*root);

  EXPECT_TRUE(previous_in_flow);
  EXPECT_EQ(span->GetLayoutObject(), previous_in_flow);
}

TEST_F(NodeTest, AttachContext_PreviousInFlow_Slotted) {
  SetBodyContent("<div id=host><span id=inline></span></div>");
  ShadowRoot& shadow_root =
      GetDocument().getElementById("host")->AttachShadowRootInternal(
          ShadowRootType::kOpen);
  shadow_root.SetInnerHTMLFromString(
      "<div id=root style='display:contents'><span></span><slot></slot></div>");
  GetDocument().View()->UpdateAllLifecyclePhases();

  Element* root = shadow_root.getElementById("root");
  Element* span = GetDocument().getElementById("inline");
  LayoutObject* previous_in_flow = ReattachLayoutTreeForNode(*root);

  EXPECT_TRUE(previous_in_flow);
  EXPECT_EQ(span->GetLayoutObject(), previous_in_flow);
}

TEST_F(NodeTest, AttachContext_PreviousInFlow_V0Content) {
  SetBodyContent("<div id=host><span id=inline></span></div>");
  ShadowRoot* shadow_root = CreateShadowRootForElementWithIDAndSetInnerHTML(
      GetDocument(), "host",
      "<div id=root style='display:contents'><span></span><content /></div>");
  Element* root = shadow_root->getElementById("root");
  Element* span = GetDocument().getElementById("inline");
  LayoutObject* previous_in_flow = ReattachLayoutTreeForNode(*root);

  EXPECT_TRUE(previous_in_flow);
  EXPECT_EQ(span->GetLayoutObject(), previous_in_flow);
}

TEST_F(NodeTest, HasMediaControlAncestor_Fail) {
  HTMLDivElement* node = HTMLDivElement::Create(GetDocument());
  EXPECT_FALSE(node->HasMediaControlAncestor());
  EXPECT_FALSE(InitializeUserAgentShadowTree(node)->HasMediaControlAncestor());
}

TEST_F(NodeTest, HasMediaControlAncestor_MediaControlElement) {
  FakeMediaControlElement* node = new FakeMediaControlElement(GetDocument());
  EXPECT_TRUE(node->HasMediaControlAncestor());
  EXPECT_TRUE(InitializeUserAgentShadowTree(node)->HasMediaControlAncestor());
}

TEST_F(NodeTest, HasMediaControlAncestor_MediaControls) {
  FakeMediaControls* node = new FakeMediaControls(GetDocument());
  EXPECT_TRUE(node->HasMediaControlAncestor());
  EXPECT_TRUE(InitializeUserAgentShadowTree(node)->HasMediaControlAncestor());
}

}  // namespace blink
