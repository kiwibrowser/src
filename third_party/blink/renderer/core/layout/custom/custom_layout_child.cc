// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/layout/custom/custom_layout_child.h"

#include "third_party/blink/renderer/core/css/cssom/prepopulated_computed_style_property_map.h"
#include "third_party/blink/renderer/core/layout/custom/css_layout_definition.h"
#include "third_party/blink/renderer/core/layout/custom/custom_layout_fragment_request.h"
#include "third_party/blink/renderer/core/layout/layout_box.h"

namespace blink {

CustomLayoutChild::CustomLayoutChild(const CSSLayoutDefinition& definition,
                                     LayoutBox* box)
    : box_(box),
      style_map_(new PrepopulatedComputedStylePropertyMap(
          box->GetDocument(),
          box->StyleRef(),
          box->GetNode(),
          definition.ChildNativeInvalidationProperties(),
          definition.ChildCustomInvalidationProperties())) {}

CustomLayoutFragmentRequest* CustomLayoutChild::layoutNextFragment(
    const CustomLayoutConstraintsOptions& options) {
  return new CustomLayoutFragmentRequest(this, options);
}

void CustomLayoutChild::Trace(blink::Visitor* visitor) {
  visitor->Trace(style_map_);
  ScriptWrappable::Trace(visitor);
}

}  // namespace blink
