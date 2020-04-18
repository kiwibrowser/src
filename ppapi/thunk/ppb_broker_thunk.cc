// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdint.h>

#include "ppapi/c/pp_errors.h"
#include "ppapi/c/trusted/ppb_broker_trusted.h"
#include "ppapi/shared_impl/tracked_callback.h"
#include "ppapi/thunk/enter.h"
#include "ppapi/thunk/ppb_broker_api.h"
#include "ppapi/thunk/resource_creation_api.h"
#include "ppapi/thunk/thunk.h"

namespace ppapi {
namespace thunk {

namespace {

PP_Resource CreateTrusted(PP_Instance instance) {
  EnterResourceCreation enter(instance);
  if (enter.failed())
    return 0;
  return enter.functions()->CreateBroker(instance);
}

PP_Bool IsBrokerTrusted(PP_Resource resource) {
  EnterResource<PPB_Broker_API> enter(resource, false);
  return PP_FromBool(enter.succeeded());
}

int32_t Connect(PP_Resource resource,
                PP_CompletionCallback callback) {
  EnterResource<PPB_Broker_API> enter(resource, callback, true);
  if (enter.failed())
    return enter.retval();
  return enter.SetResult(enter.object()->Connect(enter.callback()));
}

int32_t GetHandle(PP_Resource resource, int32_t* handle) {
  EnterResource<PPB_Broker_API> enter(resource, true);
  if (enter.failed())
    return enter.retval();
  return enter.object()->GetHandle(handle);
}

PP_Bool IsAllowed(PP_Resource resource) {
  // TODO(raymes): This is a hack. See the note in ppb_broker_api.h.
  PP_Instance instance = 0;
  {
    EnterResource<PPB_Broker_API> enter_resource(resource, true);
    if (enter_resource.failed())
      return PP_FALSE;
    instance = enter_resource.resource()->pp_instance();
  }
  EnterInstanceAPI<PPB_Broker_Instance_API> enter_instance(instance);
  if (enter_instance.failed())
    return PP_FALSE;
  return enter_instance.functions()->IsAllowed();
}

const PPB_BrokerTrusted_0_2 g_ppb_broker_0_2_thunk = {
  &CreateTrusted,
  &IsBrokerTrusted,
  &Connect,
  &GetHandle,
};

const PPB_BrokerTrusted_0_3 g_ppb_broker_0_3_thunk = {
  &CreateTrusted,
  &IsBrokerTrusted,
  &Connect,
  &GetHandle,
  &IsAllowed,
};

}  // namespace

const PPB_BrokerTrusted_0_2* GetPPB_BrokerTrusted_0_2_Thunk() {
  return &g_ppb_broker_0_2_thunk;
}

const PPB_BrokerTrusted_0_3* GetPPB_BrokerTrusted_0_3_Thunk() {
  return &g_ppb_broker_0_3_thunk;
}

}  // namespace thunk
}  // namespace ppapi
