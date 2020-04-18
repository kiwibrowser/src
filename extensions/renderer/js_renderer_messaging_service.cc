// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/renderer/js_renderer_messaging_service.h"

#include <stdint.h>

#include <string>

#include "content/public/common/child_process_host.h"
#include "content/public/renderer/v8_value_converter.h"
#include "extensions/common/api/messaging/message.h"
#include "extensions/common/api/messaging/port_id.h"
#include "extensions/common/extension_messages.h"
#include "extensions/common/manifest_handlers/externally_connectable.h"
#include "extensions/renderer/extension_port.h"
#include "extensions/renderer/messaging_bindings.h"
#include "extensions/renderer/script_context.h"
#include "extensions/renderer/v8_helpers.h"
#include "v8/include/v8.h"

namespace extensions {

using v8_helpers::ToV8String;

JSRendererMessagingService::JSRendererMessagingService(
    ExtensionBindingsSystem* bindings_system)
    : RendererMessagingService(bindings_system) {}
JSRendererMessagingService::~JSRendererMessagingService() {}

bool JSRendererMessagingService::ContextHasMessagePort(
    ScriptContext* script_context,
    const PortId& port_id) {
  MessagingBindings* bindings = MessagingBindings::ForContext(script_context);
  DCHECK(bindings);
  return bindings->GetPortWithId(port_id) != nullptr;
}

void JSRendererMessagingService::DispatchOnConnectToListeners(
    ScriptContext* script_context,
    const PortId& target_port_id,
    const ExtensionId& target_extension_id,
    const std::string& channel_name,
    const ExtensionMsg_TabConnectionInfo* source,
    const ExtensionMsg_ExternalConnectionInfo& info,
    const std::string& tls_channel_id,
    const std::string& event_name) {
  MessagingBindings* bindings = MessagingBindings::ForContext(script_context);
  ExtensionPort* port = bindings->CreateNewPortWithId(target_port_id);

  v8::Isolate* isolate = script_context->isolate();
  v8::HandleScope handle_scope(isolate);

  const std::string& source_url_spec = info.source_url.spec();
  const Extension* extension = script_context->extension();

  v8::Local<v8::Value> tab = v8::Null(isolate);
  v8::Local<v8::Value> tls_channel_id_value = v8::Undefined(isolate);
  v8::Local<v8::Value> guest_process_id = v8::Undefined(isolate);
  v8::Local<v8::Value> guest_render_frame_routing_id = v8::Undefined(isolate);

  if (extension) {
    if (!source->tab.empty() && !extension->is_platform_app()) {
      tab = content::V8ValueConverter::Create()->ToV8Value(
          &source->tab, script_context->v8_context());
    }

    ExternallyConnectableInfo* externally_connectable =
        ExternallyConnectableInfo::Get(extension);
    if (externally_connectable &&
        externally_connectable->accepts_tls_channel_id) {
      v8::Local<v8::String> v8_tls_channel_id;
      if (ToV8String(isolate, tls_channel_id.c_str(), &v8_tls_channel_id))
        tls_channel_id_value = v8_tls_channel_id;
    }

    if (info.guest_process_id != content::ChildProcessHost::kInvalidUniqueID) {
      guest_process_id = v8::Integer::New(isolate, info.guest_process_id);
      guest_render_frame_routing_id =
          v8::Integer::New(isolate, info.guest_render_frame_routing_id);
    }
  }

  v8::Local<v8::String> v8_channel_name;
  v8::Local<v8::String> v8_source_id;
  v8::Local<v8::String> v8_target_extension_id;
  v8::Local<v8::String> v8_source_url_spec;
  if (!ToV8String(isolate, channel_name.c_str(), &v8_channel_name) ||
      !ToV8String(isolate, info.source_id.c_str(), &v8_source_id) ||
      !ToV8String(isolate, target_extension_id.c_str(),
                  &v8_target_extension_id) ||
      !ToV8String(isolate, source_url_spec.c_str(), &v8_source_url_spec)) {
    NOTREACHED() << "dispatchOnConnect() passed non-string argument";
    return;
  }

  v8::Local<v8::Value> arguments[] = {
      // portId
      v8::Integer::New(isolate, port->js_id()),
      // channelName
      v8_channel_name,
      // sourceTab
      tab,
      // source_frame_id
      v8::Integer::New(isolate, source->frame_id),
      // guestProcessId
      guest_process_id,
      // guestRenderFrameRoutingId
      guest_render_frame_routing_id,
      // sourceExtensionId
      v8_source_id,
      // targetExtensionId
      v8_target_extension_id,
      // sourceUrl
      v8_source_url_spec,
      // tlsChannelId
      tls_channel_id_value,
  };

  // Note: this can execute asynchronously if JS is suspended.
  script_context->module_system()->CallModuleMethodSafe(
      "messaging", "dispatchOnConnect", arraysize(arguments), arguments);
}

void JSRendererMessagingService::DispatchOnMessageToListeners(
    ScriptContext* script_context,
    const Message& message,
    const PortId& target_port_id) {
  MessagingBindings* bindings = MessagingBindings::ForContext(script_context);
  ExtensionPort* port = bindings->GetPortWithId(target_port_id);
  DCHECK(port);

  v8::Isolate* isolate = script_context->isolate();
  v8::HandleScope handle_scope(isolate);

  v8::Local<v8::Value> port_id_handle =
      v8::Integer::New(isolate, port->js_id());

  v8::Local<v8::String> v8_data;
  if (!ToV8String(isolate, message.data.c_str(), &v8_data))
    return;
  std::vector<v8::Local<v8::Value>> arguments;
  arguments.push_back(v8_data);
  arguments.push_back(port_id_handle);

  script_context->module_system()->CallModuleMethodSafe(
      "messaging", "dispatchOnMessage", &arguments);
}

void JSRendererMessagingService::DispatchOnDisconnectToListeners(
    ScriptContext* script_context,
    const PortId& port_id,
    const std::string& error_message) {
  MessagingBindings* bindings = MessagingBindings::ForContext(script_context);
  ExtensionPort* port = bindings->GetPortWithId(port_id);
  DCHECK(port);

  v8::Isolate* isolate = script_context->isolate();
  v8::HandleScope handle_scope(isolate);

  std::vector<v8::Local<v8::Value>> arguments;
  arguments.push_back(v8::Integer::New(isolate, port->js_id()));
  v8::Local<v8::String> v8_error_message;
  if (!error_message.empty())
    ToV8String(isolate, error_message.c_str(), &v8_error_message);
  if (!v8_error_message.IsEmpty()) {
    arguments.push_back(v8_error_message);
  } else {
    arguments.push_back(v8::Null(isolate));
  }

  script_context->module_system()->CallModuleMethodSafe(
      "messaging", "dispatchOnDisconnect", &arguments);
}

}  // namespace extensions
