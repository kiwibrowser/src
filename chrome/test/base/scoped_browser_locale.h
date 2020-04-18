// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_TEST_BASE_SCOPED_BROWSER_LOCALE_H_
#define CHROME_TEST_BASE_SCOPED_BROWSER_LOCALE_H_

#include <string>

#include "base/macros.h"

// Helper class to temporarily set the locale of the browser process.
class ScopedBrowserLocale {
 public:
  explicit ScopedBrowserLocale(const std::string& new_locale);
  ~ScopedBrowserLocale();

 private:
  const std::string old_locale_;

  DISALLOW_COPY_AND_ASSIGN(ScopedBrowserLocale);
};

#endif  // CHROME_TEST_BASE_SCOPED_BROWSER_LOCALE_H_
