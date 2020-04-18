// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/aura/test/event_generator_delegate_aura.h"

#include "base/macros.h"
#include "base/memory/singleton.h"
#include "ui/aura/client/screen_position_client.h"
#include "ui/aura/env.h"
#include "ui/aura/test/env_test_helper.h"
#include "ui/aura/test/mus/window_tree_client_private.h"
#include "ui/aura/window_event_dispatcher.h"
#include "ui/aura/window_tree_host.h"
#include "ui/base/ime/input_method.h"

namespace aura {
namespace test {
namespace {

class DefaultEventGeneratorDelegate : public EventGeneratorDelegateAura {
 public:
  static DefaultEventGeneratorDelegate* GetInstance() {
    return base::Singleton<DefaultEventGeneratorDelegate>::get();
  }

  // EventGeneratorDelegate:
  void SetContext(ui::test::EventGenerator* owner,
                  gfx::NativeWindow root_window,
                  gfx::NativeWindow window) override {
    root_window_ = root_window;
  }

  // EventGeneratorDelegateAura:
  WindowTreeHost* GetHostAt(const gfx::Point& point) const override {
    return root_window_->GetHost();
  }

  client::ScreenPositionClient* GetScreenPositionClient(
      const aura::Window* window) const override {
    return NULL;
  }

 private:
  friend struct base::DefaultSingletonTraits<DefaultEventGeneratorDelegate>;

  DefaultEventGeneratorDelegate() : root_window_(NULL) {
    DCHECK(!ui::test::EventGenerator::default_delegate);
    ui::test::EventGenerator::default_delegate = this;
  }

  ~DefaultEventGeneratorDelegate() override {
    DCHECK_EQ(this, ui::test::EventGenerator::default_delegate);
    ui::test::EventGenerator::default_delegate = NULL;
  }

  Window* root_window_;

  DISALLOW_COPY_AND_ASSIGN(DefaultEventGeneratorDelegate);
};

const Window* WindowFromTarget(const ui::EventTarget* event_target) {
  return static_cast<const Window*>(event_target);
}

}  // namespace

void InitializeAuraEventGeneratorDelegate() {
  if (!ui::test::EventGenerator::default_delegate) {
    DefaultEventGeneratorDelegate::GetInstance();
  }
}

EventGeneratorDelegateAura::EventGeneratorDelegateAura() {
}

EventGeneratorDelegateAura::~EventGeneratorDelegateAura() {
}

ui::EventTarget* EventGeneratorDelegateAura::GetTargetAt(
    const gfx::Point& location) {
  // TODO(andresantoso): Add support for EventGenerator::targeting_application()
  // if needed for Ash.
  return GetHostAt(location)->window();
}

ui::EventSource* EventGeneratorDelegateAura::GetEventSource(
    ui::EventTarget* target) {
  return static_cast<Window*>(target)->GetHost()->GetEventSource();
}

gfx::Point EventGeneratorDelegateAura::CenterOfTarget(
    const ui::EventTarget* target) const {
  gfx::Point center =
      gfx::Rect(WindowFromTarget(target)->bounds().size()).CenterPoint();
  ConvertPointFromTarget(target, &center);
  return center;
}

gfx::Point EventGeneratorDelegateAura::CenterOfWindow(
    gfx::NativeWindow window) const {
  return CenterOfTarget(window);
}

void EventGeneratorDelegateAura::ConvertPointFromTarget(
    const ui::EventTarget* event_target,
    gfx::Point* point) const {
  DCHECK(point);
  const Window* target = WindowFromTarget(event_target);
  aura::client::ScreenPositionClient* client = GetScreenPositionClient(target);
  if (client)
    client->ConvertPointToScreen(target, point);
  else
    aura::Window::ConvertPointToTarget(target, target->GetRootWindow(), point);
}

void EventGeneratorDelegateAura::ConvertPointToTarget(
    const ui::EventTarget* event_target,
    gfx::Point* point) const {
  DCHECK(point);
  const Window* target = WindowFromTarget(event_target);
  aura::client::ScreenPositionClient* client = GetScreenPositionClient(target);
  if (client)
    client->ConvertPointFromScreen(target, point);
  else
    aura::Window::ConvertPointToTarget(target->GetRootWindow(), target, point);
}

void EventGeneratorDelegateAura::ConvertPointFromHost(
    const ui::EventTarget* hosted_target,
    gfx::Point* point) const {
  const Window* window = WindowFromTarget(hosted_target);
  window->GetHost()->ConvertPixelsToDIP(point);
}

ui::EventDispatchDetails EventGeneratorDelegateAura::DispatchKeyEventToIME(
    ui::EventTarget* target,
    ui::KeyEvent* event) {
  Window* window = static_cast<Window*>(target);
  return window->GetHost()->GetInputMethod()->DispatchKeyEvent(event);
}

void EventGeneratorDelegateAura::DispatchEventToPointerWatchers(
    ui::EventTarget* target,
    const ui::PointerEvent& event) {
  // In non-mus aura PointerWatchers are handled by system-wide EventHandlers,
  // for example ash::Shell and ash::PointerWatcherAdapter.
  if (!Env::GetInstance()->HasWindowTreeClient())
    return;

  // Route the event through WindowTreeClient as in production mus. Does nothing
  // if there are no PointerWatchers installed.
  Window* window = static_cast<Window*>(target);
  WindowTreeClient* window_tree_client = EnvTestHelper().GetWindowTreeClient();
  WindowTreeClientPrivate(window_tree_client)
      .CallOnPointerEventObserved(window, ui::Event::Clone(event));
}

}  // namespace test
}  // namespace aura
