// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/proxy/ppb_broker_proxy.h"

#include "base/bind.h"
#include "base/macros.h"
#include "ppapi/c/pp_errors.h"
#include "ppapi/c/trusted/ppb_broker_trusted.h"
#include "ppapi/proxy/enter_proxy.h"
#include "ppapi/proxy/plugin_dispatcher.h"
#include "ppapi/proxy/ppapi_messages.h"
#include "ppapi/shared_impl/platform_file.h"
#include "ppapi/shared_impl/tracked_callback.h"
#include "ppapi/thunk/enter.h"
#include "ppapi/thunk/ppb_broker_api.h"
#include "ppapi/thunk/resource_creation_api.h"
#include "ppapi/thunk/thunk.h"

using ppapi::IntToPlatformFile;
using ppapi::PlatformFileToInt;
using ppapi::thunk::PPB_Broker_API;

namespace ppapi {
namespace proxy {

class Broker : public PPB_Broker_API, public Resource {
 public:
  explicit Broker(const HostResource& resource);
  ~Broker() override;

  // Resource overrides.
  PPB_Broker_API* AsPPB_Broker_API() override;

  // PPB_Broker_API implementation.
  int32_t Connect(scoped_refptr<TrackedCallback> connect_callback) override;
  int32_t GetHandle(int32_t* handle) override;

  // Called by the proxy when the host side has completed the request.
  void ConnectComplete(IPC::PlatformFileForTransit socket_handle,
                       int32_t result);

 private:
  bool called_connect_;
  scoped_refptr<TrackedCallback> current_connect_callback_;

  // The plugin module owns the handle.
  // The host side transfers ownership of the handle to the plugin side when it
  // sends the IPC. This member holds the handle value for the plugin module
  // to read, but the plugin side of the proxy never takes ownership.
  base::SyncSocket::Handle socket_handle_;

