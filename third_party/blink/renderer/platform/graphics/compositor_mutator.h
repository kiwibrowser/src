// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_COMPOSITOR_MUTATOR_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_COMPOSITOR_MUTATOR_H_

#include "third_party/blink/renderer/platform/graphics/compositor_animators_state.h"
#include "third_party/blink/renderer/platform/platform_export.h"

namespace blink {

class PLATFORM_EXPORT CompositorMutator {
 public:
  virtual ~CompositorMutator() = default;

  // Called from compositor thread to run the animation frame callbacks from all
  // connected AnimationWorklets.
  virtual void Mutate(std::unique_ptr<CompositorMutatorInputState>) = 0;
  // Returns true if Mutate may do something if called 'now'.
  virtual bool HasAnimators() = 0;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_COMPOSITOR_MUTATOR_H_
