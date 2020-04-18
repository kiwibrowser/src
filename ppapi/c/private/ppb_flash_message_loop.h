/* Copyright (c) 2012 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* From private/ppb_flash_message_loop.idl modified Tue Jan 17 17:48:30 2012. */

#ifndef PPAPI_C_PRIVATE_PPB_FLASH_MESSAGE_LOOP_H_
#define PPAPI_C_PRIVATE_PPB_FLASH_MESSAGE_LOOP_H_

#include "ppapi/c/pp_bool.h"
#include "ppapi/c/pp_instance.h"
#include "ppapi/c/pp_macros.h"
#include "ppapi/c/pp_resource.h"
#include "ppapi/c/pp_stdint.h"

#define PPB_FLASH_MESSAGELOOP_INTERFACE_0_1 "PPB_Flash_MessageLoop;0.1"
#define PPB_FLASH_MESSAGELOOP_INTERFACE PPB_FLASH_MESSAGELOOP_INTERFACE_0_1

/**
 * @file
 * This file contains the <code>PPB_Flash_MessageLoop</code> interface.
 */


/**
 * @addtogroup Interfaces
 * @{
 */
/**
 * The <code>PPB_Flash_MessageLoop</code> interface supports Pepper Flash to run
 * nested run loops.
 */
struct PPB_Flash_MessageLoop_0_1 {
  /**
   * Allocates a Flash message loop resource.
   *
   * @param[in] instance A <code>PP_Instance</code> identifying one instance
   * of a module.
   *
   * @return A <code>PP_Resource</code> that can be used to run a nested message
   * loop if successful; 0 if failed.
   */
  PP_Resource (*Create)(PP_Instance instance);
  /**
   * Determines if a given resource is a Flash message loop.
   *
   * @param[in] resource A <code>PP_Resource</code> corresponding to a generic
   * resource.
   *
   * @return A <code>PP_Bool</code> that is <code>PP_TRUE</code> if the given
   * resource is a Flash message loop, otherwise <code>PP_FALSE</code>.
   */
  PP_Bool (*IsFlashMessageLoop)(PP_Resource resource);
  /**
   * Runs a nested run loop. The plugin will be reentered from this call.
   * This function is used in places where Flash would normally enter a nested
   * message loop (e.g., when displaying context menus), but Pepper provides
   * only an asynchronous call. After performing that asynchronous call, call
   * <code>Run()</code>. In the callback, call <code>Quit()</code>.
   *
   * For a given message loop resource, only the first call to
   * <code>Run()</code> will start a nested run loop. The subsequent calls
   * will return <code>PP_ERROR_FAILED</code> immediately.
   *
   * @param[in] flash_message_loop The Flash message loop.
   *
   * @return <code>PP_ERROR_ABORTED</code> if the message loop quits because the
   * resource is destroyed; <code>PP_OK</code> if the message loop quits because
   * of other reasons (e.g., <code>Quit()</code> is called);
   * <code>PP_ERROR_FAILED</code> if this is not the first call to
   * <code>Run()</code>.
   */
  int32_t (*Run)(PP_Resource flash_message_loop);
  /**
   * Signals to quit the outermost nested run loop. Use this to exit and
   * return back to the caller after you call <code>Run()</code>.
   *
   * If <code>Quit()</code> is not called to balance the call to
   * <code>Run()</code>, the outermost nested run loop will be quitted
   * implicitly when the resource is destroyed.
   *
   * @param[in] flash_message_loop The Flash message loop.
   */
  void (*Quit)(PP_Resource flash_message_loop);
};

typedef struct PPB_Flash_MessageLoop_0_1 PPB_Flash_MessageLoop;
/**
 * @}
 */

#endif  /* PPAPI_C_PRIVATE_PPB_FLASH_MESSAGE_LOOP_H_ */

