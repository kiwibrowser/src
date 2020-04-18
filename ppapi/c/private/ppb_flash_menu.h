/* Copyright (c) 2012 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* From private/ppb_flash_menu.idl modified Tue Dec 11 13:47:09 2012. */

#ifndef PPAPI_C_PRIVATE_PPB_FLASH_MENU_H_
#define PPAPI_C_PRIVATE_PPB_FLASH_MENU_H_

#include "ppapi/c/pp_bool.h"
#include "ppapi/c/pp_completion_callback.h"
#include "ppapi/c/pp_instance.h"
#include "ppapi/c/pp_macros.h"
#include "ppapi/c/pp_point.h"
#include "ppapi/c/pp_resource.h"
#include "ppapi/c/pp_stdint.h"

/* Struct prototypes */
struct PP_Flash_Menu;

#define PPB_FLASH_MENU_INTERFACE_0_2 "PPB_Flash_Menu;0.2"
#define PPB_FLASH_MENU_INTERFACE PPB_FLASH_MENU_INTERFACE_0_2

/**
 * @file
 * This file defines the <code>PPB_Flash_Menu</code> interface.
 */


/**
 * @addtogroup Enums
 * @{
 */
/* Menu item type.
 *
 * TODO(viettrungluu): Radio items not supported yet. Will also probably want
 * special menu items tied to clipboard access.
 */
typedef enum {
  PP_FLASH_MENUITEM_TYPE_NORMAL = 0,
  PP_FLASH_MENUITEM_TYPE_CHECKBOX = 1,
  PP_FLASH_MENUITEM_TYPE_SEPARATOR = 2,
  PP_FLASH_MENUITEM_TYPE_SUBMENU = 3
} PP_Flash_MenuItem_Type;
PP_COMPILE_ASSERT_SIZE_IN_BYTES(PP_Flash_MenuItem_Type, 4);
/**
 * @}
 */

/**
 * @addtogroup Structs
 * @{
 */
struct PP_Flash_MenuItem {
  PP_Flash_MenuItem_Type type;
  char* name;
  int32_t id;
  PP_Bool enabled;
  PP_Bool checked;
  struct PP_Flash_Menu* submenu;
};

struct PP_Flash_Menu {
  uint32_t count;
  struct PP_Flash_MenuItem *items;
};
/**
 * @}
 */

/**
 * @addtogroup Interfaces
 * @{
 */
struct PPB_Flash_Menu_0_2 {
  PP_Resource (*Create)(PP_Instance instance_id,
                        const struct PP_Flash_Menu* menu_data);
  PP_Bool (*IsFlashMenu)(PP_Resource resource_id);
  /* Display a context menu at the given location. If the user selects an item,
   * |selected_id| will be set to its |id| and the callback called with |PP_OK|.
   * If the user dismisses the menu without selecting an item,
   * |PP_ERROR_USERCANCEL| will be indicated.
   */
  int32_t (*Show)(PP_Resource menu_id,
                  const struct PP_Point* location,
                  int32_t* selected_id,
                  struct PP_CompletionCallback callback);
};

typedef struct PPB_Flash_Menu_0_2 PPB_Flash_Menu;
/**
 * @}
 */

#endif  /* PPAPI_C_PRIVATE_PPB_FLASH_MENU_H_ */

