// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_PUBLIC_WEB_STATE_PAGE_DISPLAY_STATE_H_
#define IOS_WEB_PUBLIC_WEB_STATE_PAGE_DISPLAY_STATE_H_

#import <Foundation/Foundation.h>

namespace web {

// Class used to represent the scrolling offset of a webview.
class PageScrollState {
 public:
  // Default constructor.  Initializes scroll offsets to NAN.
  PageScrollState();
  // Constructor with initial values.
  PageScrollState(double offset_x, double offset_y);
  ~PageScrollState();

  // The scroll offset is valid if its x and y values are both non-NAN.
  bool IsValid() const;

  // Accessors for scroll offsets and zoom scale.
  double offset_x() const { return offset_x_; }
  void set_offset_x(double offset_x) { offset_x_ = offset_x; }
  double offset_y() const { return offset_y_; }
  void set_offset_y(double offset_y) { offset_y_ = offset_y; }

  // Comparator operators.
  bool operator==(const PageScrollState& other) const;
  bool operator!=(const PageScrollState& other) const;

 private:
  // The x value of the page's UIScrollView contentOffset.
  double offset_x_;
  // The y value of the page's UIScrollView contentOffset.
  double offset_y_;
};

// Class used to represent the scrolling offset and the zoom scale of a webview.
class PageZoomState {
 public:
  // Default constructor.  Initializes scroll offsets and zoom scales to NAN.
  PageZoomState();
  // Constructor with initial values.
  PageZoomState(double minimum_zoom_scale,
                double maximum_zoom_scale,
                double zoom_scale);
  ~PageZoomState();

  // Non-legacy zoom scales are valid if all three values are non-NAN and the
  // zoom scale is within the minimum and maximum scales.  Legacy-format
  // PageScrollStates are considered valid if the minimum and maximum scales
  // are NAN and the zoom scale is greater than zero.
  bool IsValid() const;

  // Returns the allowed zoom scale range for this scroll state.
  double GetMinMaxZoomDifference() const {
    return maximum_zoom_scale_ - minimum_zoom_scale_;
  }

  // Accessors.
  double minimum_zoom_scale() const { return minimum_zoom_scale_; }
  void set_minimum_zoom_scale(double minimum_zoom_scale) {
    minimum_zoom_scale_ = minimum_zoom_scale;
  }
  double maximum_zoom_scale() const { return maximum_zoom_scale_; }
  void set_maximum_zoom_scale(double maximum_zoom_scale) {
    maximum_zoom_scale_ = maximum_zoom_scale;
  }
  double zoom_scale() const { return zoom_scale_; }
  void set_zoom_scale(double zoom_scale) { zoom_scale_ = zoom_scale; }

  // Comparator operators.
  bool operator==(const PageZoomState& other) const;
  bool operator!=(const PageZoomState& other) const;

 private:
  // The minimumZoomScale value of the page's UIScrollView.
  double minimum_zoom_scale_;
  // The maximumZoomScale value of the page's UIScrollView.
  double maximum_zoom_scale_;
  // The zoomScale value of the page's UIScrollView.
  double zoom_scale_;
};

// Class used to represent the scroll offset and zoom scale of a webview.
class PageDisplayState {
 public:
  // Default constructor.  Initializes scroll offsets and zoom scales to NAN.
  PageDisplayState();
  // Constructor with initial values.
  PageDisplayState(const PageScrollState& scroll_state,
                   const PageZoomState& zoom_state);
  PageDisplayState(double offset_x,
                   double offset_y,
                   double minimum_zoom_scale,
                   double maximum_zoom_scale,
                   double zoom_scale);
  PageDisplayState(NSDictionary* serialization);
  ~PageDisplayState();

  // PageScrollStates cannot be applied until the scroll offset and zoom scale
  // are both valid.
  bool IsValid() const;

  // Accessors.
  const PageScrollState& scroll_state() const { return scroll_state_; }
  PageScrollState& scroll_state() { return scroll_state_; }
  void set_scroll_state(const PageScrollState& scroll_state) {
    scroll_state_ = scroll_state;
  }
  const PageZoomState& zoom_state() const { return zoom_state_; }
  PageZoomState& zoom_state() { return zoom_state_; }
  void set_zoom_state(const PageZoomState& zoom_state) {
    zoom_state_ = zoom_state;
  }

  // Comparator operators.
  bool operator==(const PageDisplayState& other) const;
  bool operator!=(const PageDisplayState& other) const;

  // Returns a serialized representation of the PageDisplayState.
  NSDictionary* GetSerialization() const;

  // Returns a description string for the PageDisplayState.
  NSString* GetDescription() const;

 private:
  // The scroll state for the page's UIScrollView.
  PageScrollState scroll_state_;
  // The zoom state for the page's UIScrollView.
  PageZoomState zoom_state_;
};

}  // namespace web

#endif  // IOS_WEB_PUBLIC_WEB_STATE_PAGE_DISPLAY_STATE_H_
