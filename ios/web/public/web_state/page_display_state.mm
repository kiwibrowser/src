// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/web/public/web_state/page_display_state.h"

#include <cmath>

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace web {

namespace {
// Serialiation keys.
NSString* const kXOffsetKey = @"scrollX";
NSString* const kYOffsetKey = @"scrollY";
NSString* const kMinZoomKey = @"minZoom";
NSString* const kMaxZoomKey = @"maxZoom";
NSString* const kZoomKey = @"zoom";
// Returns true if:
// - both |value1| and |value2| are NAN, or
// - |value1| and |value2| are equal non-NAN values.
inline bool StateValuesAreEqual(double value1, double value2) {
  return std::isnan(value1) ? std::isnan(value2) : value1 == value2;
}
// Returns the double stored under |key| in |serialization|, or NAN if it is not
// set.
inline double GetValue(NSString* key, NSDictionary* serialization) {
  NSNumber* value = serialization[key];
  return value ? [value doubleValue] : NAN;
}
}  // namespace

PageScrollState::PageScrollState() : offset_x_(NAN), offset_y_(NAN) {
}

PageScrollState::PageScrollState(double offset_x, double offset_y)
    : offset_x_(offset_x), offset_y_(offset_y) {
}

PageScrollState::~PageScrollState() {
}

bool PageScrollState::IsValid() const {
  return !std::isnan(offset_x_) && !std::isnan(offset_y_);
}

bool PageScrollState::operator==(const PageScrollState& other) const {
  return StateValuesAreEqual(offset_x_, other.offset_x_) &&
         StateValuesAreEqual(offset_y_, other.offset_y_);
}

bool PageScrollState::operator!=(const PageScrollState& other) const {
  return !(*this == other);
}

PageZoomState::PageZoomState()
    : minimum_zoom_scale_(NAN), maximum_zoom_scale_(NAN), zoom_scale_(NAN) {
}

PageZoomState::PageZoomState(double minimum_zoom_scale,
                             double maximum_zoom_scale,
                             double zoom_scale)
    : minimum_zoom_scale_(minimum_zoom_scale),
      maximum_zoom_scale_(maximum_zoom_scale),
      zoom_scale_(zoom_scale) {
}

PageZoomState::~PageZoomState() {
}

bool PageZoomState::IsValid() const {
  return (!std::isnan(minimum_zoom_scale_) &&
          !std::isnan(maximum_zoom_scale_) && !std::isnan(zoom_scale_) &&
          zoom_scale_ >= minimum_zoom_scale_ &&
          zoom_scale_ <= maximum_zoom_scale_);
}

bool PageZoomState::operator==(const PageZoomState& other) const {
  return StateValuesAreEqual(minimum_zoom_scale_, other.minimum_zoom_scale_) &&
         StateValuesAreEqual(maximum_zoom_scale_, other.maximum_zoom_scale_) &&
         StateValuesAreEqual(zoom_scale_, other.zoom_scale_);
}

bool PageZoomState::operator!=(const PageZoomState& other) const {
  return !(*this == other);
}

PageDisplayState::PageDisplayState() {
}

PageDisplayState::PageDisplayState(const PageScrollState& scroll_state,
                                   const PageZoomState& zoom_state)
    : scroll_state_(scroll_state), zoom_state_(zoom_state) {
}

PageDisplayState::PageDisplayState(double offset_x,
                                   double offset_y,
                                   double minimum_zoom_scale,
                                   double maximum_zoom_scale,
                                   double zoom_scale)
    : scroll_state_(offset_x, offset_y),
      zoom_state_(minimum_zoom_scale, maximum_zoom_scale, zoom_scale) {
}

PageDisplayState::PageDisplayState(NSDictionary* serialization)
    : PageDisplayState(GetValue(kXOffsetKey, serialization),
                       GetValue(kYOffsetKey, serialization),
                       GetValue(kMinZoomKey, serialization),
                       GetValue(kMaxZoomKey, serialization),
                       GetValue(kZoomKey, serialization)) {}

PageDisplayState::~PageDisplayState() {
}

bool PageDisplayState::IsValid() const {
  return scroll_state_.IsValid() && zoom_state_.IsValid();
}

bool PageDisplayState::operator==(const PageDisplayState& other) const {
  return scroll_state_ == other.scroll_state_ &&
         zoom_state_ == other.zoom_state_;
}

bool PageDisplayState::operator!=(const PageDisplayState& other) const {
  return !(*this == other);
}

NSDictionary* PageDisplayState::GetSerialization() const {
  return @{
    kXOffsetKey : @(scroll_state_.offset_x()),
    kYOffsetKey : @(scroll_state_.offset_y()),
    kMinZoomKey : @(zoom_state_.minimum_zoom_scale()),
    kMaxZoomKey : @(zoom_state_.maximum_zoom_scale()),
    kZoomKey : @(zoom_state_.zoom_scale())
  };
}

NSString* PageDisplayState::GetDescription() const {
  NSString* const kPageScrollStateDescriptionFormat =
      @"{ scrollOffset:(%0.2f, %0.2f), zoomScaleRange:(%0.2f, %0.2f), "
      @"zoomScale:%0.2f }";
  return [NSString stringWithFormat:kPageScrollStateDescriptionFormat,
                                    scroll_state_.offset_x(),
                                    scroll_state_.offset_y(),
                                    zoom_state_.minimum_zoom_scale(),
                                    zoom_state_.maximum_zoom_scale(),
                                    zoom_state_.zoom_scale()];
}

}  // namespace web
