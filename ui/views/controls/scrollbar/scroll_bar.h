// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_CONTROLS_SCROLLBAR_SCROLL_BAR_H_
#define UI_VIEWS_CONTROLS_SCROLLBAR_SCROLL_BAR_H_

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "ui/views/view.h"
#include "ui/views/views_export.h"

namespace views {

class ScrollBar;

/////////////////////////////////////////////////////////////////////////////
//
// ScrollBarController
//
// ScrollBarController defines the method that should be implemented to
// receive notification from a scrollbar
//
/////////////////////////////////////////////////////////////////////////////
class VIEWS_EXPORT ScrollBarController {
 public:
  // Invoked by the scrollbar when the scrolling position changes
  // This method typically implements the actual scrolling.
  //
  // The provided position is expressed in pixels. It is the new X or Y
  // position which is in the GetMinPosition() / GetMaxPosition range.
  virtual void ScrollToPosition(ScrollBar* source, int position) = 0;

  // Returns the amount to scroll. The amount to scroll may be requested in
  // two different amounts. If is_page is true the 'page scroll' amount is
  // requested. The page scroll amount typically corresponds to the
  // visual size of the view. If is_page is false, the 'line scroll' amount
  // is being requested. The line scroll amount typically corresponds to the
  // size of one row/column.
  //
  // The return value should always be positive. A value <= 0 results in
  // scrolling by a fixed amount.
  virtual int GetScrollIncrement(ScrollBar* source,
                                 bool is_page,
                                 bool is_positive) = 0;
};

/////////////////////////////////////////////////////////////////////////////
//
// ScrollBar
//
// A View subclass to wrap to implement a ScrollBar. Our current windows
// version simply wraps a native windows scrollbar.
//
// A scrollbar is either horizontal or vertical
//
/////////////////////////////////////////////////////////////////////////////
class VIEWS_EXPORT ScrollBar : public View {
 public:
  ~ScrollBar() override;

  // Overridden from View:
  void GetAccessibleNodeData(ui::AXNodeData* node_data) override;

  // Returns whether this scrollbar is horizontal.
  bool IsHorizontal() const;

  void set_controller(ScrollBarController* controller) {
     controller_ = controller;
  }
  ScrollBarController* controller() const { return controller_; }

  // Update the scrollbar appearance given a viewport size, content size and
  // current position
  virtual void Update(int viewport_size, int content_size, int current_pos);

  // Returns the max and min positions.
  int GetMaxPosition() const;
  int GetMinPosition() const;

  // Returns the position of the scrollbar.
  virtual int GetPosition() const = 0;

  // Get the width or height of this scrollbar. For a vertical scrollbar, this
  // is the width of the scrollbar, likewise it is the height for a horizontal
  // scrollbar.
  virtual int GetThickness() const = 0;

  // Returns true if the scrollbar should sit on top of the content area (e.g.
  // for overlay scrollbars).
  virtual bool OverlapsContent() const = 0;

  // Called when a ScrollEvent (in any, or no, direction) is seen by the parent
  // ScrollView. E.g., this may reveal an overlay scrollbar to indicate
  // possible scrolling directions to the user.
  virtual void ObserveScrollEvent(const ui::ScrollEvent& event);

 protected:
  // Create new scrollbar, either horizontal or vertical. These are protected
  // since you need to be creating either a NativeScrollBar or a
  // ImageScrollBar.
  explicit ScrollBar(bool is_horiz);

 private:
  const bool is_horiz_;

  ScrollBarController* controller_;

  int max_pos_;

  DISALLOW_COPY_AND_ASSIGN(ScrollBar);
};

}  // namespace views

#endif  // UI_VIEWS_CONTROLS_SCROLLBAR_SCROLL_BAR_H_
