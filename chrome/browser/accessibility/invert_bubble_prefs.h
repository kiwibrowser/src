// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ACCESSIBILITY_INVERT_BUBBLE_PREFS_H_
#define CHROME_BROWSER_ACCESSIBILITY_INVERT_BUBBLE_PREFS_H_

namespace user_prefs {
class PrefRegistrySyncable;
}

void RegisterInvertBubbleUserPrefs(user_prefs::PrefRegistrySyncable* registry);

#endif  // CHROME_BROWSER_ACCESSIBILITY_INVERT_BUBBLE_PREFS_H_
