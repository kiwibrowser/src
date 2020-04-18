// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/accessibility/ax_aura_obj_cache.h"
#include "ui/views/test/widget_test.h"

namespace views {
namespace test {

namespace {

// This class can be used as a deleter for std::unique_ptr<Widget>
// to call function Widget::CloseNow automatically.
struct WidgetCloser {
  inline void operator()(Widget* widget) const { widget->CloseNow(); }
};

using WidgetAutoclosePtr = std::unique_ptr<Widget, WidgetCloser>;
}

class AXAuraObjCacheTest : public WidgetTest {
 public:
  AXAuraObjCacheTest() {}
  ~AXAuraObjCacheTest() override {}
};

TEST_F(AXAuraObjCacheTest, TestViewRemoval) {
  WidgetAutoclosePtr widget(CreateTopLevelPlatformWidget());
  View* parent = new View();
  widget->GetRootView()->AddChildView(parent);
  View* child = new View();
  parent->AddChildView(child);

  AXAuraObjCache* cache = AXAuraObjCache::GetInstance();
  AXAuraObjWrapper* ax_widget = cache->GetOrCreate(widget.get());
  ASSERT_NE(nullptr, ax_widget);
  AXAuraObjWrapper* ax_parent = cache->GetOrCreate(parent);
  ASSERT_NE(nullptr, ax_parent);
  AXAuraObjWrapper* ax_child = cache->GetOrCreate(child);
  ASSERT_NE(nullptr, ax_child);

  // Everything should have an ID, indicating it's in the cache.
  ASSERT_GT(cache->GetID(widget.get()), 0);
  ASSERT_GT(cache->GetID(parent), 0);
  ASSERT_GT(cache->GetID(child), 0);

  // Removing the parent view should remove both the parent and child
  // from the cache, but leave the widget.
  widget->GetRootView()->RemoveChildView(parent);
  ASSERT_GT(cache->GetID(widget.get()), 0);
  ASSERT_EQ(-1, cache->GetID(parent));
  ASSERT_EQ(-1, cache->GetID(child));

  // Explicitly delete |parent| to prevent a memory leak, since calling
  // RemoveChildView() doesn't delete it.
  delete parent;
}

}  // namespace test
}  // namespace views
