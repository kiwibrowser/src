// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>

#include "base/macros.h"
#include "chrome/test/base/view_event_test_platform_part.h"
#include "ui/aura/env.h"
#include "ui/display/screen.h"
#include "ui/views/widget/desktop_aura/desktop_screen.h"

#if !defined(OS_CHROMEOS) && defined(OS_LINUX)
#include "ui/views/test/test_desktop_screen_x11.h"
#endif

namespace {

// ViewEventTestPlatformPart implementation for Views, but non-CrOS.
class ViewEventTestPlatformPartDefault : public ViewEventTestPlatformPart {
 public:
  ViewEventTestPlatformPartDefault(
      ui::ContextFactory* context_factory,
      ui::ContextFactoryPrivate* context_factory_private) {
#if defined(USE_AURA)
    DCHECK(!display::Screen::GetScreen());
#if defined(USE_X11)
    display::Screen::SetScreenInstance(
        views::test::TestDesktopScreenX11::GetInstance());
#else
    screen_.reset(views::CreateDesktopScreen());
    display::Screen::SetScreenInstance(screen_.get());
#endif
    env_ = aura::Env::CreateInstance();
    env_->set_context_factory(context_factory);
    env_->set_context_factory_private(context_factory_private);
#endif
  }

  ~ViewEventTestPlatformPartDefault() override {
#if defined(USE_AURA)
    env_.reset();
    display::Screen::SetScreenInstance(nullptr);
#endif
  }

  // Overridden from ViewEventTestPlatformPart:
  gfx::NativeWindow GetContext() override { return NULL; }

 private:
  std::unique_ptr<display::Screen> screen_;
  std::unique_ptr<aura::Env> env_;

  DISALLOW_COPY_AND_ASSIGN(ViewEventTestPlatformPartDefault);
};

}  // namespace

// static
ViewEventTestPlatformPart* ViewEventTestPlatformPart::Create(
    ui::ContextFactory* context_factory,
    ui::ContextFactoryPrivate* context_factory_private) {
  return new ViewEventTestPlatformPartDefault(context_factory,
                                              context_factory_private);
}
