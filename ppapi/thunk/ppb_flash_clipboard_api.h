// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_THUNK_PPB_FLASH_CLIPBOARD_API_H_
#define PPAPI_THUNK_PPB_FLASH_CLIPBOARD_API_H_

#include <stdint.h>

#include "ppapi/c/private/ppb_flash_clipboard.h"
#include "ppapi/shared_impl/singleton_resource_id.h"

namespace ppapi {
namespace thunk {

class PPB_Flash_Clipboard_API {
 public:
  virtual ~PPB_Flash_Clipboard_API() {}

  // PPB_Flash_Clipboard.
  virtual uint32_t RegisterCustomFormat(PP_Instance instance,
                                        const char* format_name) = 0;
  virtual PP_Bool IsFormatAvailable(PP_Instance instance,
                                    PP_Flash_Clipboard_Type clipboard_type,
                                    uint32_t format) = 0;
  virtual PP_Var ReadData(PP_Instance instance,
                          PP_Flash_Clipboard_Type clipboard_type,
                          uint32_t format) = 0;
  virtual int32_t WriteData(PP_Instance instance,
                            PP_Flash_Clipboard_Type clipboard_type,
                            uint32_t data_item_count,
                            const uint32_t formats[],
                            const PP_Var data_items[]) = 0;
  virtual PP_Bool GetSequenceNumber(
      PP_Instance instance,
      PP_Flash_Clipboard_Type clipboard_type,
      uint64_t* sequence_number) = 0;

  static const SingletonResourceID kSingletonResourceID =
      FLASH_CLIPBOARD_SINGLETON_ID;
};

}  // namespace thunk
}  // namespace ppapi

#endif // PPAPI_THUNK_PPB_FLASH_CLIPBOARD_API_H_
