// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ANDROID_CONTEXTUAL_SUGGESTIONS_CONTEXTUAL_SUGGESTIONS_PREFS_H_
#define CHROME_BROWSER_ANDROID_CONTEXTUAL_SUGGESTIONS_CONTEXTUAL_SUGGESTIONS_PREFS_H_

class PrefRegistrySimple;

namespace contextual_suggestions {

namespace prefs {

// True if contextual suggestions is enabled.
extern const char kContextualSuggestionsEnabled[];

}  // namespace prefs

class ContextualSuggestionsPrefs {
 public:
  static void RegisterProfilePrefs(PrefRegistrySimple* registry);
};

}  // namespace contextual_suggestions

#endif  // CHROME_BROWSER_ANDROID_CONTEXTUAL_SUGGESTIONS_CONTEXTUAL_SUGGESTIONS_PREFS_H_
