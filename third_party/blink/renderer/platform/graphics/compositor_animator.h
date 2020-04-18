// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_COMPOSITOR_ANIMATOR_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_COMPOSITOR_ANIMATOR_H_

#include "third_party/blink/renderer/platform/graphics/compositor_animators_state.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/platform_export.h"

namespace blink {

class PLATFORM_EXPORT CompositorAnimator : public GarbageCollectedMixin {
 public:
  // Runs the animation frame callback.
  virtual std::unique_ptr<CompositorMutatorOutputState> Mutate(
      const CompositorMutatorInputState&) = 0;
  void Trace(blink::Visitor* visitor) override {}
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_COMPOSITOR_ANIMATOR_H_
