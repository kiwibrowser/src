// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_AURA_TEST_EVENT_GENERATOR_DELEGATE_AURA_H_
#define UI_AURA_TEST_EVENT_GENERATOR_DELEGATE_AURA_H_

#include "base/macros.h"
#include "ui/events/test/event_generator.h"

namespace aura {
class Window;
class WindowTreeHost;

namespace client {
class ScreenPositionClient;
}

namespace test {

void InitializeAuraEventGeneratorDelegate();

// Implementation of ui::test::EventGeneratorDelegate for Aura.
class EventGeneratorDelegateAura : public ui::test::EventGeneratorDelegate {
 public:
  EventGeneratorDelegateAura();
  ~EventGeneratorDelegateAura() override;

  // Returns the host for given point.
  virtual WindowTreeHost* GetHostAt(const gfx::Point& point) const = 0;

  // Returns the screen position client that determines the
  // coordinates used in EventGenerator. EventGenerator uses
  // root Window's coordinate if this returns NULL.
  virtual client::ScreenPositionClient* GetScreenPositionClient(
      const aura::Window* window) const = 0;

  // Overridden from ui::test::EventGeneratorDelegate:
  ui::EventTarget* GetTargetAt(const gfx::Point& location) override;
  ui::EventSource* GetEventSource(ui::EventTarget* target) override;
  gfx::Point CenterOfTarget(const ui::EventTarget* target) const override;
  gfx::Point CenterOfWindow(gfx::NativeWindow window) const override;
  void ConvertPointFromTarget(const ui::EventTarget* target,
                              gfx::Point* point) const override;
  void ConvertPointToTarget(const ui::EventTarget* target,
                            gfx::Point* point) const override;
  void ConvertPointFromHost(const ui::EventTarget* hosted_target,
                            gfx::Point* point) const override;
  ui::EventDispatchDetails DispatchKeyEventToIME(ui::EventTarget* target,
                                                 ui::KeyEvent* event) override;
  void DispatchEventToPointerWatchers(ui::EventTarget* target,
                                      const ui::PointerEvent& event) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(EventGeneratorDelegateAura);
};

}  // namespace test
}  // namespace aura

#endif  // UI_AURA_TEST_EVENT_GENERATOR_DELEGATE_AURA_H_
