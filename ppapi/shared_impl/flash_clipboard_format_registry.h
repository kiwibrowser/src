// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_SHARED_IMPL_FLASH_CLIPBOARD_FORMAT_REGISTRY_H_
#define PPAPI_SHARED_IMPL_FLASH_CLIPBOARD_FORMAT_REGISTRY_H_

#include <stdint.h>

#include <map>
#include <string>

#include "base/macros.h"
#include "ppapi/c/private/ppb_flash_clipboard.h"
#include "ppapi/shared_impl/ppapi_shared_export.h"

namespace ppapi {

// This class manages custom clipboard formats that can be registered by a user
// of PPB_Flash_Clipboard. It is used by both the plugin and host side
// of the proxy. The host side is the definitive source of which formats
// have been registered but format registration is tracked on the plugin
// side to better handle error cases.
class PPAPI_SHARED_EXPORT FlashClipboardFormatRegistry {
 public:
  FlashClipboardFormatRegistry();
  ~FlashClipboardFormatRegistry();

  // Registers a custom format with the given string. The ID of the format is
  // returned if registered successfully, otherwise
  // PP_FLASH_CLIPBOARD_FORMAT_INVALID is returned. If a format with a
  // particular name is already registered, the ID associated with that name
  // will be returned. The format name is checked for validity.
  uint32_t RegisterFormat(const std::string& format_name);

  // This sets the name of a particular format in the registry. This is only
  // used on the plugin side in order to track format IDs that are returned
  // by the host.
  void SetRegisteredFormat(const std::string& format_name, uint32_t format);

  // Checks whether the given custom format ID has been registered.
  bool IsFormatRegistered(uint32_t format) const;

  // Returns the name of a particular format.
  std::string GetFormatName(uint32_t format) const;

  // Gets the ID of a particular format name that has already been registered.
  // PP_FLASH_CLIPBOARD_FORMAT_INVALID is returned if the format has not been
  // registered.
  uint32_t GetFormatID(const std::string& format_name) const;

  // Returns whether the given clipboard format is a valid predefined format
  // (i.e. one defined in the PP_Flash_Clipboard_Format enum).
  static bool IsValidPredefinedFormat(uint32_t format);

 private:
  // A map of custom format ID to format name.
  typedef std::map<uint32_t, std::string> FormatMap;
  FormatMap custom_formats_;

  DISALLOW_COPY_AND_ASSIGN(FlashClipboardFormatRegistry);
};

}  // ppapi

#endif  // PPAPI_SHARED_IMPL_FLASH_CLIPBOARD_FORMAT_REGISTRY_H_
