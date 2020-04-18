// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LANGUAGE_PREFERENCES_H_
#define CHROME_BROWSER_CHROMEOS_LANGUAGE_PREFERENCES_H_

class PrefRegistrySimple;

// TODO(yusukes): Rename this file to input_method_preference.cc. Since
// "language" usually means UI language, the current file name is confusing.
// The namespace should also be changed to "namespace input_method {".

// This file defines types and declare variables used in "Languages and
// Input" settings in Chromium OS.
namespace chromeos {
namespace language_prefs {

// ---------------------------------------------------------------------------
// For input method engine management
// ---------------------------------------------------------------------------
extern const char kGeneralSectionName[];
extern const char kPreloadEnginesConfigName[];

// ---------------------------------------------------------------------------
// For keyboard stuff
// ---------------------------------------------------------------------------
// A flag indicating whether the keyboard auto repeat is enabled.
extern const bool kXkbAutoRepeatEnabled;
// A delay between the first and the start of the rest.
extern const int kXkbAutoRepeatDelayInMs;
// An interval between the repeated keys.
extern const int kXkbAutoRepeatIntervalInMs;

// A string Chrome preference (Local State) of the preferred keyboard layout in
// the login screen.
extern const char kPreferredKeyboardLayout[];

// Registers non-user prefs for the default keyboard layout on the login screen.
void RegisterPrefs(PrefRegistrySimple* registry);

}  // namespace language_prefs
}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_LANGUAGE_PREFERENCES_H_
