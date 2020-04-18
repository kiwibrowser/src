// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_TEST_SCOPED_FORCE_RTL_MAC_H__
#define CHROME_BROWSER_UI_COCOA_TEST_SCOPED_FORCE_RTL_MAC_H__

#include <string>

#include "base/test/scoped_feature_list.h"

namespace cocoa_l10n_util {

// Enables all flags required for
// |cocoa_l10n_util::ShouldDoExperimentalRTLLayout| to return true.
class ScopedForceRTLMac final {
 public:
  ScopedForceRTLMac();
  ~ScopedForceRTLMac();

 private:
  BOOL original_apple_text_direction_;
  BOOL original_rtl_writing_direction_;
  std::string original_locale_;
  base::test::ScopedFeatureList scoped_feature_list_;

  DISALLOW_COPY_AND_ASSIGN(ScopedForceRTLMac);
};

}  // namespace cocoa_l10n_util

#endif  // CHROME_BROWSER_UI_COCOA_TEST_SCOPED_FORCE_RTL_MAC_H__
