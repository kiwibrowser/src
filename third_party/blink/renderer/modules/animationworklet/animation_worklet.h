// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_ANIMATIONWORKLET_ANIMATION_WORKLET_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_ANIMATIONWORKLET_ANIMATION_WORKLET_H_

#include "third_party/blink/renderer/core/workers/worklet.h"
#include "third_party/blink/renderer/modules/modules_export.h"
#include "third_party/blink/renderer/platform/heap/handle.h"

namespace blink {

class Document;

// Represents the animation worklet on the main thread. All the logic for
// loading a new source module is implemented in its parent class |Worklet|. The
// sole responsibility of this class it to create the appropriate
// |WorkletGlobalScopeProxy| instances that are responsible to proxy a
// corresponding |AnimationWorkletGlobalScope| on the worklet thread.
class MODULES_EXPORT AnimationWorklet final : public Worklet {
  WTF_MAKE_NONCOPYABLE(AnimationWorklet);

 public:
  explicit AnimationWorklet(Document*);
  ~AnimationWorklet() override;

  void Trace(blink::Visitor*) override;

 private:

  // Implements Worklet.
  bool NeedsToCreateGlobalScope() final;
  WorkletGlobalScopeProxy* CreateGlobalScope() final;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_ANIMATIONWORKLET_ANIMATION_WORKLET_H_
