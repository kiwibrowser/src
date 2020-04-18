// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/test/fake_painted_scrollbar_layer.h"

#include "base/auto_reset.h"
#include "cc/test/fake_scrollbar.h"

namespace cc {

scoped_refptr<FakePaintedScrollbarLayer> FakePaintedScrollbarLayer::Create(
    bool paint_during_update,
    bool has_thumb,
    ElementId scrolling_element_id) {
  return Create(paint_during_update, has_thumb, HORIZONTAL, false, false,
                scrolling_element_id);
}

scoped_refptr<FakePaintedScrollbarLayer> FakePaintedScrollbarLayer::Create(
    bool paint_during_update,
    bool has_thumb,
    ScrollbarOrientation orientation,
    bool is_left_side_vertical_scrollbar,
    bool is_overlay,
    ElementId scrolling_element_id) {
  FakeScrollbar* fake_scrollbar =
      new FakeScrollbar(paint_during_update, has_thumb, orientation,
                        is_left_side_vertical_scrollbar, is_overlay);
  return base::WrapRefCounted(
      new FakePaintedScrollbarLayer(fake_scrollbar, scrolling_element_id));
}

FakePaintedScrollbarLayer::FakePaintedScrollbarLayer(
    FakeScrollbar* fake_scrollbar,
    ElementId scrolling_element_id)
    : PaintedScrollbarLayer(std::unique_ptr<Scrollbar>(fake_scrollbar),
                            scrolling_element_id),
      update_count_(0),
      push_properties_count_(0),
      fake_scrollbar_(fake_scrollbar) {
  SetBounds(gfx::Size(1, 1));
  SetIsDrawable(true);
}

FakePaintedScrollbarLayer::~FakePaintedScrollbarLayer() = default;

bool FakePaintedScrollbarLayer::Update() {
  bool updated = PaintedScrollbarLayer::Update();
  ++update_count_;
  return updated;
}

void FakePaintedScrollbarLayer::PushPropertiesTo(LayerImpl* layer) {
  PaintedScrollbarLayer::PushPropertiesTo(layer);
  ++push_properties_count_;
}

std::unique_ptr<base::AutoReset<bool>>
FakePaintedScrollbarLayer::IgnoreSetNeedsCommit() {
  return std::make_unique<base::AutoReset<bool>>(&ignore_set_needs_commit_,
                                                 true);
}

}  // namespace cc
