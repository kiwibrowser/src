// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_L10N_L10N_UTIL_MAC_H_
#define UI_BASE_L10N_L10N_UTIL_MAC_H_

#include <stddef.h>

#include <string>
#include <vector>

#include "base/strings/string16.h"
#include "ui/base/ui_base_export.h"

#ifdef __OBJC__
@class NSString;
#else
class NSString;
#endif

namespace l10n_util {

// Get localized language name using NSLocale Foundation API. If the system
// API returns null or an empty string, ICU's formatting style of an unknown
// language will be used which is "xyz (XYZ)" where the input is parsed into
// language and script by the - token and reformatted as
// "$lowercase_language ($UPPERCASE_SCRIPT)". If the - token is not found, the
// lowercase version of |locale| will be returned.
UI_BASE_EXPORT base::string16 GetDisplayNameForLocale(
    const std::string& locale,
    const std::string& display_locale);

// Remove the Windows-style accelerator marker (for labels, menuitems, etc.)
// and change "..." into an ellipsis.
// Returns the result in an autoreleased NSString.
UI_BASE_EXPORT NSString* FixUpWindowsStyleLabel(const base::string16& label);

// Pulls resource string from the string bundle and returns it.
UI_BASE_EXPORT NSString* GetNSString(int message_id);

// Get a resource string and replace $1-$2-$3 with |a| and |b|
// respectively.  Additionally, $$ is replaced by $.
UI_BASE_EXPORT NSString* GetNSStringF(int message_id, const base::string16& a);
UI_BASE_EXPORT NSString* GetNSStringF(int message_id,
                                      const base::string16& a,
                                      const base::string16& b);
UI_BASE_EXPORT NSString* GetNSStringF(int message_id,
                                      const base::string16& a,
                                      const base::string16& b,
                                      const base::string16& c);
UI_BASE_EXPORT NSString* GetNSStringF(int message_id,
                                      const base::string16& a,
                                      const base::string16& b,
                                      const base::string16& c,
                                      const base::string16& d);
UI_BASE_EXPORT NSString* GetNSStringF(int message_id,
                                      const base::string16& a,
                                      const base::string16& b,
                                      const base::string16& c,
                                      const base::string16& d,
                                      const base::string16& e);

// Variants that return the offset(s) of the replaced parameters. (See
// app/l10n_util.h for more details.)
UI_BASE_EXPORT NSString* GetNSStringF(int message_id,
                                      const base::string16& a,
                                      const base::string16& b,
                                      std::vector<size_t>* offsets);

// Same as GetNSString, but runs the result through FixUpWindowsStyleLabel
// before returning it.
UI_BASE_EXPORT NSString* GetNSStringWithFixup(int message_id);

// Same as GetNSStringF, but runs the result through FixUpWindowsStyleLabel
// before returning it.
UI_BASE_EXPORT NSString* GetNSStringFWithFixup(int message_id,
                                               const base::string16& a);
UI_BASE_EXPORT NSString* GetNSStringFWithFixup(int message_id,
                                               const base::string16& a,
                                               const base::string16& b);
UI_BASE_EXPORT NSString* GetNSStringFWithFixup(int message_id,
                                               const base::string16& a,
                                               const base::string16& b,
                                               const base::string16& c);
UI_BASE_EXPORT NSString* GetNSStringFWithFixup(int message_id,
                                               const base::string16& a,
                                               const base::string16& b,
                                               const base::string16& c,
                                               const base::string16& d);

// Get a resource string using |number| with a locale-specific plural rule.
// |message_id| points to a message in the ICU syntax.
// See http://userguide.icu-project.org/formatparse/messages and
// go/plurals (Google internal).
UI_BASE_EXPORT NSString* GetPluralNSStringF(int message_id, int number);

// Support the override of the locale with the value from Cocoa.
UI_BASE_EXPORT void OverrideLocaleWithCocoaLocale();
UI_BASE_EXPORT const std::string& GetLocaleOverride();

}  // namespace l10n_util

#endif  // UI_BASE_L10N_L10N_UTIL_MAC_H_
