// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_FRAME_OPAQUE_BROWSER_FRAME_VIEW_PLATFORM_SPECIFIC_H_
#define CHROME_BROWSER_UI_VIEWS_FRAME_OPAQUE_BROWSER_FRAME_VIEW_PLATFORM_SPECIFIC_H_

class OpaqueBrowserFrameView;
class OpaqueBrowserFrameViewLayout;
class ThemeService;

// Handles platform specific configuration concepts.
class OpaqueBrowserFrameViewPlatformSpecific {
 public:
  virtual ~OpaqueBrowserFrameViewPlatformSpecific() {}

  // Returns whether we're using native system like rendering for theme
  // elements.
  //
  // Why not just ask ThemeService::UsingSystemTheme()? Because on Windows, the
  // default theme is UsingSystemTheme(). Therefore, the default implementation
  // always returns false and we specifically override this on Linux.
  virtual bool IsUsingSystemTheme();

  // Builds an observer for |view| and |layout|.
  static OpaqueBrowserFrameViewPlatformSpecific* Create(
      OpaqueBrowserFrameView* view,
      OpaqueBrowserFrameViewLayout* layout,
      ThemeService* theme_service);
};

#endif  // CHROME_BROWSER_UI_VIEWS_FRAME_OPAQUE_BROWSER_FRAME_VIEW_PLATFORM_SPECIFIC_H_
