// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_IME_CHROMEOS_EXTENSION_IME_UTIL_H_
#define UI_BASE_IME_CHROMEOS_EXTENSION_IME_UTIL_H_

#include <string>

#include "base/auto_reset.h"
#include "ui/base/ime/ui_base_ime_export.h"

namespace chromeos {

// Extension IME related utilities.
namespace extension_ime_util {

#if defined(GOOGLE_CHROME_BUILD)
const char kXkbExtensionId[] = "jkghodnilhceideoidjikpgommlajknk";
const char kM17nExtensionId[] = "jkghodnilhceideoidjikpgommlajknk";
const char kHangulExtensionId[] = "bdgdidmhaijohebebipajioienkglgfo";
const char kMozcExtensionId[] = "jkghodnilhceideoidjikpgommlajknk";
const char kT13nExtensionId[] = "jkghodnilhceideoidjikpgommlajknk";
const char kChinesePinyinExtensionId[] = "jkghodnilhceideoidjikpgommlajknk";
const char kChineseZhuyinExtensionId[] = "jkghodnilhceideoidjikpgommlajknk";
const char kChineseCangjieExtensionId[] = "jkghodnilhceideoidjikpgommlajknk";
#else
const char kXkbExtensionId[] = "fgoepimhcoialccpbmpnnblemnepkkao";
const char kM17nExtensionId[] = "jhffeifommiaekmbkkjlpmilogcfdohp";
const char kHangulExtensionId[] = "bdgdidmhaijohebebipajioienkglgfo";
const char kMozcExtensionId[] = "bbaiamgfapehflhememkfglaehiobjnk";
const char kT13nExtensionId[] = "gjaehgfemfahhmlgpdfknkhdnemmolop";
const char kChinesePinyinExtensionId[] = "cpgalbafkoofkjmaeonnfijgpfennjjn";
const char kChineseZhuyinExtensionId[] = "ekbifjdfhkmdeeajnolmgdlmkllopefi";
const char kChineseCangjieExtensionId[] = "aeebooiibjahgpgmhkeocbeekccfknbj";
#endif

// Extension id, path (relative to |chrome::DIR_RESOURCES|) and IME engine
// id for the builtin-in Braille IME extension.
UI_BASE_IME_EXPORT extern const char kBrailleImeExtensionId[];
UI_BASE_IME_EXPORT extern const char kBrailleImeExtensionPath[];
UI_BASE_IME_EXPORT extern const char kBrailleImeEngineId[];

// Returns InputMethodID for |engine_id| in |extension_id| of extension IME.
// This function does not check |extension_id| is installed extension IME nor
// |engine_id| is really a member of |extension_id|.
std::string UI_BASE_IME_EXPORT
GetInputMethodID(const std::string& extension_id, const std::string& engine_id);

// Returns InputMethodID for |engine_id| in |extension_id| of component
// extension IME, This function does not check |extension_id| is component one
// nor |engine_id| is really a member of |extension_id|.
std::string UI_BASE_IME_EXPORT
GetComponentInputMethodID(const std::string& extension_id,
                          const std::string& engine_id);

// Returns extension ID if |input_method_id| is extension IME ID or component
// extension IME ID. Otherwise returns an empty string ("").
std::string UI_BASE_IME_EXPORT
GetExtensionIDFromInputMethodID(const std::string& input_method_id);

// Returns InputMethodID from engine id (e.g. xkb:fr:fra), or returns itself if
// the |engine_id| is not a known engine id.
// The caller must make sure the |engine_id| is from system input methods
// instead of 3rd party input methods.
std::string UI_BASE_IME_EXPORT
GetInputMethodIDByEngineID(const std::string& engine_id);

// Returns true if |input_method_id| is extension IME ID. This function does not
// check |input_method_id| is installed extension IME.
bool UI_BASE_IME_EXPORT IsExtensionIME(const std::string& input_method_id);

// Returns true if |input_method_id| is component extension IME ID. This
// function does not check |input_method_id| is really whitelisted one or not.
// If you want to check |input_method_id| is whitelisted component extension
// IME, please use ComponentExtensionIMEManager::IsWhitelisted instead.
bool UI_BASE_IME_EXPORT
IsComponentExtensionIME(const std::string& input_method_id);

// Returns true if the |input_method| is a member of |extension_id| of extension
// IME, otherwise returns false.
bool UI_BASE_IME_EXPORT IsMemberOfExtension(const std::string& input_method_id,
                                            const std::string& extension_id);

// Returns true if the |input_method_id| is the extension based xkb keyboard,
// otherwise returns false.
bool UI_BASE_IME_EXPORT
IsKeyboardLayoutExtension(const std::string& input_method_id);

// Returns input method component id from the extension-based InputMethodID
// for component IME extensions. This function does not check that
// |input_method_id| is installed.
std::string UI_BASE_IME_EXPORT
GetComponentIDByInputMethodID(const std::string& input_method_id);

// Gets legacy xkb id (e.g. xkb:us::eng) from the new extension based xkb id
// (e.g. _comp_ime_...xkb:us::eng). If the given id is not prefixed with
// 'xkb:', just return the same as the given id.
std::string UI_BASE_IME_EXPORT
MaybeGetLegacyXkbId(const std::string& input_method_id);

}  // namespace extension_ime_util

}  // namespace chromeos

#endif  // UI_BASE_IME_CHROMEOS_EXTENSION_IME_UTIL_H_
