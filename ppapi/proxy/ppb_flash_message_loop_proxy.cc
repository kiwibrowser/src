// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/proxy/ppb_flash_message_loop_proxy.h"

#include "base/bind.h"
#include "base/macros.h"
#include "ppapi/c/pp_errors.h"
#include "ppapi/c/private/ppb_flash_message_loop.h"
#include "ppapi/proxy/enter_proxy.h"
#include "ppapi/proxy/plugin_dispatcher.h"
#include "ppapi/proxy/ppapi_messages.h"
#include "ppapi/shared_impl/resource.h"
#include "ppapi/thunk/enter.h"
#include "ppapi/thunk/ppb_flash_message_loop_api.h"
#include "ppapi/thunk/resource_creation_api.h"

using ppapi::thunk::PPB_Flash_MessageLoop_API;

namespace ppapi {
namespace proxy {
namespace {

class FlashMessageLoop : public PPB_Flash_MessageLoop_API, public Resource {
 public:
  explicit FlashMessageLoop(const HostResource& resource);
  ~FlashMessageLoop() override;

  // Resource overrides.
  PPB_Flash_MessageLoop_API* AsPPB_Flash_MessageLoop_API() override;

  // PPB_Flash_MesssageLoop_API implementation.
  int32_t Run() override;
  void Quit() override;
  void RunFromHostProxy(const RunFromHostProxyCallback& callback) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(FlashMessageLoop);
};

FlashMessageLoop::FlashMessageLoop(const HostResource& resource)
    : Resource(OBJECT_IS_PROXY, resource) {
}

FlashMessageLoop::~FlashMessageLoop() {
}

PPB_Flash_MessageLoop_API* FlashMessageLoop::AsPPB_Flash_MessageLoop_API() {
  return this;
}

int32_t FlashMessageLoop::Run() {
  int32_t result = PP_ERROR_FAILED;
  IPC::SyncMessage* msg = new PpapiHostMsg_PPBFlashMessageLoop_Run(
      API_ID_PPB_FLASH_MESSAGELOOP, host_resource(), &result);
  msg->EnableMessagePumping();
  PluginDispatcher::GetForResource(this)->Send(msg);
  return result;
}

void FlashMessageLoop::Quit() {
  PluginDispatcher::GetForResource(this)->Send(
      new PpapiHostMsg_PPBFlashMessageLoop_Quit(
          API_ID_PPB_FLASH_MESSAGELOOP, host_resource()));
}

void FlashMessageLoop::RunFromHostProxy(
    const RunFromHostProxyCallback& callback) {
  // This should never be called on the plugin side.
  NOTREACHED();
}

}  // namespace

PPB_Flash_MessageLoop_Proxy::PPB_Flash_MessageLoop_Proxy(Dispatcher* dispatcher)
    : InterfaceProxy(dispatcher) {
}

PPB_Flash_MessageLoop_Proxy::~PPB_Flash_MessageLoop_Proxy() {
}

// static
PP_Resource PPB_Flash_MessageLoop_Proxy::CreateProxyResource(
    PP_Instance instance) {
  PluginDispatcher* dispatcher = PluginDispatcher::GetForInstance(instance);
  if (!dispatcher)
    return 0;

  HostResource result;
  dispatcher->Send(new PpapiHostMsg_PPBFlashMessageLoop_Create(
      API_ID_PPB_FLASH_MESSAGELOOP, instance, &result));
  if (result.is_null())
    return 0;
  return (new FlashMessageLoop(result))->GetReference();
}

bool PPB_Flash_MessageLoop_Proxy::OnMessageReceived(const IPC::Message& msg) {
  if (!dispatcher()->permissions().HasPermission(PERMISSION_FLASH))
    return false;

  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(PPB_Flash_MessageLoop_Proxy, msg)
    IPC_MESSAGE_HANDLER(PpapiHostMsg_PPBFlashMessageLoop_Create,
                        OnMsgCreate)
    // We cannot use IPC_MESSAGE_HANDLER here. Because it tries to send the sync
    // message reply after the handler returns. However, in this case, the
    // PPB_Flash_MessageLoop_Proxy object may be destroyed before the handler
    // returns.
    IPC_MESSAGE_HANDLER_DELAY_REPLY(PpapiHostMsg_PPBFlashMessageLoop_Run,
                                    OnMsgRun)
    IPC_MESSAGE_HANDLER(PpapiHostMsg_PPBFlashMessageLoop_Quit,
                        OnMsgQuit)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

void PPB_Flash_MessageLoop_Proxy::OnMsgCreate(PP_Instance instance,
                                              HostResource* result) {
  if (!dispatcher()->permissions().HasPermission(PERMISSION_FLASH))
    return;
  thunk::EnterResourceCreation enter(instance);
  if (enter.succeeded()) {
    result->SetHostResource(
        instance, enter.functions()->CreateFlashMessageLoop(instance));
  }
}

void PPB_Flash_MessageLoop_Proxy::OnMsgRun(
    const HostResource& flash_message_loop,
    IPC::Message* reply) {
  if (!dispatcher()->permissions().HasPermission(PERMISSION_FLASH))
    return;

  PPB_Flash_MessageLoop_API::RunFromHostProxyCallback callback =
      base::Bind(&PPB_Flash_MessageLoop_Proxy::WillQuitSoon, AsWeakPtr(),
                 base::Passed(std::unique_ptr<IPC::Message>(reply)));

  EnterHostFromHostResource<PPB_Flash_MessageLoop_API>
      enter(flash_message_loop);
  if (enter.succeeded())
    enter.object()->RunFromHostProxy(callback);
  else
    callback.Run(PP_ERROR_BADRESOURCE);
}

void PPB_Flash_MessageLoop_Proxy::OnMsgQuit(
    const ppapi::HostResource& flash_message_loop) {
  EnterHostFromHostResource<PPB_Flash_MessageLoop_API>
      enter(flash_message_loop);
  if (enter.succeeded())
    enter.object()->Quit();
}

void PPB_Flash_MessageLoop_Proxy::WillQuitSoon(
    std::unique_ptr<IPC::Message> reply_message,
    int32_t result) {
  PpapiHostMsg_PPBFlashMessageLoop_Run::WriteReplyParams(reply_message.get(),
                                                         result);
  Send(reply_message.release());
}

}  // namespace proxy
}  // namespace ppapi
