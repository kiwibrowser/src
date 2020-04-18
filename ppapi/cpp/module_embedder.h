// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_CPP_MODULE_EMBEDDER_H_
#define PPAPI_CPP_MODULE_EMBEDDER_H_

#include "ppapi/c/ppp.h"

/// @file
/// This file defines the APIs for creating a Module object.
namespace pp {

class Module;

/// This function creates the <code>pp::Module</code> object associated with
/// this module.
///
/// <strong>Note: </strong>NaCl module developers must implement this function.
///
/// @return Returns the module if it was successfully created, or NULL on
/// failure. Upon failure, the module will be unloaded.
pp::Module* CreateModule();

/// Sets the get interface function in the broker process.
///
/// This function is only relevant when you're using the PPB_Broker interface
/// in a trusted native plugin. In this case, you may need to implement
/// PPP_GetInterface when the plugin is loaded in the unsandboxed process.
/// Normally the C++ wrappers implement PPP_GetInterface for you but this
/// doesn't work in the context of the broker process.
//
/// So if you need to implement PPP_* interfaces in the broker process, call
/// this function in your PPP_InitializeBroker implementation which will set
/// up the given function as implementing PPP_GetInterface.
void SetBrokerGetInterfaceFunc(PP_GetInterface_Func broker_get_interface);

}  // namespace pp

#endif  // PPAPI_CPP_MODULE_EMBEDDER_H_
