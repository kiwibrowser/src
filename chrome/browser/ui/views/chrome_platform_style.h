// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_CHROME_PLATFORM_STYLE_H_
#define CHROME_BROWSER_UI_VIEWS_CHROME_PLATFORM_STYLE_H_

// This class defines per-platform design variations in the //chrome layer, to
// make it more apparent which are accidental and which are intentional. It
// is similar in purpose to //ui/views' PlatformStyle.
class ChromePlatformStyle {
 public:
  // Whether the Omnibox should use a focus ring to indicate that it has
  // keyboard focus.
  static bool ShouldOmniboxUseFocusRing();

 private:
  ChromePlatformStyle() = delete;
};

#endif  // CHROME_BROWSER_UI_VIEWS_CHROME_PLATFORM_STYLE_H_
