// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/test/views/chrome_test_views_delegate.h"

#include "chrome/browser/ui/views/harmony/chrome_layout_provider.h"

ChromeTestViewsDelegate::ChromeTestViewsDelegate()
    : views::TestViewsDelegate() {
  // Overrides the LayoutProvider created by TestViewsDelegate.
  set_layout_provider(std::make_unique<ChromeLayoutProvider>());
}

ChromeTestViewsDelegate::~ChromeTestViewsDelegate() {}
