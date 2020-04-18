/* Copyright 2014 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* From private/ppb_find_private.idl modified Wed Mar 19 13:42:13 2014. */

#ifndef PPAPI_C_PRIVATE_PPB_FIND_PRIVATE_H_
#define PPAPI_C_PRIVATE_PPB_FIND_PRIVATE_H_

#include "ppapi/c/pp_bool.h"
#include "ppapi/c/pp_instance.h"
#include "ppapi/c/pp_macros.h"
#include "ppapi/c/pp_point.h"
#include "ppapi/c/pp_rect.h"
#include "ppapi/c/pp_size.h"
#include "ppapi/c/pp_stdint.h"

#define PPB_FIND_PRIVATE_INTERFACE_0_3 "PPB_Find_Private;0.3"
#define PPB_FIND_PRIVATE_INTERFACE PPB_FIND_PRIVATE_INTERFACE_0_3

/**
 * @file
 * This file defines the <code>PPB_Find_Private</code> interface.
 */


/**
 * @addtogroup Interfaces
 * @{
 */
/**
 * This is a private interface for doing browser Find in the PDF plugin.
 */
struct PPB_Find_Private_0_3 {
  /**
   * Sets the instance of this plugin as the mechanism that will be used to
   * handle find requests in the renderer. This will only succeed if the plugin
   * is embedded within the content of the top level frame. Note that this will
   * result in the renderer handing over all responsibility for doing find to
   * the plugin and content from the rest of the page will not be searched.
   *
   *
   * In the case that the plugin is loaded directly as the top level document,
   * this function does not need to be called. In that case the plugin is
   * assumed to handle find requests.
   *
   * There can only be one plugin which handles find requests. If a plugin calls
   * this while an existing plugin is registered, the existing plugin will be
   * de-registered and will no longer receive any requests.
   */
  void (*SetPluginToHandleFindRequests)(PP_Instance instance);
  /**
   * Updates the number of find results for the current search term.  If
   * there are no matches 0 should be passed in.  Only when the plugin has
   * finished searching should it pass in the final count with final_result set
   * to PP_TRUE.
   */
  void (*NumberOfFindResultsChanged)(PP_Instance instance,
                                     int32_t total,
                                     PP_Bool final_result);
  /**
   * Updates the index of the currently selected search item.
   */
  void (*SelectedFindResultChanged)(PP_Instance instance, int32_t index);
  /**
   * Updates the tickmarks on the scrollbar for the find request. |tickmarks|
   * contains |count| PP_Rects indicating the tickmark ranges.
   */
  void (*SetTickmarks)(PP_Instance instance,
                       const struct PP_Rect tickmarks[],
                       uint32_t count);
};

typedef struct PPB_Find_Private_0_3 PPB_Find_Private;
/**
 * @}
 */

#endif  /* PPAPI_C_PRIVATE_PPB_FIND_PRIVATE_H_ */

