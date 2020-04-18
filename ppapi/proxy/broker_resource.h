// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_PROXY_BROKER_RESOURCE_H_
#define PPAPI_PROXY_BROKER_RESOURCE_H_

#include "base/macros.h"
#include "ppapi/proxy/connection.h"
#include "ppapi/proxy/plugin_resource.h"
#include "ppapi/thunk/ppb_broker_api.h"

namespace ppapi {
namespace proxy {

class BrokerResource
    : public PluginResource,
      public thunk::PPB_Broker_Instance_API {
 public:
  BrokerResource(Connection connection, PP_Instance instance);
  ~BrokerResource() override;

  // Resource override.
  thunk::PPB_Broker_Instance_API* AsPPB_Broker_Instance_API() override;

  // thunk::PPB_Broker_Instance_API implementation.
  PP_Bool IsAllowed() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(BrokerResource);
};

}  // namespace proxy
}  // namespace ppapi

#endif  // PPAPI_PROXY_BROKER_RESOURCE_H_
