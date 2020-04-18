/* Copyright (c) 2012 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* From dev/ppb_console_dev.idl modified Mon Nov 14 10:36:01 2011. */

#ifndef PPAPI_C_DEV_PPB_CONSOLE_DEV_H_
#define PPAPI_C_DEV_PPB_CONSOLE_DEV_H_

#include "ppapi/c/pp_bool.h"
#include "ppapi/c/pp_instance.h"
#include "ppapi/c/pp_macros.h"
#include "ppapi/c/pp_stdint.h"
#include "ppapi/c/pp_var.h"

#define PPB_CONSOLE_DEV_INTERFACE_0_1 "PPB_Console(Dev);0.1"
#define PPB_CONSOLE_DEV_INTERFACE PPB_CONSOLE_DEV_INTERFACE_0_1

/**
 * @file
 * This file defines the <code>PPB_Console_Dev</code> interface.
 */


/**
 * @addtogroup Enums
 * @{
 */
typedef enum {
  PP_LOGLEVEL_TIP = 0,
  PP_LOGLEVEL_LOG = 1,
  PP_LOGLEVEL_WARNING = 2,
  PP_LOGLEVEL_ERROR = 3
} PP_LogLevel_Dev;
PP_COMPILE_ASSERT_SIZE_IN_BYTES(PP_LogLevel_Dev, 4);
/**
 * @}
 */

/**
 * @addtogroup Interfaces
 * @{
 */
struct PPB_Console_Dev_0_1 {
  /**
   * Logs the given message to the JavaScript console associated with the
   * given plugin instance with the given logging level. The name of the plugin
   * issuing the log message will be automatically prepended to the message.
   * The value may be any type of Var.
   */
  void (*Log)(PP_Instance instance, PP_LogLevel_Dev level, struct PP_Var value);
  /**
   * Logs a message to the console with the given source information rather
   * than using the internal PPAPI plugin name. The name must be a string var.
   *
   * The regular log function will automatically prepend the name of your
   * plugin to the message as the "source" of the message. Some plugins may
   * wish to override this. For example, if your plugin is a Python
   * interpreter, you would want log messages to contain the source .py file
   * doing the log statement rather than have "python" show up in the console.
   */
  void (*LogWithSource)(PP_Instance instance,
                        PP_LogLevel_Dev level,
                        struct PP_Var source,
                        struct PP_Var value);
};

typedef struct PPB_Console_Dev_0_1 PPB_Console_Dev;
/**
 * @}
 */

#endif  /* PPAPI_C_DEV_PPB_CONSOLE_DEV_H_ */

