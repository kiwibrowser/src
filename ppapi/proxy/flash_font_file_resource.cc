// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/proxy/flash_font_file_resource.h"

#include <cstring>
#include <memory>

#include "ppapi/c/pp_errors.h"
#include "ppapi/proxy/ppapi_messages.h"

namespace ppapi {
namespace proxy {

FlashFontFileResource::FlashFontFileResource(
    Connection connection,
    PP_Instance instance,
    const PP_BrowserFont_Trusted_Description* description,
    PP_PrivateFontCharset charset)
    : PluginResource(connection, instance),
      charset_(charset) {
  description_.SetFromPPBrowserFontDescription(*description);
}

FlashFontFileResource::~FlashFontFileResource() {
}

thunk::PPB_Flash_FontFile_API*
    FlashFontFileResource::AsPPB_Flash_FontFile_API() {
  return this;
}

PP_Bool FlashFontFileResource::GetFontTable(uint32_t table,
                                            void* output,
                                            uint32_t* output_length) {
  if (!output_length)
    return PP_FALSE;

  if (!sent_create_to_renderer()) {
    SendCreate(
        RENDERER, PpapiHostMsg_FlashFontFile_Create(description_, charset_));
  }

  const std::string* contents = GetFontTable(table);
  if (!contents) {
    std::string out_contents;
    int32_t result = SyncCall<PpapiPluginMsg_FlashFontFile_GetFontTableReply>(
        RENDERER, PpapiHostMsg_FlashFontFile_GetFontTable(table),
        &out_contents);
    if (result != PP_OK)
      return PP_FALSE;

    contents = AddFontTable(table, out_contents);
  }

  // If we are going to copy the data into |output|, it must be big enough.
  if (output && *output_length < contents->size())
    return PP_FALSE;

  *output_length = static_cast<uint32_t>(contents->size());
  if (output)
    memcpy(output, contents->c_str(), *output_length);
  return PP_TRUE;
}

const std::string* FlashFontFileResource::GetFontTable(uint32_t table) const {
  auto found = font_tables_.find(table);
  return (found != font_tables_.end()) ? found->second.get() : nullptr;
}

const std::string* FlashFontFileResource::AddFontTable(
    uint32_t table,
    const std::string& contents) {
  auto contents_copy = std::make_unique<std::string>(contents);
  std::string* contents_copy_ptr = contents_copy.get();
  font_tables_[table] = std::move(contents_copy);
  return contents_copy_ptr;
}

}  // namespace proxy
}  // namespace ppapi
