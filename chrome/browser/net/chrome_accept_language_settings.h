// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_NET_CHROME_ACCEPT_LANGUAGE_SETTINGS_H_
#define CHROME_BROWSER_NET_CHROME_ACCEPT_LANGUAGE_SETTINGS_H_

#include <string>

namespace chrome_accept_language_settings {

// Given value of prefs::kAcceptLanguages pref, computes the corresponding
// Accept-Language header to send.
std::string ComputeAcceptLanguageFromPref(const std::string& language_pref);

// Adds the base language if a corresponding language+region code is present.
std::string ExpandLanguageList(const std::string& language_prefs);

}  // namespace chrome_accept_language_settings

#endif  // CHROME_BROWSER_NET_CHROME_ACCEPT_LANGUAGE_SETTINGS_H_
