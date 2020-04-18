// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_SCREEN_ORIENTATION_SCREEN_ORIENTATION_DISPATCHER_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_SCREEN_ORIENTATION_SCREEN_ORIENTATION_DISPATCHER_H_

#include "base/macros.h"
#include "services/device/public/mojom/screen_orientation.mojom-blink.h"
#include "third_party/blink/renderer/core/frame/platform_event_dispatcher.h"
#include "third_party/blink/renderer/platform/heap/handle.h"

namespace blink {

// ScreenOrientationDispatcher is a singleton that handles whether the current
// Blink instance should be listening to the screen orientation platform events.
// It is not a common implementation of PlatformEventDispatcher in the sense
// that it doesn't actually dispatch events but simply start/stop listening. The
// reason being that screen orientation events are always sent to the WebFrame's
// but some platforms require to poll to have an accurate reporting. When
// ScreenOrientationDispatcher is listening, that means that the platform should
// be polling if required.
class ScreenOrientationDispatcher final
    : public GarbageCollectedFinalized<ScreenOrientationDispatcher>,
      public PlatformEventDispatcher {
  USING_GARBAGE_COLLECTED_MIXIN(ScreenOrientationDispatcher);

 public:
  static ScreenOrientationDispatcher& Instance();

  ~ScreenOrientationDispatcher();

  void Trace(blink::Visitor*) override;

 private:
  ScreenOrientationDispatcher();

  // Inherited from PlatformEventDispatcher.
  void StartListening() override;
  void StopListening() override;

  device::mojom::blink::ScreenOrientationListenerPtr listener_;

  DISALLOW_COPY_AND_ASSIGN(ScreenOrientationDispatcher);
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_SCREEN_ORIENTATION_SCREEN_ORIENTATION_DISPATCHER_H_
