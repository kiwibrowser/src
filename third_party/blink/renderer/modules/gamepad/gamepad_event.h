// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_GAMEPAD_GAMEPAD_EVENT_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_GAMEPAD_GAMEPAD_EVENT_H_

#include "third_party/blink/renderer/modules/event_modules.h"
#include "third_party/blink/renderer/modules/gamepad/gamepad.h"
#include "third_party/blink/renderer/modules/gamepad/gamepad_event_init.h"

namespace blink {

class GamepadEvent final : public Event {
  DEFINE_WRAPPERTYPEINFO();

 public:
  static GamepadEvent* Create(const AtomicString& type,
                              Bubbles bubbles,
                              Cancelable cancelable,
                              Gamepad* gamepad) {
    return new GamepadEvent(type, bubbles, cancelable, gamepad);
  }
  static GamepadEvent* Create(const AtomicString& type,
                              const GamepadEventInit& initializer) {
    return new GamepadEvent(type, initializer);
  }
  ~GamepadEvent() override;

  Gamepad* getGamepad() const { return gamepad_.Get(); }

  const AtomicString& InterfaceName() const override;

  void Trace(blink::Visitor*) override;

 private:
  GamepadEvent(const AtomicString& type, Bubbles, Cancelable, Gamepad*);
  GamepadEvent(const AtomicString&, const GamepadEventInit&);

  Member<Gamepad> gamepad_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_GAMEPAD_GAMEPAD_EVENT_H_
