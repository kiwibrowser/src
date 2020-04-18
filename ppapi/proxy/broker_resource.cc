// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/proxy/broker_resource.h"

#include <stdint.h>

#include "ppapi/c/pp_bool.h"
#include "ppapi/proxy/ppapi_messages.h"

namespace ppapi {
namespace proxy {

BrokerResource::BrokerResource(Connection connection, PP_Instance instance)
    : PluginResource(connection, instance) {
  SendCreate(BROWSER, PpapiHostMsg_Broker_Create());
}

BrokerResource::~BrokerResource() {
}

thunk::PPB_Broker_Instance_API* BrokerResource::AsPPB_Broker_Instance_API() {
  return this;
}

PP_Bool BrokerResource::IsAllowed() {
  int32_t result =
      SyncCall<IPC::Message>(BROWSER, PpapiHostMsg_Broker_IsAllowed());
  return PP_FromBool(result == PP_OK);
}

}  // namespace proxy
}  // namespace ppapi
