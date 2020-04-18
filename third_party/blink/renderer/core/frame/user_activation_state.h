// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_FRAME_USER_ACTIVATION_STATE_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_FRAME_USER_ACTIVATION_STATE_H_

#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/platform/wtf/time.h"

namespace blink {

// This class represents the user activation state of a frame.  It maintains two
// bits of information: whether this frame has ever seen an activation in its
// lifetime, and whether this frame has a current activation that was neither
// expired nor consumed.  The "owner" frame propagates both bits to ancestor
// frames.
//
// This provides a simple alternative to current user gesture tracking code
// based on UserGestureIndicator and UserGestureToken.
class CORE_EXPORT UserActivationState {
 public:
  void Activate();
  void Clear();

  // Returns the sticky activation state, which is |true| if
  // |UserActivationState| has ever seen an activation.
  bool HasBeenActive() const { return has_been_active_; }

  // Returns the transient activation state, which is |true| if
  // |UserActivationState| has recently been activated and the transient state
  // hasn't been consumed yet.
  bool IsActive();

  // Consumes the transient activation state if available, and returns |true| if
  // successfully consumed.
  bool ConsumeIfActive();

 private:
  bool has_been_active_ = false;
  bool is_active_ = false;
  TimeTicks activation_timestamp_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_FRAME_USER_ACTIVATION_STATE_H_
