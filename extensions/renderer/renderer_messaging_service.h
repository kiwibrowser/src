// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_RENDERER_RENDERER_MESSAGING_SERVICE_H_
#define EXTENSIONS_RENDERER_RENDERER_MESSAGING_SERVICE_H_

#include <string>

#include "base/macros.h"
#include "extensions/common/extension_id.h"

struct ExtensionMsg_ExternalConnectionInfo;
struct ExtensionMsg_TabConnectionInfo;

namespace content {
class RenderFrame;
}

namespace extensions {
class ExtensionBindingsSystem;
class ScriptContext;
class ScriptContextSet;
struct Message;
struct PortId;

class RendererMessagingService {
 public:
  explicit RendererMessagingService(ExtensionBindingsSystem* bindings_system);
  virtual ~RendererMessagingService();

  // Checks whether the port exists in the given frame. If it does not, a reply
  // is sent back to the browser.
  void ValidateMessagePort(const ScriptContextSet& context_set,
                           const PortId& port_id,
                           content::RenderFrame* render_frame);

  // Dispatches the onConnect content script messaging event to some contexts
  // in |context_set|. If |restrict_to_render_frame| is specified, only contexts
  // in that render frame will receive the message.
  void DispatchOnConnect(const ScriptContextSet& context_set,
                         const PortId& target_port_id,
                         const std::string& channel_name,
                         const ExtensionMsg_TabConnectionInfo& source,
                         const ExtensionMsg_ExternalConnectionInfo& info,
                         const std::string& tls_channel_id,
                         content::RenderFrame* restrict_to_render_frame);

  // Delivers a message sent using content script messaging to some of the
  // contexts in |bindings_context_set|. If |restrict_to_render_frame| is
  // specified, only contexts in that render view will receive the message.
  void DeliverMessage(const ScriptContextSet& context_set,
                      const PortId& target_port_id,
                      const Message& message,
                      content::RenderFrame* restrict_to_render_frame);

  // Dispatches the onDisconnect event in response to the channel being closed.
  void DispatchOnDisconnect(const ScriptContextSet& context_set,
                            const PortId& port_id,
                            const std::string& error_message,
                            content::RenderFrame* restrict_to_render_frame);

 private:
  // Helpers for the public methods to perform the action in a single
  // ScriptContext.
  void ValidateMessagePortInContext(const PortId& port_id,
                                    bool* has_port,
                                    ScriptContext* script_context);
  void DispatchOnConnectToScriptContext(
      const PortId& target_port_id,
      const std::string& channel_name,
      const ExtensionMsg_TabConnectionInfo* source,
      const ExtensionMsg_ExternalConnectionInfo& info,
      const std::string& tls_channel_id,
      bool* port_created,
      ScriptContext* script_context);
  void DeliverMessageToScriptContext(const Message& message,
                                     const PortId& target_port_id,
                                     ScriptContext* script_context);
  void DispatchOnDisconnectToScriptContext(const PortId& port_id,
                                           const std::string& error_message,
                                           ScriptContext* script_context);

  // Returns true if the given |script_context| has a port with the given
  // |port_id|.
  virtual bool ContextHasMessagePort(ScriptContext* script_context,
                                     const PortId& port_id) = 0;

  // Dispatches the onConnect event to listeners in the given |script_context|.
  virtual void DispatchOnConnectToListeners(
      ScriptContext* script_context,
      const PortId& target_port_id,
      const ExtensionId& target_extension_id,
      const std::string& channel_name,
      const ExtensionMsg_TabConnectionInfo* source,
      const ExtensionMsg_ExternalConnectionInfo& info,
      const std::string& tls_channel_id,
      const std::string& event_name) = 0;

  // Dispatches the onMessage event to listeners in the given |script_context|.
  // This will only be called if the context has a port with the given id.
  virtual void DispatchOnMessageToListeners(ScriptContext* script_context,
                                            const Message& message,
                                            const PortId& target_port_id) = 0;

  // Dispatches the onDisconnect event to listeners in the given
  // |script_context|. This will only be called if the context has a port
  // with the given id.
  virtual void DispatchOnDisconnectToListeners(ScriptContext* script_context,
                                               const PortId& port_id,
                                               const std::string& error) = 0;

  // The associated ExtensionBindingsSystem; guaranteed to outlive this object.
  ExtensionBindingsSystem* const bindings_system_;

  DISALLOW_COPY_AND_ASSIGN(RendererMessagingService);
};

}  // namespace extensions

#endif  // EXTENSIONS_RENDERER_RENDERER_MESSAGING_SERVICE_H_
