// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_TEST_BASE_DEFAULT_ASH_EVENT_GENERATOR_DELEGATE_H_
#define CHROME_TEST_BASE_DEFAULT_ASH_EVENT_GENERATOR_DELEGATE_H_

#include "ash/shell.h"
#include "base/macros.h"
#include "base/memory/singleton.h"
#include "ui/aura/test/event_generator_delegate_aura.h"
#include "ui/aura/window.h"

class DefaultAshEventGeneratorDelegate
    : public aura::test::EventGeneratorDelegateAura {
 public:
  static DefaultAshEventGeneratorDelegate* GetInstance();

  // EventGeneratorDelegate:
  void SetContext(ui::test::EventGenerator* owner,
                  gfx::NativeWindow root_window,
                  gfx::NativeWindow window) override;

  aura::WindowTreeHost* GetHostAt(const gfx::Point& point) const override;

  aura::client::ScreenPositionClient* GetScreenPositionClient(
      const aura::Window* window) const override;

  ui::EventDispatchDetails DispatchKeyEventToIME(ui::EventTarget* target,
                                                 ui::KeyEvent* event) override;

 private:
  friend struct base::DefaultSingletonTraits<DefaultAshEventGeneratorDelegate>;

  DefaultAshEventGeneratorDelegate();

  ~DefaultAshEventGeneratorDelegate() override;

  aura::Window* root_window_;

  DISALLOW_COPY_AND_ASSIGN(DefaultAshEventGeneratorDelegate);
};

#endif  // CHROME_TEST_BASE_DEFAULT_ASH_EVENT_GENERATOR_DELEGATE_H_
