// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_PROXY_SERIALIZED_FLASH_MENU_H_
#define PPAPI_PROXY_SERIALIZED_FLASH_MENU_H_

#include <string>
#include <vector>

#include "base/macros.h"
#include "ppapi/proxy/ppapi_proxy_export.h"

namespace base {
class Pickle;
class PickleIterator;
}

struct PP_Flash_Menu;

namespace ppapi {
namespace proxy {

class PPAPI_PROXY_EXPORT SerializedFlashMenu {
 public:
  SerializedFlashMenu();
  ~SerializedFlashMenu();

  bool SetPPMenu(const PP_Flash_Menu* menu);

  const PP_Flash_Menu* pp_menu() const { return pp_menu_; }

  void WriteToMessage(base::Pickle* m) const;
  bool ReadFromMessage(const base::Pickle* m, base::PickleIterator* iter);

 private:
  const PP_Flash_Menu* pp_menu_;
  bool own_menu_;
  DISALLOW_COPY_AND_ASSIGN(SerializedFlashMenu);
};

}  // namespace proxy
}  // namespace ppapi

#endif  // PPAPI_PROXY_SERIALIZED_FLASH_MENU_H_
