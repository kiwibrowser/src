// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_PUBLIC_CPP_IMMERSIVE_IMMERSIVE_REVEALED_LOCK_H_
#define ASH_PUBLIC_CPP_IMMERSIVE_IMMERSIVE_REVEALED_LOCK_H_

#include "ash/public/cpp/ash_public_export.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"

namespace ash {

// Class which keeps the top-of-window views revealed for the duration of its
// lifetime. If acquiring the lock causes a reveal, the top-of-window views
// will animate according to the |animate_reveal| parameter passed in the
// constructor. See ImmersiveFullscreenController::GetRevealedLock() for more
// details.
class ASH_PUBLIC_EXPORT ImmersiveRevealedLock {
 public:
  class ASH_PUBLIC_EXPORT Delegate {
   public:
    enum AnimateReveal { ANIMATE_REVEAL_YES, ANIMATE_REVEAL_NO };

    virtual void LockRevealedState(AnimateReveal animate_reveal) = 0;
    virtual void UnlockRevealedState() = 0;

   protected:
    virtual ~Delegate() {}
  };

  ImmersiveRevealedLock(const base::WeakPtr<Delegate>& delegate,
                        Delegate::AnimateReveal animate_reveal);
  ~ImmersiveRevealedLock();

 private:
  base::WeakPtr<Delegate> delegate_;

  DISALLOW_COPY_AND_ASSIGN(ImmersiveRevealedLock);
};

}  // namespace ash

#endif  // ASH_PUBLIC_CPP_IMMERSIVE_IMMERSIVE_REVEALED_LOCK_H_
