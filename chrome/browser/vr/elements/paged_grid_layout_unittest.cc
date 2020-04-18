// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <vector>

#include "base/memory/ptr_util.h"
#include "cc/test/geometry_test_utils.h"
#include "chrome/browser/vr/elements/paged_grid_layout.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace vr {

typedef std::vector<UiElement*> ElementVector;

namespace {

static constexpr float kElementWidth = 20.0f;
static constexpr float kElementHeight = 10.0f;

void AddChildren(PagedGridLayout* view, ElementVector* elements, size_t count) {
  for (size_t i = 0; i < count; ++i) {
    auto child = std::make_unique<UiElement>();
    child->SetSize(kElementWidth, kElementHeight);
    child->SetVisible(true);
    elements->push_back(child.get());
    view->AddChild(std::move(child));
  }
}

gfx::Point3F GetLayoutPosition(const UiElement& element) {
  gfx::Point3F position;
  element.LocalTransform().TransformPoint(&position);
  return position;
}

}  // namespace

TEST(PagedGridLayout, NoElements) {
  PagedGridLayout view(4lu, 4lu, gfx::SizeF(kElementWidth, kElementHeight));
  view.set_margin(0.05f);
  view.SizeAndLayOut();
  EXPECT_EQ(0lu, view.NumPages());
  EXPECT_EQ(0lu, view.current_page());
}

TEST(PagedGridLayout, SinglePage) {
  float margin = 0.5f;
  ElementVector elements;
  PagedGridLayout view(2lu, 1lu, gfx::SizeF(kElementWidth, kElementHeight));
  view.set_margin(margin);

  AddChildren(&view, &elements, 2lu);

  view.SizeAndLayOut();
  EXPECT_EQ(1lu, view.NumPages());
  EXPECT_EQ(0lu, view.current_page());

  EXPECT_POINT3F_EQ(gfx::Point3F(0.0f, 0.5f * (kElementHeight + margin), 0.0f),
                    GetLayoutPosition(*elements[0]));
  EXPECT_POINT3F_EQ(gfx::Point3F(0.0f, -0.5f * (kElementHeight + margin), 0.0f),
                    GetLayoutPosition(*elements[1]));
}

TEST(PagedGridLayout, UnfilledPage) {
  float margin = 0.5f;
  ElementVector elements;
  PagedGridLayout view(2lu, 1lu, gfx::SizeF(kElementWidth, kElementHeight));
  view.set_margin(margin);

  AddChildren(&view, &elements, 1lu);

  view.SizeAndLayOut();
  EXPECT_EQ(1lu, view.NumPages());
  EXPECT_EQ(0lu, view.current_page());

  // This test is very much like SinglePage but we've only added a single
  // UiElement rather than two. The grid should not attempt to center the
  // elements in the page, so the omission of the second element should have no
  // effect on the positioning of the first.
  EXPECT_POINT3F_EQ(gfx::Point3F(0.0f, 0.5f * (kElementHeight + margin), 0.0f),
                    GetLayoutPosition(*elements[0]));
}

TEST(PagedGridLayout, MultiplePages) {
  float margin = 0.5f;
  ElementVector elements;
  PagedGridLayout view(2lu, 1lu, gfx::SizeF(kElementWidth, kElementHeight));
  view.set_margin(margin);

  AddChildren(&view, &elements, 3lu);

  view.SizeAndLayOut();
  EXPECT_EQ(margin * 1 + kElementWidth * 2, view.size().width());
  EXPECT_EQ(margin * 1 + kElementHeight * 2, view.size().height());
  EXPECT_EQ(2lu, view.NumPages());
  EXPECT_EQ(0lu, view.current_page());

  // This test is very much like SinglePage but we've added three UiElement
  // instances rather than two. The addition of the third element should have no
  // effect on the positioning of the first two, but it should cause the
  // addition of another page.
  EXPECT_POINT3F_EQ(gfx::Point3F(0.5f * (kElementWidth - view.size().width()),
                                 0.5f * (kElementHeight + margin), 0.0f),
                    GetLayoutPosition(*elements[0]));
  EXPECT_POINT3F_EQ(gfx::Point3F(0.5f * (kElementWidth - view.size().width()),
                                 -0.5f * (kElementHeight + margin), 0.0f),
                    GetLayoutPosition(*elements[1]));
  EXPECT_POINT3F_EQ(gfx::Point3F(0.5f * (kElementWidth + margin),
                                 0.5f * (kElementHeight + margin), 0.0f),
                    GetLayoutPosition(*elements[2]));

  // Setting the current page should have no effect on the local positioning of
  // the child elements. Rather, it should have an impact on the inheritable
  // transform provided by the parent. I.e., it will adjust the transform of
  // |view|.
  view.SetCurrentPage(1lu);
  view.SizeAndLayOut();
  EXPECT_POINT3F_EQ(gfx::Point3F(0.5f * (kElementWidth - view.size().width()),
                                 0.5f * (kElementHeight + margin), 0.0f),
                    GetLayoutPosition(*elements[0]));
  EXPECT_POINT3F_EQ(gfx::Point3F(0.5f * (kElementWidth - view.size().width()),
                                 -0.5f * (kElementHeight + margin), 0.0f),
                    GetLayoutPosition(*elements[1]));
  EXPECT_POINT3F_EQ(gfx::Point3F(0.5f * (kElementWidth + margin),
                                 0.5f * (kElementHeight + margin), 0.0f),
                    GetLayoutPosition(*elements[2]));

  // If we remove an element from the layout, we should reduce the number of
  // pages back to one. This will cause the current page to become invalid. We
  // should see the grid layout update the current page and related transform in
  // response. Again, this should have no impact on the laid out position of the
  // children.
  view.RemoveChild(elements.back());
  view.SizeAndLayOut();

  EXPECT_EQ(1lu, view.NumPages());
  EXPECT_EQ(0lu, view.current_page());
  EXPECT_POINT3F_EQ(gfx::Point3F(0.0f, 0.5f * (kElementHeight + margin), 0.0f),
                    GetLayoutPosition(*elements[0]));
  EXPECT_POINT3F_EQ(gfx::Point3F(0.0f, -0.5f * (kElementHeight + margin), 0.0f),
                    GetLayoutPosition(*elements[1]));
  EXPECT_POINT3F_EQ(gfx::Point3F(0.0f, 0.0f, 0.0f), GetLayoutPosition(view));
}

TEST(PagedGridLayout, LayoutOrder) {
  float margin = 0.5f;
  PagedGridLayout view(2lu, 2lu, gfx::SizeF(kElementWidth, kElementHeight));
  view.set_margin(margin);
  ElementVector elements;

  AddChildren(&view, &elements, 8lu);

  view.SizeAndLayOut();
  EXPECT_EQ(2lu, view.NumPages());
  EXPECT_EQ(0lu, view.current_page());

  EXPECT_POINT3F_EQ(gfx::Point3F(-0.5f * (kElementWidth + margin),
                                 0.5f * (kElementHeight + margin), 0.0f),
                    GetLayoutPosition(*elements[1]));
  EXPECT_POINT3F_EQ(gfx::Point3F(0.5f * (kElementWidth + margin),
                                 -0.5f * (kElementHeight + margin), 0.0f),
                    GetLayoutPosition(*elements[6]));
}

}  // namespace vr
