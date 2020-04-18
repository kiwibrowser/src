/* Copyright (c) 2012 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* From trusted/ppb_broker_trusted.idl modified Mon Dec  3 11:10:40 2012. */

#ifndef PPAPI_C_TRUSTED_PPB_BROKER_TRUSTED_H_
#define PPAPI_C_TRUSTED_PPB_BROKER_TRUSTED_H_

#include "ppapi/c/pp_bool.h"
#include "ppapi/c/pp_completion_callback.h"
#include "ppapi/c/pp_instance.h"
#include "ppapi/c/pp_macros.h"
#include "ppapi/c/pp_resource.h"
#include "ppapi/c/pp_stdint.h"

#define PPB_BROKER_TRUSTED_INTERFACE_0_2 "PPB_BrokerTrusted;0.2"
#define PPB_BROKER_TRUSTED_INTERFACE_0_3 "PPB_BrokerTrusted;0.3"
#define PPB_BROKER_TRUSTED_INTERFACE PPB_BROKER_TRUSTED_INTERFACE_0_3

/**
 * @file
 * This file defines the PPB_BrokerTrusted interface, which provides
 * access to a trusted broker with greater privileges than the plugin.
 */


/**
 * @addtogroup Interfaces
 * @{
 */
/**
 * The PPB_BrokerTrusted interface provides access to a trusted broker
 * with greater privileges than the plugin. The interface only supports
 * out-of-process plugins and is to be used by proxy implementations.  All
 * functions should be called from the main thread only.
 *
 * A PPB_BrokerTrusted resource represents a connection to the broker. Its
 * lifetime controls the lifetime of the broker, regardless of whether the
 * handle is closed. The handle should be closed before the resource is
 * released.
 */
struct PPB_BrokerTrusted_0_3 {
  /**
   * Returns a trusted broker resource.
   */
  PP_Resource (*CreateTrusted)(PP_Instance instance);
  /**
   * Returns true if the resource is a trusted broker.
   */
  PP_Bool (*IsBrokerTrusted)(PP_Resource resource);
  /**
   * Connects to the trusted broker. It may have already
   * been launched by another instance.
   * The plugin takes ownership of the handle once the callback has been called
   * with a result of PP_OK. The plugin should immediately call GetHandle and
   * begin managing it. If the result is not PP_OK, the browser still owns the
   * handle.
   *
   * Returns PP_ERROR_WOULD_BLOCK on success, and invokes
   * the |connect_callback| asynchronously to complete.
   * As this function should always be invoked from the main thread,
   * do not use the blocking variant of PP_CompletionCallback.
   * Returns PP_ERROR_FAILED if called from an in-process plugin.
   */
  int32_t (*Connect)(PP_Resource broker,
                     struct PP_CompletionCallback connect_callback);
  /**
   * Gets the handle to the pipe. Use once Connect has completed. Each instance
   * of this interface has its own pipe.
   *
   * Returns PP_OK on success, and places the result into the given output
   * parameter. The handle is only set when returning PP_OK. Calling this
   * before connect has completed will return PP_ERROR_FAILED.
   */
  int32_t (*GetHandle)(PP_Resource broker, int32_t* handle);
  /**
   * Returns PP_TRUE if the plugin has permission to launch the broker. A user
   * must explicitly grant permission to launch the broker for a particular
   * website. This is done through an infobar that is displayed when |Connect|
   * is called. This function returns PP_TRUE if the user has already granted
   * permission to launch the broker for the website containing this plugin
   * instance. Returns PP_FALSE otherwise.
   */
  PP_Bool (*IsAllowed)(PP_Resource broker);
};

typedef struct PPB_BrokerTrusted_0_3 PPB_BrokerTrusted;

struct PPB_BrokerTrusted_0_2 {
  PP_Resource (*CreateTrusted)(PP_Instance instance);
  PP_Bool (*IsBrokerTrusted)(PP_Resource resource);
  int32_t (*Connect)(PP_Resource broker,
                     struct PP_CompletionCallback connect_callback);
  int32_t (*GetHandle)(PP_Resource broker, int32_t* handle);
};
/**
 * @}
 */

#endif  /* PPAPI_C_TRUSTED_PPB_BROKER_TRUSTED_H_ */

