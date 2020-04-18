// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/shared_impl/flash_clipboard_format_registry.h"

#include <stddef.h>

#include <cctype>

#include "base/macros.h"
#include "base/numerics/safe_conversions.h"

namespace ppapi {

namespace {

// These values are chosen arbitrarily. Flash will never exceed these but if
// the interface becomes public, we can reconsider these.
const size_t kMaxNumFormats = 10;
const size_t kMaxFormatNameLength = 50;

// All formats in PP_Flash_Clipboard_Format should be added here.
const PP_Flash_Clipboard_Format kPredefinedFormats[] = {
    PP_FLASH_CLIPBOARD_FORMAT_INVALID, PP_FLASH_CLIPBOARD_FORMAT_PLAINTEXT,
    PP_FLASH_CLIPBOARD_FORMAT_HTML,    PP_FLASH_CLIPBOARD_FORMAT_RTF};

// The first custom format ID will be the ID after that max value in
// PP_Flash_Clipboard_Format.
const size_t kFirstCustomFormat = arraysize(kPredefinedFormats);

// Checks the validity of the given format name.
bool IsValidFormatName(const std::string& format_name) {
  if (format_name.empty() || format_name.length() > kMaxFormatNameLength)
    return false;
  return true;
}

}  // namespace

FlashClipboardFormatRegistry::FlashClipboardFormatRegistry() {}

FlashClipboardFormatRegistry::~FlashClipboardFormatRegistry() {}

uint32_t FlashClipboardFormatRegistry::RegisterFormat(
    const std::string& format_name) {
  if (!IsValidFormatName(format_name) ||
      custom_formats_.size() > kMaxNumFormats) {
    return PP_FLASH_CLIPBOARD_FORMAT_INVALID;
  }
  uint32_t key = kFirstCustomFormat +
                 base::checked_cast<uint32_t>(custom_formats_.size());
  custom_formats_[key] = format_name;
  return key;
}

void FlashClipboardFormatRegistry::SetRegisteredFormat(
    const std::string& format_name,
    uint32_t format) {
  custom_formats_[format] = format_name;
}

bool FlashClipboardFormatRegistry::IsFormatRegistered(uint32_t format) const {
  return custom_formats_.find(format) != custom_formats_.end();
}

std::string FlashClipboardFormatRegistry::GetFormatName(uint32_t format) const {
  FormatMap::const_iterator it = custom_formats_.find(format);
  if (it == custom_formats_.end())
    return std::string();
  return it->second;
}

uint32_t FlashClipboardFormatRegistry::GetFormatID(
    const std::string& format_name) const {
  for (FormatMap::const_iterator it = custom_formats_.begin();
       it != custom_formats_.end();
       ++it) {
    if (it->second == format_name)
      return it->first;
  }
  return PP_FLASH_CLIPBOARD_FORMAT_INVALID;
}

// static
bool FlashClipboardFormatRegistry::IsValidPredefinedFormat(uint32_t format) {
  if (format == PP_FLASH_CLIPBOARD_FORMAT_INVALID)
    return false;
  return format < kFirstCustomFormat;
}

}  // namespace ppapi
