// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_CPP_PRIVATE_FLASH_FONT_FILE_H_
#define PPAPI_CPP_PRIVATE_FLASH_FONT_FILE_H_

#include <stdint.h>

#include "ppapi/c/private/pp_private_font_charset.h"
#include "ppapi/cpp/resource.h"

struct PP_BrowserFont_Trusted_Description;

namespace pp {

class InstanceHandle;

namespace flash {

class FontFile : public Resource {
 public:
  // Default constructor for making an is_null() FontFile resource.
  FontFile();
  FontFile(const InstanceHandle& instance,
           const PP_BrowserFont_Trusted_Description* description,
           PP_PrivateFontCharset charset);
  virtual ~FontFile();

  // Returns true if the required interface is available.
  static bool IsAvailable();

  // Returns true if this interface is supported for Windows.
  bool IsSupportedForWindows();

  bool GetFontTable(uint32_t table, void* output, uint32_t* output_length);
};

}  // namespace flash
}  // namespace pp

#endif  // PPAPI_CPP_PRIVATE_FLASH_FONT_FILE_H_
