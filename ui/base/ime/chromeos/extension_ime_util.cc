// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/ime/chromeos/extension_ime_util.h"

#include "base/strings/string_util.h"

namespace chromeos {

namespace {

const char kExtensionIMEPrefix[] = "_ext_ime_";
const int kExtensionIMEPrefixLength =
    sizeof(kExtensionIMEPrefix) / sizeof(kExtensionIMEPrefix[0]) - 1;
const char kComponentExtensionIMEPrefix[] = "_comp_ime_";
const int kComponentExtensionIMEPrefixLength =
    sizeof(kComponentExtensionIMEPrefix) /
        sizeof(kComponentExtensionIMEPrefix[0]) - 1;
const int kExtensionIdLength = 32;

}  // namespace

namespace extension_ime_util {

const char kBrailleImeExtensionId[] = "jddehjeebkoimngcbdkaahpobgicbffp";
const char kBrailleImeExtensionPath[] = "chromeos/braille_ime";
const char kBrailleImeEngineId[] =
    "_comp_ime_jddehjeebkoimngcbdkaahpobgicbffpbraille";

std::string GetInputMethodID(const std::string& extension_id,
                             const std::string& engine_id) {
  DCHECK(!extension_id.empty());
  DCHECK(!engine_id.empty());
  return kExtensionIMEPrefix + extension_id + engine_id;
}

std::string GetComponentInputMethodID(const std::string& extension_id,
                                      const std::string& engine_id) {
  DCHECK(!extension_id.empty());
  DCHECK(!engine_id.empty());
  return kComponentExtensionIMEPrefix + extension_id + engine_id;
}

std::string GetExtensionIDFromInputMethodID(
    const std::string& input_method_id) {
  if (IsExtensionIME(input_method_id)) {
    return input_method_id.substr(kExtensionIMEPrefixLength,
                                  kExtensionIdLength);
  }
  if (IsComponentExtensionIME(input_method_id)) {
    return input_method_id.substr(kComponentExtensionIMEPrefixLength,
                                  kExtensionIdLength);
  }
  return "";
}

std::string GetComponentIDByInputMethodID(const std::string& input_method_id) {
  if (IsComponentExtensionIME(input_method_id))
    return input_method_id.substr(kComponentExtensionIMEPrefixLength +
                                  kExtensionIdLength);
  if (IsExtensionIME(input_method_id))
    return input_method_id.substr(kExtensionIMEPrefixLength +
                                  kExtensionIdLength);
  return input_method_id;
}

std::string GetInputMethodIDByEngineID(const std::string& engine_id) {
  if (base::StartsWith(engine_id, kComponentExtensionIMEPrefix,
                       base::CompareCase::SENSITIVE) ||
      base::StartsWith(engine_id, kExtensionIMEPrefix,
                       base::CompareCase::SENSITIVE)) {
    return engine_id;
  }
  if (base::StartsWith(engine_id, "xkb:", base::CompareCase::SENSITIVE))
    return GetComponentInputMethodID(kXkbExtensionId, engine_id);
  if (base::StartsWith(engine_id, "vkd_", base::CompareCase::SENSITIVE))
    return GetComponentInputMethodID(kM17nExtensionId, engine_id);
  if (base::StartsWith(engine_id, "nacl_mozc_", base::CompareCase::SENSITIVE))
    return GetComponentInputMethodID(kMozcExtensionId, engine_id);
  if (base::StartsWith(engine_id, "hangul_",
                       base::CompareCase::SENSITIVE))
    return GetComponentInputMethodID(kHangulExtensionId, engine_id);

  if (base::StartsWith(engine_id, "zh-", base::CompareCase::SENSITIVE) &&
      engine_id.find("pinyin") != std::string::npos) {
    return GetComponentInputMethodID(kChinesePinyinExtensionId, engine_id);
  }
  if (base::StartsWith(engine_id, "zh-", base::CompareCase::SENSITIVE) &&
      engine_id.find("zhuyin") != std::string::npos) {
    return GetComponentInputMethodID(kChineseZhuyinExtensionId, engine_id);
  }
  if (base::StartsWith(engine_id, "zh-", base::CompareCase::SENSITIVE) &&
      engine_id.find("cangjie") != std::string::npos) {
    return GetComponentInputMethodID(kChineseCangjieExtensionId, engine_id);
  }
  if (engine_id.find("-t-i0-") != std::string::npos)
    return GetComponentInputMethodID(kT13nExtensionId, engine_id);

  return engine_id;
}

bool IsExtensionIME(const std::string& input_method_id) {
  return base::StartsWith(input_method_id, kExtensionIMEPrefix,
                          base::CompareCase::SENSITIVE) &&
         input_method_id.size() >
             kExtensionIMEPrefixLength + kExtensionIdLength;
}

bool IsComponentExtensionIME(const std::string& input_method_id) {
  return base::StartsWith(input_method_id, kComponentExtensionIMEPrefix,
                          base::CompareCase::SENSITIVE) &&
         input_method_id.size() >
             kComponentExtensionIMEPrefixLength + kExtensionIdLength;
}

bool IsMemberOfExtension(const std::string& input_method_id,
                         const std::string& extension_id) {
  return base::StartsWith(input_method_id,
                          kExtensionIMEPrefix + extension_id,
                          base::CompareCase::SENSITIVE);
}

bool IsKeyboardLayoutExtension(const std::string& input_method_id) {
  if (IsComponentExtensionIME(input_method_id))
    return base::StartsWith(GetComponentIDByInputMethodID(input_method_id),
                            "xkb:", base::CompareCase::SENSITIVE);
  return false;
}

std::string MaybeGetLegacyXkbId(const std::string& input_method_id) {
  if (IsKeyboardLayoutExtension(input_method_id))
    return GetComponentIDByInputMethodID(input_method_id);
  return input_method_id;
}

}  // namespace extension_ime_util
}  // namespace chromeos
