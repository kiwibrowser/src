// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/proxy/serialized_flash_menu.h"

#include <stdint.h>

#include "ipc/ipc_message.h"
#include "ppapi/c/private/ppb_flash_menu.h"
#include "ppapi/proxy/ppapi_param_traits.h"

namespace ppapi {
namespace proxy {

namespace {
// Maximum depth of submenus allowed (e.g., 1 indicates that submenus are
// allowed, but not sub-submenus).
const int kMaxMenuDepth = 2;
const uint32_t kMaxMenuEntries = 1000;

bool CheckMenu(int depth, const PP_Flash_Menu* menu);
void FreeMenu(const PP_Flash_Menu* menu);
void WriteMenu(base::Pickle* m, const PP_Flash_Menu* menu);
PP_Flash_Menu* ReadMenu(int depth,
                        const base::Pickle* m,
                        base::PickleIterator* iter);

bool CheckMenuItem(int depth, const PP_Flash_MenuItem* item) {
  if (item->type == PP_FLASH_MENUITEM_TYPE_SUBMENU)
    return CheckMenu(depth, item->submenu);
  return true;
}

bool CheckMenu(int depth, const PP_Flash_Menu* menu) {
  if (depth > kMaxMenuDepth || !menu)
    return false;
  ++depth;

  if (menu->count && !menu->items)
    return false;

  for (uint32_t i = 0; i < menu->count; ++i) {
    if (!CheckMenuItem(depth, menu->items + i))
      return false;
  }
  return true;
}

void WriteMenuItem(base::Pickle* m, const PP_Flash_MenuItem* menu_item) {
  PP_Flash_MenuItem_Type type = menu_item->type;
  m->WriteUInt32(type);
  m->WriteString(menu_item->name ? menu_item->name : "");
  m->WriteInt(menu_item->id);
  IPC::WriteParam(m, menu_item->enabled);
  IPC::WriteParam(m, menu_item->checked);
  if (type == PP_FLASH_MENUITEM_TYPE_SUBMENU)
    WriteMenu(m, menu_item->submenu);
}

void WriteMenu(base::Pickle* m, const PP_Flash_Menu* menu) {
  m->WriteUInt32(menu->count);
  for (uint32_t i = 0; i < menu->count; ++i)
    WriteMenuItem(m, menu->items + i);
}

void FreeMenuItem(const PP_Flash_MenuItem* menu_item) {
  if (menu_item->name)
    delete [] menu_item->name;
  if (menu_item->submenu)
    FreeMenu(menu_item->submenu);
}

void FreeMenu(const PP_Flash_Menu* menu) {
  if (menu->items) {
    for (uint32_t i = 0; i < menu->count; ++i)
      FreeMenuItem(menu->items + i);
    delete [] menu->items;
  }
  delete menu;
}

bool ReadMenuItem(int depth,
                  const base::Pickle* m,
                  base::PickleIterator* iter,
                  PP_Flash_MenuItem* menu_item) {
  uint32_t type;
  if (!iter->ReadUInt32(&type))
    return false;
  if (type > PP_FLASH_MENUITEM_TYPE_SUBMENU)
    return false;
  menu_item->type = static_cast<PP_Flash_MenuItem_Type>(type);
  std::string name;
  if (!iter->ReadString(&name))
    return false;
  menu_item->name = new char[name.size() + 1];
  std::copy(name.begin(), name.end(), menu_item->name);
  menu_item->name[name.size()] = 0;
  if (!iter->ReadInt(&menu_item->id))
    return false;
  if (!IPC::ReadParam(m, iter, &menu_item->enabled))
    return false;
  if (!IPC::ReadParam(m, iter, &menu_item->checked))
    return false;
  if (type == PP_FLASH_MENUITEM_TYPE_SUBMENU) {
    menu_item->submenu = ReadMenu(depth, m, iter);
    if (!menu_item->submenu)
      return false;
  }
  return true;
}

PP_Flash_Menu* ReadMenu(int depth,
                        const base::Pickle* m,
                        base::PickleIterator* iter) {
  if (depth > kMaxMenuDepth)
    return NULL;
  ++depth;

  PP_Flash_Menu* menu = new PP_Flash_Menu;
  menu->items = NULL;

  if (!iter->ReadUInt32(&menu->count)) {
    FreeMenu(menu);
    return NULL;
  }

  if (menu->count == 0)
    return menu;

  if (menu->count > kMaxMenuEntries) {
    FreeMenu(menu);
    return NULL;
  }

  menu->items = new PP_Flash_MenuItem[menu->count];
  memset(menu->items, 0, sizeof(PP_Flash_MenuItem) * menu->count);
  for (uint32_t i = 0; i < menu->count; ++i) {
    if (!ReadMenuItem(depth, m, iter, menu->items + i)) {
      FreeMenu(menu);
      return NULL;
    }
  }
  return menu;
}

}  // anonymous namespace

SerializedFlashMenu::SerializedFlashMenu()
    : pp_menu_(NULL),
      own_menu_(false) {
}

SerializedFlashMenu::~SerializedFlashMenu() {
  if (own_menu_)
    FreeMenu(pp_menu_);
}

bool SerializedFlashMenu::SetPPMenu(const PP_Flash_Menu* menu) {
  DCHECK(!pp_menu_);
  if (!CheckMenu(0, menu))
    return false;
  pp_menu_ = menu;
  own_menu_ = false;
  return true;
}

void SerializedFlashMenu::WriteToMessage(base::Pickle* m) const {
  WriteMenu(m, pp_menu_);
}

bool SerializedFlashMenu::ReadFromMessage(const base::Pickle* m,
                                          base::PickleIterator* iter) {
  DCHECK(!pp_menu_);
  pp_menu_ = ReadMenu(0, m, iter);
  if (!pp_menu_)
    return false;

  own_menu_ = true;
  return true;
}

}  // namespace proxy
}  // namespace ppapi
