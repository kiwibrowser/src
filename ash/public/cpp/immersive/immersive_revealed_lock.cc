// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/public/cpp/immersive/immersive_revealed_lock.h"

namespace ash {

ImmersiveRevealedLock::ImmersiveRevealedLock(
    const base::WeakPtr<Delegate>& delegate,
    Delegate::AnimateReveal animate_reveal)
    : delegate_(delegate) {
  delegate_->LockRevealedState(animate_reveal);
}

ImmersiveRevealedLock::~ImmersiveRevealedLock() {
  if (delegate_)
    delegate_->UnlockRevealedState();
}

}  // namespace ash
