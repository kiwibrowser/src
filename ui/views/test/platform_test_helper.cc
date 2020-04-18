// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/test/platform_test_helper.h"

#include "base/callback.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "ui/compositor/test/context_factories_for_test.h"
#include "ui/views/widget/widget.h"

#if defined(USE_AURA)
#include "ui/aura/window.h"
#endif

namespace views {
namespace {

PlatformTestHelper::Factory test_helper_factory;
bool is_mus = false;

}  // namespace

PlatformTestHelper::~PlatformTestHelper() {
  ui::TerminateContextFactoryForTests();
}

void PlatformTestHelper::set_factory(const Factory& factory) {
  DCHECK(test_helper_factory.is_null());
  test_helper_factory = factory;
}

// static
std::unique_ptr<PlatformTestHelper> PlatformTestHelper::Create() {
  return !test_helper_factory.is_null()
             ? test_helper_factory.Run()
             : base::WrapUnique(new PlatformTestHelper);
}

// static
void PlatformTestHelper::SetIsMus() {
  is_mus = true;
}

// static
bool PlatformTestHelper::IsMus() {
  return is_mus;
}

#if defined(USE_AURA)
void PlatformTestHelper::SimulateNativeDestroy(Widget* widget) {
  delete widget->GetNativeView();
}
#endif

void PlatformTestHelper::InitializeContextFactory(
    ui::ContextFactory** context_factory,
    ui::ContextFactoryPrivate** context_factory_private) {
  bool enable_pixel_output = false;
  ui::InitializeContextFactoryForTests(enable_pixel_output, context_factory,
                                       context_factory_private);
}

}  // namespace views
