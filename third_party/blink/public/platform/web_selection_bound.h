// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_SELECTION_BOUND_H_
#define THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_SELECTION_BOUND_H_

#include "third_party/blink/public/platform/web_point.h"

namespace blink {

// An endpoint for an active selection region.
// TODO(jdduke): Move this to web/ after downstream code adopts |WebSelection|.
struct WebSelectionBound {
  // TODO(jdduke): Remove the type identifier after downstream code adopts
  // |WebSelection| for determining bound orientation.
  enum Type { kCaret, kSelectionLeft, kSelectionRight };

#if INSIDE_BLINK
  WebSelectionBound() = default;
#endif

  explicit WebSelectionBound(Type type) : type(type) {}

  // The logical type of the endpoint. Note that this is dependent not only on
  // the bound's relative location, but also the underlying text direction.
  Type type = kCaret;

  // The id of the platform layer to which the bound should be anchored.
  int layer_id = 0;

  // The bottom and top coordinates of the edge (caret), in layer coordinates,
  // that define the selection bound.
  WebPoint edge_top_in_layer;
  WebPoint edge_bottom_in_layer;

  // Whether the text direction at this location is RTL.
  bool is_text_direction_rtl = false;

  // Whether this bound is hidden (clipped out/occluded).
  bool hidden = false;
};

}  // namespace blink

#endif
