// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_ANIMATION_SCROLL_TIMELINE_H_
#define CC_ANIMATION_SCROLL_TIMELINE_H_

#include "cc/animation/animation_export.h"
#include "cc/trees/element_id.h"

namespace cc {

class ScrollTree;

// A ScrollTimeline is an animation timeline that bases its current time on the
// progress of scrolling in some scroll container.
//
// This is the compositor-side representation of the web concept expressed in
// https://wicg.github.io/scroll-animations/#scrolltimeline-interface. There are
// differences between this class and the web definition of a ScrollTimeline.
// For example the compositor does not know (or care) about 'writing modes', so
// this class only tracks whether the ScrollTimeline orientation is horizontal
// or vertical. Blink is expected to resolve any such 'web' requirements and
// construct/update the compositor ScrollTimeline accordingly.
class CC_ANIMATION_EXPORT ScrollTimeline {
 public:
  enum ScrollDirection { Horizontal, Vertical };

  ScrollTimeline(ElementId scroller_id,
                 ScrollDirection orientation,
                 double time_range);
  virtual ~ScrollTimeline() {}

  // Create a copy of this ScrollTimeline intended for the impl thread in the
  // compositor.
  std::unique_ptr<ScrollTimeline> CreateImplInstance() const;

  // Calculate the current time of the ScrollTimeline. This is either a double
  // value or std::numeric_limits<double>::quiet_NaN() if the current time is
  // unresolved.
  virtual double CurrentTime(const ScrollTree& scroll_tree) const;

 private:
  // The scroller which this ScrollTimeline is based on. It is expected that
  // this scroller will exist in the scroll property tree, or otherwise calling
  // CurrentTime will fail.
  ElementId scroller_id_;

  // The orientation of the ScrollTimeline indicates which axis of the scroller
  // it should base its current time on - either the horizontal or vertical.
  ScrollDirection orientation_;

  // A ScrollTimeline maps from the scroll offset in the scroller to a time
  // value based on a 'time range'. See the implementation of CurrentTime or the
  // spec for details.
  double time_range_;
};

}  // namespace cc

#endif  // CC_ANIMATION_SCROLL_TIMELINE_H_
