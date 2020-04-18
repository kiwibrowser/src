// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_ANIMATION_WORKLET_ANIMATION_BASE_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_ANIMATION_WORKLET_ANIMATION_BASE_H_

#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/platform/bindings/script_wrappable.h"
#include "third_party/blink/renderer/platform/wtf/forward.h"

namespace blink {

class Document;
class KeyframeEffect;

class CORE_EXPORT WorkletAnimationBase : public ScriptWrappable {
 public:
  ~WorkletAnimationBase() override = default;

  // Asks the animation to update its effect inherited time.
  virtual void Update(TimingUpdateReason) = 0;

  // Updates the animation on the compositor side to reflect the main thread
  // state.
  //
  // Returns true if animation is started on compositor and should now start
  // receiving timing update requests.
  virtual bool UpdateCompositingState() = 0;

  virtual Document* GetDocument() const = 0;
  virtual KeyframeEffect* GetEffect() const = 0;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_ANIMATION_WORKLET_ANIMATION_BASE_H_
