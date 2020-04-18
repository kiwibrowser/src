// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_SCREEN_ORIENTATION_SCREEN_ORIENTATION_CONTROLLER_IMPL_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_SCREEN_ORIENTATION_SCREEN_ORIENTATION_CONTROLLER_IMPL_H_

#include <memory>
#include "services/device/public/mojom/screen_orientation.mojom-blink.h"
#include "third_party/blink/public/common/screen_orientation/web_screen_orientation_lock_type.h"
#include "third_party/blink/public/common/screen_orientation/web_screen_orientation_type.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/frame/platform_event_controller.h"
#include "third_party/blink/renderer/core/frame/screen_orientation_controller.h"
#include "third_party/blink/renderer/modules/modules_export.h"
#include "third_party/blink/renderer/modules/screen_orientation/web_lock_orientation_callback.h"

namespace blink {

class ScreenOrientation;

using device::mojom::blink::ScreenOrientationAssociatedPtr;
using device::mojom::blink::ScreenOrientationLockResult;

class MODULES_EXPORT ScreenOrientationControllerImpl final
    : public ScreenOrientationController,
      public ContextLifecycleObserver,
      public PlatformEventController {
  USING_GARBAGE_COLLECTED_MIXIN(ScreenOrientationControllerImpl);
  WTF_MAKE_NONCOPYABLE(ScreenOrientationControllerImpl);

 public:
  ~ScreenOrientationControllerImpl() override;

  void SetOrientation(ScreenOrientation*);
  void NotifyOrientationChanged() override;

  // Implementation of ScreenOrientationController.
  void lock(WebScreenOrientationLockType,
            std::unique_ptr<WebLockOrientationCallback>) override;
  void unlock() override;
  bool MaybeHasActiveLock() const override;

  static void ProvideTo(LocalFrame&);
  static ScreenOrientationControllerImpl* From(LocalFrame&);

  void SetScreenOrientationAssociatedPtrForTests(
      ScreenOrientationAssociatedPtr);

  void Trace(blink::Visitor*) override;

 private:
  friend class MediaControlsOrientationLockAndRotateToFullscreenDelegateTest;
  friend class ScreenOrientationControllerImplTest;

  explicit ScreenOrientationControllerImpl(LocalFrame&);

  static WebScreenOrientationType ComputeOrientation(const IntRect&, uint16_t);

  // Inherited from PlatformEventController.
  void DidUpdateData() override;
  void RegisterWithDispatcher() override;
  void UnregisterWithDispatcher() override;
  bool HasLastData() override;

  // Inherited from ContextLifecycleObserver and PageVisibilityObserver.
  void ContextDestroyed(ExecutionContext*) override;
  void PageVisibilityChanged() override;

  void NotifyDispatcher();

  void UpdateOrientation();

  void DispatchEventTimerFired(TimerBase*);

  bool IsActive() const;
  bool IsVisible() const;
  bool IsActiveAndVisible() const;

  void OnLockOrientationResult(int, ScreenOrientationLockResult);
  void CancelPendingLocks();
  int GetRequestIdForTests();

  Member<ScreenOrientation> orientation_;
  TaskRunnerTimer<ScreenOrientationControllerImpl> dispatch_event_timer_;
  bool active_lock_ = false;
  ScreenOrientationAssociatedPtr screen_orientation_service_;
  std::unique_ptr<WebLockOrientationCallback> pending_callback_;
  int request_id_ = 0;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_SCREEN_ORIENTATION_SCREEN_ORIENTATION_CONTROLLER_IMPL_H_
