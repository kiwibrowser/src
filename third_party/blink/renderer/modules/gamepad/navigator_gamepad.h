/*
 * Copyright (C) 2011, Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_GAMEPAD_NAVIGATOR_GAMEPAD_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_GAMEPAD_NAVIGATOR_GAMEPAD_H_

#include "third_party/blink/renderer/core/dom/context_lifecycle_observer.h"
#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/core/frame/navigator.h"
#include "third_party/blink/renderer/core/frame/platform_event_controller.h"
#include "third_party/blink/renderer/modules/modules_export.h"
#include "third_party/blink/renderer/platform/async_method_runner.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/supplementable.h"

namespace blink {

class Document;
class Gamepad;
class GamepadList;
class Navigator;

class MODULES_EXPORT NavigatorGamepad final
    : public GarbageCollectedFinalized<NavigatorGamepad>,
      public Supplement<Navigator>,
      public DOMWindowClient,
      public PlatformEventController,
      public LocalDOMWindow::EventListenerObserver {
  USING_GARBAGE_COLLECTED_MIXIN(NavigatorGamepad);

 public:
  static const char kSupplementName[];

  static NavigatorGamepad* From(Document&);
  static NavigatorGamepad& From(Navigator&);
  ~NavigatorGamepad() override;

  static GamepadList* getGamepads(Navigator&);
  GamepadList* Gamepads();

  void Trace(blink::Visitor*) override;

 private:
  explicit NavigatorGamepad(Navigator&);

  void DispatchOneEvent();
  void DidRemoveGamepadEventListeners();
  bool StartUpdatingIfAttached();
  void SampleAndCheckConnectedGamepads();
  bool CheckConnectedGamepads(GamepadList*, GamepadList*);
  void CheckConnectedGamepad(Gamepad*, Gamepad*, bool*, bool*);

  // PageVisibilityObserver
  void PageVisibilityChanged() override;

  // PlatformEventController
  void RegisterWithDispatcher() override;
  void UnregisterWithDispatcher() override;
  bool HasLastData() override;
  void DidUpdateData() override;

  // LocalDOMWindow::EventListenerObserver
  void DidAddEventListener(LocalDOMWindow*, const AtomicString&) override;
  void DidRemoveEventListener(LocalDOMWindow*, const AtomicString&) override;
  void DidRemoveAllEventListeners(LocalDOMWindow*) override;

  Member<GamepadList> gamepads_;
  Member<GamepadList> gamepads_back_;
  HeapDeque<Member<Gamepad>> pending_events_;
  Member<AsyncMethodRunner<NavigatorGamepad>> dispatch_one_event_runner_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_GAMEPAD_NAVIGATOR_GAMEPAD_H_
