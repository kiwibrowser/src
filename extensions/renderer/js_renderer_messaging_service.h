// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_RENDERER_JS_RENDERER_MESSAGING_SERVICE_H_
#define EXTENSIONS_RENDERER_JS_RENDERER_MESSAGING_SERVICE_H_

#include <string>

#include "base/macros.h"
#include "extensions/renderer/renderer_messaging_service.h"

struct ExtensionMsg_ExternalConnectionInfo;
struct ExtensionMsg_TabConnectionInfo;

namespace extensions {
struct Message;
struct PortId;

// The messaging service to handle dispatching extension messages and connection
// events to different contexts.
class JSRendererMessagingService : public RendererMessagingService {
 public:
  explicit JSRendererMessagingService(ExtensionBindingsSystem* bindings_system);
  ~JSRendererMessagingService() override;

 private:
  // RendererMessagingService:
  bool ContextHasMessagePort(ScriptContext* script_context,
                             const PortId& port_id) override;
  void DispatchOnConnectToListeners(
      ScriptContext* script_context,
      const PortId& target_port_id,
      const ExtensionId& target_extension_id,
      const std::string& channel_name,
      const ExtensionMsg_TabConnectionInfo* source,
      const ExtensionMsg_ExternalConnectionInfo& info,
      const std::string& tls_channel_id,
      const std::string& event_name) override;
  void DispatchOnMessageToListeners(ScriptContext* script_context,
                                    const Message& message,
                                    const PortId& target_port_id) override;
  void DispatchOnDisconnectToListeners(ScriptContext* script_context,
                                       const PortId& port_id,
                                       const std::string& error) override;

  DISALLOW_COPY_AND_ASSIGN(JSRendererMessagingService);
};

}  // namespace extensions

#endif  // EXTENSIONS_RENDERER_JS_RENDERER_MESSAGING_SERVICE_H_
