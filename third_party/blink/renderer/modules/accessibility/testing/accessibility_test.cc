// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/accessibility/testing/accessibility_test.h"

#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/frame/local_frame_client.h"
#include "third_party/blink/renderer/core/frame/settings.h"
#include "third_party/blink/renderer/core/layout/layout_view.h"
#include "third_party/blink/renderer/modules/accessibility/ax_object.h"
#include "third_party/blink/renderer/modules/accessibility/ax_object_cache_impl.h"

namespace blink {

void AccessibilityTest::SetUp() {
  RenderingTest::SetUp();
  DCHECK(GetDocument().GetSettings());
  GetDocument().GetSettings()->SetAccessibilityEnabled(true);
}

void AccessibilityTest::TearDown() {
  GetDocument().ClearAXObjectCache();
  DCHECK(GetDocument().GetSettings());
  GetDocument().GetSettings()->SetAccessibilityEnabled(false);
  RenderingTest::TearDown();
}

AXObjectCacheImpl& AccessibilityTest::GetAXObjectCache() const {
  auto* ax_object_cache =
      ToAXObjectCacheImpl(GetDocument().GetOrCreateAXObjectCache());
  DCHECK(ax_object_cache);
  return *ax_object_cache;
}

AXObject* AccessibilityTest::GetAXRootObject() const {
  return GetAXObjectCache().GetOrCreate(&GetLayoutView());
}

AXObject* AccessibilityTest::GetAXFocusedObject() const {
  return GetAXObjectCache().FocusedObject();
}

AXObject* AccessibilityTest::GetAXObjectByElementId(const char* id) const {
  const auto* element = GetElementById(id);
  return element ? GetAXObjectCache().GetOrCreate(element) : nullptr;
}

std::string AccessibilityTest::PrintAXTree() const {
  std::ostringstream stream;
  PrintAXTreeHelper(stream, GetAXRootObject(), 0);
  return stream.str();
}

std::ostringstream& AccessibilityTest::PrintAXTreeHelper(
    std::ostringstream& stream,
    const AXObject* root,
    size_t level) const {
  if (!root)
    return stream;

  stream << std::string(level * 2, '+');
  stream << *root << std::endl;
  for (const Member<AXObject> child : root->Children()) {
    DCHECK(child);
    PrintAXTreeHelper(stream, child.Get(), level + 1);
  }
  return stream;
}

}  // namespace blink
