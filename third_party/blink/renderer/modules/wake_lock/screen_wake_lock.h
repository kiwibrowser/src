// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_WAKE_LOCK_SCREEN_WAKE_LOCK_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_WAKE_LOCK_SCREEN_WAKE_LOCK_H_

#include "services/device/public/mojom/wake_lock.mojom-blink.h"
#include "third_party/blink/renderer/core/dom/context_lifecycle_observer.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/page/page_visibility_observer.h"
#include "third_party/blink/renderer/modules/modules_export.h"
#include "third_party/blink/renderer/platform/wtf/noncopyable.h"

namespace blink {

class LocalFrame;
class Screen;

class MODULES_EXPORT ScreenWakeLock final
    : public GarbageCollectedFinalized<ScreenWakeLock>,
      public Supplement<LocalFrame>,
      public ContextLifecycleObserver,
      public PageVisibilityObserver {
  USING_GARBAGE_COLLECTED_MIXIN(ScreenWakeLock);
  WTF_MAKE_NONCOPYABLE(ScreenWakeLock);

 public:
  static const char kSupplementName[];

  static bool keepAwake(Screen&);
  static void setKeepAwake(Screen&, bool);

  static ScreenWakeLock* From(LocalFrame*);

  ~ScreenWakeLock() = default;

  void Trace(blink::Visitor*) override;

 private:
  explicit ScreenWakeLock(LocalFrame&);

  // Inherited from PageVisibilityObserver.
  void PageVisibilityChanged() override;

  // Inherited from ContextLifecycleObserver.
  void ContextDestroyed(ExecutionContext*) override;

  bool keepAwake() const;
  void setKeepAwake(bool);

  static ScreenWakeLock* FromScreen(Screen&);
  void NotifyService();

  device::mojom::blink::WakeLockPtr service_;
  bool keep_awake_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_WAKE_LOCK_SCREEN_WAKE_LOCK_H_
