// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PREFS_ORIGIN_TRIAL_PREFS_H_
#define CHROME_BROWSER_PREFS_ORIGIN_TRIAL_PREFS_H_

class PrefRegistrySimple;

class OriginTrialPrefs {
 public:
  static void RegisterPrefs(PrefRegistrySimple* registry);
};

#endif  // CHROME_BROWSER_PREFS_ORIGIN_TRIAL_PREFS_H_
