// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_CHROME_STYLE_H_
#define CHROME_BROWSER_UI_COCOA_CHROME_STYLE_H_

#include "third_party/skia/include/core/SkColor.h"
#include "ui/base/resource/resource_bundle.h"

// This file contains constants and functions specifying appearance properties
// of the new "Chrome-style" UI.

// TODO(wittman): These functions and constants should be moved under src/ui,
// possibly src/ui/base, as the "Chrome-style" UI will become the default
// styling for Views.

namespace chrome_style {

int GetCloseButtonSize();  // Size of close button.
SkColor GetBackgroundColor();  // Dialog background color.
SkColor GetLinkColor();  // Dialog link color.

const int kTitleTopPadding = 15; // Padding above the title.
const int kHorizontalPadding = 20; // Left and right padding.
const int kClientBottomPadding = 20; // Padding below the client view.
const int kCloseButtonPadding = 7; // Padding around the close button.
const int kBorderRadius = 2; // Border radius for dialog corners.
const int kRowPadding = 20; // Padding between rows of text.

// Font style for dialog text.
const ui::ResourceBundle::FontStyle kTextFontStyle =
    ui::ResourceBundle::BaseFont;
// Font style for dialog title.
const ui::ResourceBundle::FontStyle kTitleFontStyle =
    ui::ResourceBundle::MediumFont;

}  // namespace chrome_style

#endif  // CHROME_BROWSER_UI_COCOA_CHROME_STYLE_H_
