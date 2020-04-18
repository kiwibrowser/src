// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_BOX_DECORATION_DATA_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_BOX_DECORATION_DATA_H_

#include "third_party/blink/renderer/core/layout/background_bleed_avoidance.h"
#include "third_party/blink/renderer/platform/graphics/color.h"

namespace blink {

class LayoutBox;
class LayoutObject;
class Document;
class NGPhysicalFragment;
class ComputedStyle;

// Information extracted from ComputedStyle for box painting.
struct BoxDecorationData {
  STACK_ALLOCATED();

 public:
  BoxDecorationData(const LayoutBox&);
  BoxDecorationData(const NGPhysicalFragment&);

  Color background_color;
  BackgroundBleedAvoidance bleed_avoidance;
  bool has_background;
  bool has_border_decoration;
  bool has_appearance;

 private:
  BackgroundBleedAvoidance DetermineBackgroundBleedAvoidance(
      const Document&,
      const ComputedStyle&,
      bool background_should_always_be_clipped);
  BackgroundBleedAvoidance ComputeBleedAvoidance(const LayoutObject*);

  BoxDecorationData(const ComputedStyle&);
};

}  // namespace blink

#endif
