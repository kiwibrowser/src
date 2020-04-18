// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/elements/paged_scroll_view.h"

#include "chrome/browser/vr/target_property.h"
#include "third_party/blink/public/platform/web_gesture_event.h"

namespace vr {

namespace {
constexpr float kScrollScaleFactor = 1.0f / 400.0f;
constexpr float kScrollThreshold = 0.2f;
}  // namespace

PagedScrollView::PagedScrollView(float page_width) : page_width_(page_width) {
  auto scrolling_element = std::make_unique<UiElement>();
  scrolling_element->set_bounds_contain_children(true);
  scrolling_element_ = scrolling_element.get();
  AddChild(std::move(scrolling_element));
}

PagedScrollView::~PagedScrollView() {}

void PagedScrollView::OnScrollBegin(
    std::unique_ptr<blink::WebGestureEvent> gesture,
    const gfx::PointF& position) {
  animation().RemoveKeyframeModels(SCROLL_OFFSET);
  scroll_drag_delta_ = 0.0f;
}

void PagedScrollView::OnScrollUpdate(
    std::unique_ptr<blink::WebGestureEvent> gesture,
    const gfx::PointF& position) {
  scroll_drag_delta_ +=
      gesture->data.scroll_update.delta_x * kScrollScaleFactor;
}

void PagedScrollView::OnScrollEnd(
    std::unique_ptr<blink::WebGestureEvent> gesture,
    const gfx::PointF& position) {
  size_t next_page = current_page_;
  if (next_page + 1 < NumPages() && scroll_drag_delta_ < -kScrollThreshold) {
    next_page++;
  } else if (next_page > 0 && scroll_drag_delta_ > kScrollThreshold) {
    next_page--;
  }
  scroll_offset_ += scroll_drag_delta_;
  scroll_drag_delta_ = 0;
  SetCurrentPage(next_page);
}

void PagedScrollView::NotifyClientFloatAnimated(
    float value,
    int target_property_id,
    cc::KeyframeModel* keyframe_model) {
  if (target_property_id == SCROLL_OFFSET) {
    scroll_offset_ = value;
  } else {
    UiElement::NotifyClientFloatAnimated(value, target_property_id,
                                         keyframe_model);
  }
}

void PagedScrollView::LayOutNonContributingChildren() {
  scrolling_element_->SetLayoutOffset(
      (scrolling_element_->size().width() - page_width_) * 0.5f +
          scroll_offset_ + scroll_drag_delta_,
      0);
}

void PagedScrollView::AddScrollingChild(std::unique_ptr<UiElement> child) {
  scrolling_element_->AddChild(std::move(child));
}

size_t PagedScrollView::NumPages() const {
  return std::floor(scrolling_element_->size().width() / page_width_);
}

void PagedScrollView::SetScrollOffset(float offset) {
  animation().TransitionFloatTo(
      last_frame_time(), TargetProperty::SCROLL_OFFSET, scroll_offset_, offset);
}

void PagedScrollView::SetCurrentPage(size_t current_page) {
  DCHECK(current_page == 0 || (NumPages() > 0 && current_page < NumPages()));
  current_page_ = current_page;
  SetScrollOffset(current_page_ * -(page_width_ + margin_));
}

}  // namespace vr
