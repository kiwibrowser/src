// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/proxy/broker_dispatcher.h"

#include "base/sync_socket.h"
#include "ppapi/c/pp_errors.h"
#include "ppapi/proxy/ppapi_messages.h"
#include "ppapi/shared_impl/platform_file.h"

namespace ppapi {
namespace proxy {

BrokerDispatcher::BrokerDispatcher(PP_ConnectInstance_Func connect_instance)
    : connect_instance_(connect_instance) {
}

BrokerDispatcher::~BrokerDispatcher() {
}

bool BrokerDispatcher::InitBrokerWithChannel(
    ProxyChannel::Delegate* delegate,
    base::ProcessId peer_pid,
    const IPC::ChannelHandle& channel_handle,
    bool is_client) {
  return ProxyChannel::InitWithChannel(delegate, peer_pid, channel_handle,
                                       is_client);
}

bool BrokerDispatcher::OnMessageReceived(const IPC::Message& msg) {
  // Control messages.
  if (msg.routing_id() == MSG_ROUTING_CONTROL) {
    bool handled = true;
    IPC_BEGIN_MESSAGE_MAP(BrokerDispatcher, msg)
      IPC_MESSAGE_HANDLER(PpapiMsg_ConnectToPlugin, OnMsgConnectToPlugin)
      IPC_MESSAGE_UNHANDLED(handled = false)
    IPC_END_MESSAGE_MAP()
    return handled;
  }
  return false;
}

// Transfers ownership of the handle to the broker module.
void BrokerDispatcher::OnMsgConnectToPlugin(
    PP_Instance instance,
    IPC::PlatformFileForTransit handle,
    int32_t* result) {
  if (handle == IPC::InvalidPlatformFileForTransit()) {
    *result = PP_ERROR_FAILED;
  } else {
    base::SyncSocket::Handle socket_handle =
        IPC::PlatformFileForTransitToPlatformFile(handle);

    if (connect_instance_) {
      *result = connect_instance_(instance,
                                  ppapi::PlatformFileToInt(socket_handle));
    } else {
      *result = PP_ERROR_FAILED;
      // Close the handle since there is no other owner.
      // The easiest way to clean it up is to just put it in an object
      // and then close them. This failure case is not performance critical.
      base::SyncSocket temp_socket(socket_handle);
    }
  }
}

BrokerHostDispatcher::BrokerHostDispatcher()
    : BrokerDispatcher(NULL) {
}

void BrokerHostDispatcher::OnChannelError() {
  DVLOG(1) << "BrokerHostDispatcher::OnChannelError()";
  BrokerDispatcher::OnChannelError();  // Stop using the channel.

  // Tell the host about the crash so it can clean up and display notification.
  // TODO(ddorwin): Add BrokerCrashed() to PPB_Proxy_Private and call it.
  // ppb_proxy_->BrokerCrashed(pp_module());
}

BrokerSideDispatcher::BrokerSideDispatcher(
    PP_ConnectInstance_Func connect_instance)
    : BrokerDispatcher(connect_instance) {
}

void BrokerSideDispatcher::OnChannelError() {
  DVLOG(1) << "BrokerSideDispatcher::OnChannelError()";
  BrokerDispatcher::OnChannelError();

  // The renderer has crashed or exited. This channel and all instances
  // associated with it are no longer valid.
  // TODO(ddorwin): This causes the broker process to exit, which may not be
  // desirable in some use cases.
  delete this;
}


}  // namespace proxy
}  // namespace ppapi
