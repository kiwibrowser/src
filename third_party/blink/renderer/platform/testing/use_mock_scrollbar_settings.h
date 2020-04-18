// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_TESTING_USE_MOCK_SCROLLBAR_SETTINGS_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_TESTING_USE_MOCK_SCROLLBAR_SETTINGS_H_

#include "third_party/blink/renderer/platform/scroll/scrollbar_theme.h"
#include "third_party/blink/renderer/platform/testing/runtime_enabled_features_test_helpers.h"

namespace blink {

// Forces to use mocked overlay scrollbars instead of the default native theme
// scrollbars to avoid crash in Chromium code when it tries to load UI
// resources that are not available when running blink unit tests, and to
// ensure consistent layout regardless of differences between scrollbar themes.
// WebViewHelper includes this, so this is only needed if a test doesn't use
// WebViewHelper or the test needs a bigger scope of mock scrollbar settings
// than the scope of WebViewHelper.
class UseMockScrollbarSettings : private ScopedOverlayScrollbarsForTest {
 public:
  UseMockScrollbarSettings()
      : ScopedOverlayScrollbarsForTest(true),
        original_mock_scrollbar_enabled_(
            ScrollbarTheme::MockScrollbarsEnabled()),
        original_overlay_scrollbars_enabled_(
            RuntimeEnabledFeatures::OverlayScrollbarsEnabled()) {
    ScrollbarTheme::SetMockScrollbarsEnabled(true);
  }

  UseMockScrollbarSettings(bool use_mock, bool use_overlay)
      : ScopedOverlayScrollbarsForTest(use_overlay),
        original_mock_scrollbar_enabled_(
            ScrollbarTheme::MockScrollbarsEnabled()),
        original_overlay_scrollbars_enabled_(
            RuntimeEnabledFeatures::OverlayScrollbarsEnabled()) {
    ScrollbarTheme::SetMockScrollbarsEnabled(use_mock);
  }

  ~UseMockScrollbarSettings() {
    ScrollbarTheme::SetMockScrollbarsEnabled(original_mock_scrollbar_enabled_);
  }

 private:
  bool original_mock_scrollbar_enabled_;
  bool original_overlay_scrollbars_enabled_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_TESTING_USE_MOCK_SCROLLBAR_SETTINGS_H_
