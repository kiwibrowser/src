// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_WM_LOCK_STATE_CONTROLLER_H_
#define ASH_WM_LOCK_STATE_CONTROLLER_H_

#include <memory>

#include "ash/ash_export.h"
#include "ash/session/session_observer.h"
#include "ash/shutdown_reason.h"
#include "ash/wm/session_state_animator.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/optional.h"
#include "base/time/time.h"
#include "base/timer/elapsed_timer.h"
#include "base/timer/timer.h"
#include "ui/aura/window_tree_host_observer.h"

namespace ash {

class ShutdownController;
enum class ShutdownReason;

// Displays onscreen animations and locks or suspends the system in response to
// the power button being pressed or released.
// Lock workflow:
// Entry points:
//  * StartLockAnimation (bool shutdown after lock) - starts lock that can be
//    cancelled.
//  * StartLockAnimationAndLockImmediately (bool shutdown after lock) - starts
//    uninterruptible lock animation.
// This leads to call of either StartImmediatePreLockAnimation or
// StartCancellablePreLockAnimation. Once they complete
// PreLockAnimationFinished is called, and system lock is requested.
// Once system locks and lock UI is created, OnLockStateChanged is called, and
// StartPostLockAnimation is called. In PostLockAnimationFinished two
// things happen : EVENT_LOCK_ANIMATION_FINISHED notification is sent (it
// triggers third part of animation within lock UI), and check for continuing to
// shutdown is made.
//
// Unlock workflow:
// WebUI does first part of animation, and calls OnLockScreenHide(callback) that
// triggers StartUnlockAnimationBeforeUIDestroyed(callback). Once callback is
// called at the end of the animation, lock UI is deleted, system unlocks, and
// OnLockStateChanged is called. It leads to
// StartUnlockAnimationAfterUIDestroyed.
class ASH_EXPORT LockStateController : public aura::WindowTreeHostObserver,
                                       public SessionObserver {
 public:
  // A bitfield mask including NON_LOCK_SCREEN_CONTAINERS and LAUNCHER, used for
  // pre-lock hiding animation.
  static const int kPreLockContainersMask;

  explicit LockStateController(ShutdownController* shutdown_controller);
  ~LockStateController() override;

  // Starts locking (with slow animation) that can be cancelled.
  void StartLockAnimation();

  // Starts locking (with slow animation) that can be cancelled.
  // After locking and |kLockToShutdownTimeoutMs| StartShutdownAnimation()
  // will be called unless CancelShutdownAnimation() is called.
  // |shutdown_reason| gives the reason for shutdown.
  void StartLockThenShutdownAnimation(ShutdownReason shutdown_reason);

  // Starts shutting down (with slow animation) that can be cancelled.
  void StartShutdownAnimation(ShutdownReason reason);

  // Starts usual lock animation, but locks immediately.
  void StartLockAnimationAndLockImmediately();

  // Starts locking without slow animation.
  void LockWithoutAnimation();

  // Returns true if we have requested system to lock, but haven't received
  // confirmation yet.
  bool LockRequested();

  // Returns true if we are shutting down.
  bool ShutdownRequested();

  // Returns true if we are within cancellable lock timeframe.
  bool CanCancelLockAnimation();

  // Cancels locking and reverts lock animation.
  void CancelLockAnimation();

  // Returns true if we are within cancellable shutdown timeframe.
  bool CanCancelShutdownAnimation();

  // Cancels shutting down and reverts shutdown animation.
  void CancelShutdownAnimation();

  // Called when Chrome gets a request to display the lock screen.
  void OnStartingLock();

  // Displays the shutdown animation and requests a system shutdown or system
  // restart depending on the the state of the |RebootOnShutdown| device policy.
  void RequestShutdown(ShutdownReason reason);

  // Called when ScreenLocker is ready to close, but not yet destroyed.
  // Can be used to display "hiding" animations on unlock.
  // |callback| will be called when all animations are done.
  void OnLockScreenHide(base::OnceClosure callback);

  // Sets up the callback that should be called once lock animation is finished.
  // Callback is guaranteed to be called once and then discarded.
  void SetLockScreenDisplayedCallback(base::OnceClosure callback);

  // aura::WindowTreeHostObserver override:
  void OnHostCloseRequested(aura::WindowTreeHost* host) override;

  // SessionObserver overrides:
  void OnChromeTerminating() override;
  void OnLockStateChanged(bool locked) override;

  void set_animator_for_test(SessionStateAnimator* animator) {
    animator_.reset(animator);
  }

 private:
  friend class LockStateControllerTestApi;

  struct UnlockedStateProperties {
    bool wallpaper_is_hidden;
  };

  // Reverts the pre-lock animation, reports the error.
  void OnLockFailTimeout();

  // Starts timer for gap between lock and shutdown.
  void StartLockToShutdownTimer();

  // Calls StartShutdownAnimation().
  void OnLockToShutdownTimeout();

  // Starts timer for undoable shutdown animation.
  void StartPreShutdownAnimationTimer();

  // Calls StartRealShutdownTimer().
  void OnPreShutdownAnimationTimeout();

  // Starts timer for final shutdown animation.
  // If |with_animation_time| is true, it will also include time of "fade to
  // white" shutdown animation.
  void StartRealShutdownTimer(bool with_animation_time);

  // Request that the machine be shut down.
  void OnRealPowerTimeout();

  // Starts shutdown animation that can be cancelled and starts pre-shutdown
  // timer.
  void StartCancellableShutdownAnimation();

  // If |request_lock_on_completion| is true, a lock request will be sent
  // after the pre-lock animation completes.  (The pre-lock animation is
  // also displayed in response to already-in-progress lock requests; in
  // these cases an additional lock request is undesirable.)
  void StartImmediatePreLockAnimation(bool request_lock_on_completion);
  void StartCancellablePreLockAnimation();
  void PreLockAnimation(SessionStateAnimator::AnimationSpeed speed,
                        bool request_lock_on_completion);
  void CancelPreLockAnimation();
  void StartPostLockAnimation();
  // This method calls |callback| when animation completes.
  void StartUnlockAnimationBeforeUIDestroyed(base::OnceClosure callback);
  void StartUnlockAnimationAfterUIDestroyed();

  // These methods are called when corresponding animation completes.
  void LockAnimationCancelled();
  void PreLockAnimationFinished(bool request_lock);
  void PostLockAnimationFinished();
  void UnlockAnimationAfterUIDestroyedFinished();

  // Stores properties of UI that have to be temporarily modified while locking.
  void StoreUnlockedProperties();
  void RestoreUnlockedProperties();

  // Fades in wallpaper layer with |speed| if it was hidden in unlocked state.
  void AnimateWallpaperAppearanceIfNecessary(
      ash::SessionStateAnimator::AnimationSpeed speed,
      SessionStateAnimator::AnimationSequence* animation_sequence);

  // Fades out wallpaper layer with |speed| if it was hidden in unlocked state.
  void AnimateWallpaperHidingIfNecessary(
      ash::SessionStateAnimator::AnimationSpeed speed,
      SessionStateAnimator::AnimationSequence* animation_sequence);

  std::unique_ptr<SessionStateAnimator> animator_;

  // Current lock status.
  bool system_is_locked_ = false;

  // Are we in the process of shutting the machine down?
  bool shutting_down_ = false;

  // The reason (e.g. user action) for a pending shutdown.
  base::Optional<ShutdownReason> shutdown_reason_;

  // Indicates whether controller should proceed to (cancellable) shutdown after
  // locking.
  bool shutdown_after_lock_ = false;

  // Indicates that controller displays lock animation.
  bool animating_lock_ = false;

  // Indicates that lock animation can be undone.
  bool can_cancel_lock_animation_ = false;

  // Indicates whether post lock animation should be immediate.
  bool post_lock_immediate_animation_ = false;

  std::unique_ptr<UnlockedStateProperties> unlocked_properties_;

  // How long has it been since the request to lock the screen?
  std::unique_ptr<base::ElapsedTimer> lock_duration_timer_;

  // Controller used to trigger the actual shutdown.
  ShutdownController* shutdown_controller_;

  // Started when we request that the screen be locked.  When it fires, we
  // assume that our request got dropped.
  base::OneShotTimer lock_fail_timer_;

  // Started when the screen is locked while the power button is held.  Adds a
  // delay between the appearance of the lock screen and the beginning of the
  // pre-shutdown animation.
  base::OneShotTimer lock_to_shutdown_timer_;

  // Started when we begin displaying the pre-shutdown animation.  When it
  // fires, we start the shutdown animation and get ready to request shutdown.
  base::OneShotTimer pre_shutdown_timer_;

  // Started when we display the shutdown animation.  When it fires, we actually
  // request shutdown.  Gives the animation time to complete before Chrome, X,
  // etc. are shut down.
  base::OneShotTimer real_shutdown_timer_;

  base::OnceClosure lock_screen_displayed_callback_;

  ScopedSessionObserver scoped_session_observer_;

  base::WeakPtrFactory<LockStateController> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(LockStateController);
};

}  // namespace ash

#endif  // ASH_WM_LOCK_STATE_CONTROLLER_H_