  DISALLOW_COPY_AND_ASSIGN(Broker);
};

Broker::Broker(const HostResource& resource)
    : Resource(OBJECT_IS_PROXY, resource),
      called_connect_(false),
      socket_handle_(base::SyncSocket::kInvalidHandle) {
}

Broker::~Broker() {
  socket_handle_ = base::SyncSocket::kInvalidHandle;
}

PPB_Broker_API* Broker::AsPPB_Broker_API() {
  return this;
}

int32_t Broker::Connect(scoped_refptr<TrackedCallback> connect_callback) {
  if (TrackedCallback::IsPending(current_connect_callback_))
    return PP_ERROR_INPROGRESS;
  else if (called_connect_)
    return PP_ERROR_FAILED;

  current_connect_callback_ = connect_callback;
  called_connect_ = true;

  bool success = PluginDispatcher::GetForResource(this)->Send(
      new PpapiHostMsg_PPBBroker_Connect(
          API_ID_PPB_BROKER, host_resource()));
  return success ?  PP_OK_COMPLETIONPENDING : PP_ERROR_FAILED;
}

int32_t Broker::GetHandle(int32_t* handle) {
  if (socket_handle_ == base::SyncSocket::kInvalidHandle)
    return PP_ERROR_FAILED;
  *handle = PlatformFileToInt(socket_handle_);
  return PP_OK;
}

void Broker::ConnectComplete(IPC::PlatformFileForTransit socket_handle,
                             int32_t result) {
  if (result == PP_OK) {
    DCHECK(socket_handle_ == base::SyncSocket::kInvalidHandle);
    socket_handle_ = IPC::PlatformFileForTransitToPlatformFile(socket_handle);
  } else {
    // The caller may still have given us a handle in the failure case.
    // The easiest way to clean it up is to just put it in an object
    // and then close them. This failure case is not performance critical.
    base::SyncSocket temp_socket(
        IPC::PlatformFileForTransitToPlatformFile(socket_handle));
  }

  if (!TrackedCallback::IsPending(current_connect_callback_)) {
    // The handle might leak if the plugin never calls GetHandle().
    return;
  }

  current_connect_callback_->Run(result);
}

PPB_Broker_Proxy::PPB_Broker_Proxy(Dispatcher* dispatcher)
    : InterfaceProxy(dispatcher),
      callback_factory_(this){
}

PPB_Broker_Proxy::~PPB_Broker_Proxy() {
}

// static
PP_Resource PPB_Broker_Proxy::CreateProxyResource(PP_Instance instance) {
  PluginDispatcher* dispatcher = PluginDispatcher::GetForInstance(instance);
  if (!dispatcher)
    return 0;

  HostResource result;
  dispatcher->Send(new PpapiHostMsg_PPBBroker_Create(
      API_ID_PPB_BROKER, instance, &result));
  if (result.is_null())
    return 0;
  return (new Broker(result))->GetReference();
}

bool PPB_Broker_Proxy::OnMessageReceived(const IPC::Message& msg) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(PPB_Broker_Proxy, msg)
    IPC_MESSAGE_HANDLER(PpapiHostMsg_PPBBroker_Create, OnMsgCreate)
    IPC_MESSAGE_HANDLER(PpapiHostMsg_PPBBroker_Connect, OnMsgConnect)
    IPC_MESSAGE_HANDLER(PpapiMsg_PPBBroker_ConnectComplete,
                        OnMsgConnectComplete)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

void PPB_Broker_Proxy::OnMsgCreate(PP_Instance instance,
                                   HostResource* result_resource) {
  if (!dispatcher()->permissions().HasPermission(PERMISSION_PRIVATE))
    return;
  thunk::EnterResourceCreation enter(instance);
  if (enter.succeeded()) {
    result_resource->SetHostResource(
        instance,
        enter.functions()->CreateBroker(instance));
  }
}

void PPB_Broker_Proxy::OnMsgConnect(const HostResource& broker) {
  if (!dispatcher()->permissions().HasPermission(PERMISSION_PRIVATE))
    return;
  EnterHostFromHostResourceForceCallback<PPB_Broker_API> enter(
      broker, callback_factory_,
      &PPB_Broker_Proxy::ConnectCompleteInHost, broker);
  if (enter.succeeded())
    enter.SetResult(enter.object()->Connect(enter.callback()));
}

// Called in the plugin to handle the connect callback.
// The proxy owns the handle and transfers it to the Broker. At that point,
// the plugin owns the handle and is responsible for closing it.
// The caller guarantees that socket_handle is not valid if result is not PP_OK.
void PPB_Broker_Proxy::OnMsgConnectComplete(
    const HostResource& resource,
    IPC::PlatformFileForTransit socket_handle,
    int32_t result) {
  DCHECK(result == PP_OK ||
         socket_handle == IPC::InvalidPlatformFileForTransit());

  EnterPluginFromHostResource<PPB_Broker_API> enter(resource);
  if (enter.failed()) {
    // As in Broker::ConnectComplete, we need to close the resource on error.
    base::SyncSocket temp_socket(
        IPC::PlatformFileForTransitToPlatformFile(socket_handle));
  } else {
    static_cast<Broker*>(enter.object())->ConnectComplete(socket_handle,
                                                          result);
  }
}

// Callback on the host side.
// Transfers ownership of the handle to the plugin side. This function must
// either successfully call the callback or close the handle.
void PPB_Broker_Proxy::ConnectCompleteInHost(int32_t result,
                                             const HostResource& broker) {
  IPC::PlatformFileForTransit foreign_socket_handle =
      IPC::InvalidPlatformFileForTransit();
  if (result == PP_OK) {
    int32_t socket_handle = PlatformFileToInt(base::SyncSocket::kInvalidHandle);
    EnterHostFromHostResource<PPB_Broker_API> enter(broker);
    if (enter.succeeded())
      result = enter.object()->GetHandle(&socket_handle);
    DCHECK(result == PP_OK ||
           socket_handle ==
               PlatformFileToInt(base::SyncSocket::kInvalidHandle));

    if (result == PP_OK) {
      foreign_socket_handle =
          dispatcher()->ShareHandleWithRemote(IntToPlatformFile(socket_handle),
                                              true);
      if (foreign_socket_handle == IPC::InvalidPlatformFileForTransit()) {
        result = PP_ERROR_FAILED;
        // Assume the local handle was closed even if the foreign handle could
        // not be created.
      }
    }
  }
  DCHECK(result == PP_OK ||
         foreign_socket_handle == IPC::InvalidPlatformFileForTransit());

  bool success = dispatcher()->Send(new PpapiMsg_PPBBroker_ConnectComplete(
      API_ID_PPB_BROKER, broker, foreign_socket_handle, result));

  if (!success || result != PP_OK) {
      // The plugin did not receive the handle, so it must be closed.
      // The easiest way to clean it up is to just put it in an object
      // and then close it. This failure case is not performance critical.
      // The handle could still leak if Send succeeded but the IPC later failed.
      base::SyncSocket temp_socket(
          IPC::PlatformFileForTransitToPlatformFile(foreign_socket_handle));
  }
}

}  // namespace proxy
}  // namespace ppapi
