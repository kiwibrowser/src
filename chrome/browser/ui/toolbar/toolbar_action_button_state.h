// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_TOOLBAR_TOOLBAR_ACTION_BUTTON_STATE_H_
#define CHROME_BROWSER_UI_TOOLBAR_TOOLBAR_ACTION_BUTTON_STATE_H_

// The state of the toolbar action button; used when generating an icon depends
// on the state. We don't use views::Button::State to avoid a hard dependency
// ui/views code from this code, since it's shared with Cocoa.
enum class ToolbarActionButtonState {
  kNormal,
  kHovered,
  kPressed,
};

#endif  // CHROME_BROWSER_UI_TOOLBAR_TOOLBAR_ACTION_BUTTON_STATE_H_
