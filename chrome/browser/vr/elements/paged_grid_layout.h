// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_ELEMENTS_PAGED_GRID_LAYOUT_H_
#define CHROME_BROWSER_VR_ELEMENTS_PAGED_GRID_LAYOUT_H_

#include "base/macros.h"
#include "chrome/browser/vr/elements/ui_element.h"

namespace vr {

// A paged grid layout is comprized of parts, the pages (and adjacent wings).
//
//   a b c G H I m n o
//   d e f J K L p q r
//
// The capital letters are the current page of elements, and the lower case
// elements represent the elements in the wings. The order of the elements
// follows the alphabetical ordering pictured above.
//
// NOTE: it is assumed that each child view has the same dimensions.
class PagedGridLayout : public UiElement {
 public:
  enum PageState { kNone, kActive, kInactive, kHidden };

  PagedGridLayout(size_t rows, size_t columns, const gfx::SizeF& tile_size);
  ~PagedGridLayout() override;

  // UiElement overrides.
  void LayOutNonContributingChildren() override;

  size_t current_page() const { return current_page_; }
  // Sets the horizontal/vertical margin between elements and horizontal margin
  // between pages.
  void set_margin(float margin) { margin_ = margin; }

  void SetCurrentPage(size_t current_page);
  size_t NumPages() const;
  PageState GetPageState(UiElement* child);

 private:
  size_t rows_;
  size_t columns_;
  gfx::SizeF tile_size_;
  float margin_ = 0.0f;
  size_t current_page_ = 0;

  DISALLOW_COPY_AND_ASSIGN(PagedGridLayout);
};

}  // namespace vr

#endif  // CHROME_BROWSER_VR_ELEMENTS_PAGED_GRID_LAYOUT_H_
