// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_TRANSLATE_BUBBLE_BRIDGE_VIEWS_H_
#define CHROME_BROWSER_UI_COCOA_TRANSLATE_BUBBLE_BRIDGE_VIEWS_H_

#include <Cocoa/Cocoa.h>

#include "components/translate/core/browser/translate_step.h"
#include "components/translate/core/common/translate_errors.h"

namespace content {
class WebContents;
}

class LocationBarViewMac;

void ShowTranslateBubbleViews(NSWindow* parent_window,
                              LocationBarViewMac* location_bar,
                              content::WebContents* web_contents,
                              translate::TranslateStep step,
                              translate::TranslateErrors::Type error_type,
                              bool is_user_gesture);

#endif  // CHROME_BROWSER_UI_COCOA_TRANSLATE_BUBBLE_BRIDGE_VIEWS_H_
