// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/html/html_frame_element.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/core/dom/document.h"

namespace blink {

class HTMLFrameElementTest : public testing::Test {};

// Test that the correct container policy is constructed on a frame element.
// Frame elements do not have any container-policy related attributes, but the
// fullscreen feature should be unconditionally disabled.
TEST_F(HTMLFrameElementTest, DefaultContainerPolicy) {
  Document* document = Document::CreateForTest();
  const KURL document_url("http://example.com");
  document->SetURL(document_url);
  document->UpdateSecurityOrigin(SecurityOrigin::Create(document_url));

  HTMLFrameElement* frame_element = HTMLFrameElement::Create(*document);

  frame_element->setAttribute(HTMLNames::srcAttr, "http://example.net/");
  frame_element->UpdateContainerPolicyForTests();

  const ParsedFeaturePolicy& container_policy =
      frame_element->ContainerPolicy();
  EXPECT_EQ(1UL, container_policy.size());
  // Fullscreen should be disabled in this frame
  EXPECT_EQ(mojom::FeaturePolicyFeature::kFullscreen,
            container_policy[0].feature);
  EXPECT_FALSE(container_policy[0].matches_all_origins);
  EXPECT_EQ(0UL, container_policy[0].origins.size());
}

}  // namespace blink
