// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_PEPPER_PEPPER_TRUETYPE_FONT_H_
#define CONTENT_BROWSER_RENDERER_HOST_PEPPER_PEPPER_TRUETYPE_FONT_H_

#include <stdint.h>

#include <string>
#include <vector>

#include "base/memory/ref_counted.h"
#include "ppapi/proxy/serialized_structs.h"

namespace content {

class PepperTrueTypeFont
    : public base::RefCountedThreadSafe<PepperTrueTypeFont> {
 public:
  // Factory method to create a font for the current host.
  static PepperTrueTypeFont* Create();

  // Initializes the font. Updates the descriptor with the actual font's
  // characteristics. The exact font will depend on the host platform's font
  // matching and fallback algorithm. On failure, returns NULL and leaves desc
  // unchanged.
  // NOTE: This method may perform long blocking file IO.
  virtual int32_t Initialize(
      ppapi::proxy::SerializedTrueTypeFontDesc* desc) = 0;

  // Retrieves an array of TrueType table tags contained in this font. Returns
  // the number of tags on success, a Pepper error code on failure. 'tags' are
  // written only on success.
  // NOTE: This method may perform long blocking file IO. It may be called even
  // though the call to Initialize failed. Implementors must check validity.
  virtual int32_t GetTableTags(std::vector<uint32_t>* tags) = 0;

  // Gets a TrueType font table corresponding to the given tag. The 'offset' and
  // 'max_data_length' parameters determine what part of the table is returned.
  // Returns the data size in bytes on success, a Pepper error code on failure.
  // 'data' is written only on success.
  // NOTE: This method may perform long blocking file IO. It may be called even
  // though the call to Initialize failed. Implementors must check validity.
  virtual int32_t GetTable(uint32_t table_tag,
                           int32_t offset,
                           int32_t max_data_length,
                           std::string* data) = 0;

 protected:
  friend class base::RefCountedThreadSafe<PepperTrueTypeFont>;
  virtual ~PepperTrueTypeFont() {};
};

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_PEPPER_PEPPER_TRUETYPE_FONT_H_
