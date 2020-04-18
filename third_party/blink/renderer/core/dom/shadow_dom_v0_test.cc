// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/dom/shadow_root.h"
#include "third_party/blink/renderer/core/dom/shadow_root_v0.h"
#include "third_party/blink/renderer/core/html/html_body_element.h"
#include "third_party/blink/renderer/core/testing/sim/sim_request.h"
#include "third_party/blink/renderer/core/testing/sim/sim_test.h"

namespace blink {

namespace {

bool HasSelectorForIdInShadow(Element* host, const AtomicString& id) {
  DCHECK(host);
  return host->GetShadowRoot()->V0().EnsureSelectFeatureSet().HasSelectorForId(
      id);
}

bool HasSelectorForClassInShadow(Element* host,
                                 const AtomicString& class_name) {
  DCHECK(host);
  return host->GetShadowRoot()
      ->V0()
      .EnsureSelectFeatureSet()
      .HasSelectorForClass(class_name);
}

bool HasSelectorForAttributeInShadow(Element* host,
                                     const AtomicString& attribute_name) {
  DCHECK(host);
  return host->GetShadowRoot()
      ->V0()
      .EnsureSelectFeatureSet()
      .HasSelectorForAttribute(attribute_name);
}

class ShadowDOMVTest : public SimTest {};

TEST_F(ShadowDOMVTest, FeatureSetId) {
  LoadURL("about:blank");
  auto* host = GetDocument().CreateRawElement(HTMLNames::divTag);
  auto* content = GetDocument().CreateRawElement(HTMLNames::contentTag);
  content->setAttribute("select", "#foo");
  host->CreateV0ShadowRootForTesting().AppendChild(content);
  EXPECT_TRUE(HasSelectorForIdInShadow(host, "foo"));
  EXPECT_FALSE(HasSelectorForIdInShadow(host, "bar"));
  EXPECT_FALSE(HasSelectorForIdInShadow(host, "host"));
  content->setAttribute("select", "#bar");
  EXPECT_TRUE(HasSelectorForIdInShadow(host, "bar"));
  EXPECT_FALSE(HasSelectorForIdInShadow(host, "foo"));
  content->setAttribute("select", "");
  EXPECT_FALSE(HasSelectorForIdInShadow(host, "bar"));
  EXPECT_FALSE(HasSelectorForIdInShadow(host, "foo"));
}

TEST_F(ShadowDOMVTest, FeatureSetClassName) {
  LoadURL("about:blank");
  auto* host = GetDocument().CreateRawElement(HTMLNames::divTag);
  auto* content = GetDocument().CreateRawElement(HTMLNames::contentTag);
  content->setAttribute("select", ".foo");
  host->CreateV0ShadowRootForTesting().AppendChild(content);
  EXPECT_TRUE(HasSelectorForClassInShadow(host, "foo"));
  EXPECT_FALSE(HasSelectorForClassInShadow(host, "bar"));
  EXPECT_FALSE(HasSelectorForClassInShadow(host, "host"));
  content->setAttribute("select", ".bar");
  EXPECT_TRUE(HasSelectorForClassInShadow(host, "bar"));
  EXPECT_FALSE(HasSelectorForClassInShadow(host, "foo"));
  content->setAttribute("select", "");
  EXPECT_FALSE(HasSelectorForClassInShadow(host, "bar"));
  EXPECT_FALSE(HasSelectorForClassInShadow(host, "foo"));
}

TEST_F(ShadowDOMVTest, FeatureSetAttributeName) {
  LoadURL("about:blank");
  auto* host = GetDocument().CreateRawElement(HTMLNames::divTag);
  auto* content = GetDocument().CreateRawElement(HTMLNames::contentTag);
  content->setAttribute("select", "div[foo]");
  host->CreateV0ShadowRootForTesting().AppendChild(content);
  EXPECT_TRUE(HasSelectorForAttributeInShadow(host, "foo"));
  EXPECT_FALSE(HasSelectorForAttributeInShadow(host, "bar"));
  EXPECT_FALSE(HasSelectorForAttributeInShadow(host, "host"));
  content->setAttribute("select", "div[bar]");
  EXPECT_TRUE(HasSelectorForAttributeInShadow(host, "bar"));
  EXPECT_FALSE(HasSelectorForAttributeInShadow(host, "foo"));
  content->setAttribute("select", "");
  EXPECT_FALSE(HasSelectorForAttributeInShadow(host, "bar"));
  EXPECT_FALSE(HasSelectorForAttributeInShadow(host, "foo"));
}

TEST_F(ShadowDOMVTest, FeatureSetMultipleSelectors) {
  LoadURL("about:blank");
  auto* host = GetDocument().CreateRawElement(HTMLNames::divTag);
  auto* content = GetDocument().CreateRawElement(HTMLNames::contentTag);
  content->setAttribute("select", "#foo,.bar,div[baz]");
  host->CreateV0ShadowRootForTesting().AppendChild(content);
  EXPECT_TRUE(HasSelectorForIdInShadow(host, "foo"));
  EXPECT_FALSE(HasSelectorForIdInShadow(host, "bar"));
  EXPECT_FALSE(HasSelectorForIdInShadow(host, "baz"));
  EXPECT_FALSE(HasSelectorForClassInShadow(host, "foo"));
  EXPECT_TRUE(HasSelectorForClassInShadow(host, "bar"));
  EXPECT_FALSE(HasSelectorForClassInShadow(host, "baz"));
  EXPECT_FALSE(HasSelectorForAttributeInShadow(host, "foo"));
  EXPECT_FALSE(HasSelectorForAttributeInShadow(host, "bar"));
  EXPECT_TRUE(HasSelectorForAttributeInShadow(host, "baz"));
}

TEST_F(ShadowDOMVTest, FeatureSetSubtree) {
  LoadURL("about:blank");
  auto* host = GetDocument().CreateRawElement(HTMLNames::divTag);
  host->CreateV0ShadowRootForTesting().SetInnerHTMLFromString(R"HTML(
    <div>
      <div></div>
      <content select='*'></content>
      <div>
        <content select='div[foo=piyo]'></content>
      </div>
    </div>
  )HTML");
  EXPECT_FALSE(HasSelectorForIdInShadow(host, "foo"));
  EXPECT_FALSE(HasSelectorForClassInShadow(host, "foo"));
  EXPECT_TRUE(HasSelectorForAttributeInShadow(host, "foo"));
  EXPECT_FALSE(HasSelectorForAttributeInShadow(host, "piyo"));
}

TEST_F(ShadowDOMVTest, FeatureSetMultipleShadowRoots) {
  LoadURL("about:blank");
  auto* host = GetDocument().CreateRawElement(HTMLNames::divTag);
  auto& host_shadow = host->CreateV0ShadowRootForTesting();
  host_shadow.SetInnerHTMLFromString("<content select='#foo'></content>");
  auto* child = GetDocument().CreateRawElement(HTMLNames::divTag);
  auto& child_root = child->CreateV0ShadowRootForTesting();
  auto* child_content = GetDocument().CreateRawElement(HTMLNames::contentTag);
  child_content->setAttribute("select", "#bar");
  child_root.AppendChild(child_content);
  host_shadow.AppendChild(child);
  EXPECT_TRUE(HasSelectorForIdInShadow(host, "foo"));
  EXPECT_TRUE(HasSelectorForIdInShadow(host, "bar"));
  EXPECT_FALSE(HasSelectorForIdInShadow(host, "baz"));
  child_content->setAttribute("select", "#baz");
  EXPECT_TRUE(HasSelectorForIdInShadow(host, "foo"));
  EXPECT_FALSE(HasSelectorForIdInShadow(host, "bar"));
  EXPECT_TRUE(HasSelectorForIdInShadow(host, "baz"));
}

}  // namespace

}  // namespace blink
