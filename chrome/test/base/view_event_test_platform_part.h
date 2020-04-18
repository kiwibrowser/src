// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_TEST_BASE_VIEW_EVENT_TEST_PLATFORM_PART_H_
#define CHROME_TEST_BASE_VIEW_EVENT_TEST_PLATFORM_PART_H_

#include "base/macros.h"
#include "ui/gfx/native_widget_types.h"

namespace ui {
class ContextFactory;
class ContextFactoryPrivate;
}

// A helper class owned by tests that performs platform specific initialization.
// ViewEventTestPlatformPart behaves a bit like views::ViewsTestHelper, but on
// ChromeOS it will create an Ash shell environment, rather than using an
// AuraTestHelper.
class ViewEventTestPlatformPart {
 public:
  virtual ~ViewEventTestPlatformPart() {}

  // Set up the platform-specific environment. Teardown is performed in the
  // destructor.
  static ViewEventTestPlatformPart* Create(
      ui::ContextFactory* context_factory,
      ui::ContextFactoryPrivate* context_factory_private);

  // The Widget context for creating the test window. This will be the Ash root
  // window on ChromeOS environments. Otherwise it should return NULL.
  virtual gfx::NativeWindow GetContext() = 0;

 protected:
  ViewEventTestPlatformPart() {}

 private:
  DISALLOW_COPY_AND_ASSIGN(ViewEventTestPlatformPart);
};

#endif  // CHROME_TEST_BASE_VIEW_EVENT_TEST_PLATFORM_PART_H_
