// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_ELEMENTS_PAGED_SCROLL_VIEW_H_
#define CHROME_BROWSER_VR_ELEMENTS_PAGED_SCROLL_VIEW_H_

#include "base/macros.h"
#include "chrome/browser/vr/elements/ui_element.h"
#include "chrome/browser/vr/transition.h"

namespace vr {

// Scrolls children added via |AddScrollingChild| horizontally. After scrolling
// ends it will snap into a page.
class PagedScrollView : public UiElement {
 public:
  explicit PagedScrollView(float page_width);
  ~PagedScrollView() override;

  // UiElement overrides.
  void OnScrollBegin(std::unique_ptr<blink::WebGestureEvent> gesture,
                     const gfx::PointF& position) override;
  void OnScrollUpdate(std::unique_ptr<blink::WebGestureEvent> gesture,
                      const gfx::PointF& position) override;
  void OnScrollEnd(std::unique_ptr<blink::WebGestureEvent> gesture,
                   const gfx::PointF& position) override;
  void NotifyClientFloatAnimated(float value,
                                 int target_property_id,
                                 cc::KeyframeModel* keyframe_model) override;
  void LayOutNonContributingChildren() override;

  size_t current_page() const { return current_page_; }
  void set_margin(float margin) { margin_ = margin; }
  void AddScrollingChild(std::unique_ptr<UiElement> child);
  size_t NumPages() const;

 private:
  void SetScrollOffset(float offset);
  void SetCurrentPage(size_t current_page);

  float page_width_;
  float margin_ = 0.0f;
  size_t current_page_ = 0;
  float scroll_offset_ = 0.0f;
  float scroll_drag_delta_ = 0.0f;

  UiElement* scrolling_element_;

  DISALLOW_COPY_AND_ASSIGN(PagedScrollView);
};

}  // namespace vr

#endif  // CHROME_BROWSER_VR_ELEMENTS_PAGED_SCROLL_VIEW_H_
