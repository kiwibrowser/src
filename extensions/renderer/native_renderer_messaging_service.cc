// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/renderer/native_renderer_messaging_service.h"

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include "base/supports_user_data.h"
#include "content/public/common/child_process_host.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/v8_value_converter.h"
#include "extensions/common/api/messaging/message.h"
#include "extensions/common/api/messaging/port_id.h"
#include "extensions/common/extension_messages.h"
#include "extensions/common/manifest_handlers/externally_connectable.h"
#include "extensions/renderer/api_activity_logger.h"
#include "extensions/renderer/bindings/api_binding_util.h"
#include "extensions/renderer/bindings/get_per_context_data.h"
#include "extensions/renderer/ipc_message_sender.h"
#include "extensions/renderer/message_target.h"
#include "extensions/renderer/native_extension_bindings_system.h"
#include "extensions/renderer/script_context.h"
#include "extensions/renderer/script_context_set.h"
#include "gin/data_object_builder.h"
#include "gin/handle.h"
#include "gin/per_context_data.h"
#include "v8/include/v8.h"

namespace extensions {

namespace {

struct MessagingPerContextData : public base::SupportsUserData::Data {
  static constexpr char kPerContextDataKey[] =
      "extension_messaging_per_context_data";

  // All the port objects that exist in this context.
  std::map<PortId, v8::Global<v8::Object>> ports;

  // The next available context-specific port id.
  int next_port_id = 0;
};

constexpr char MessagingPerContextData::kPerContextDataKey[];

bool ScriptContextIsValid(ScriptContext* script_context) {
  // TODO(devlin): This is in lieu of a similar check in the JS bindings that
  // null-checks ScriptContext::GetRenderFrame(). This is good because it
  // removes the reliance on RenderFrames (which make testing very difficult),
  // but is it fully analogous? It should be close enough, since the browser
  // has to deal with frames potentially disappearing before the IPC arrives
  // anyway.
  return script_context->is_valid();
}

}  // namespace

NativeRendererMessagingService::NativeRendererMessagingService(
    NativeExtensionBindingsSystem* bindings_system)
    : RendererMessagingService(bindings_system),
      bindings_system_(bindings_system),
      one_time_message_handler_(bindings_system) {}
NativeRendererMessagingService::~NativeRendererMessagingService() {}

gin::Handle<GinPort> NativeRendererMessagingService::Connect(
    ScriptContext* script_context,
    const MessageTarget& target,
    const std::string& channel_name,
    bool include_tls_channel_id) {
  if (!ScriptContextIsValid(script_context))
    return gin::Handle<GinPort>();

  MessagingPerContextData* data = GetPerContextData<MessagingPerContextData>(
      script_context->v8_context(), kCreateIfMissing);
  if (!data)
    return gin::Handle<GinPort>();

  bool is_opener = true;
  gin::Handle<GinPort> port = CreatePort(
      script_context, channel_name,
      PortId(script_context->context_id(), data->next_port_id++, is_opener));

  bindings_system_->GetIPCMessageSender()->SendOpenMessageChannel(
      script_context, port->port_id(), target, channel_name,
      include_tls_channel_id);
  return port;
}

void NativeRendererMessagingService::SendOneTimeMessage(
    ScriptContext* script_context,
    const MessageTarget& target,
    const std::string& method_name,
    bool include_tls_channel_id,
    const Message& message,
    v8::Local<v8::Function> response_callback) {
  if (!ScriptContextIsValid(script_context))
    return;

  MessagingPerContextData* data = GetPerContextData<MessagingPerContextData>(
      script_context->v8_context(), kCreateIfMissing);

  bool is_opener = true;
  PortId port_id(script_context->context_id(), data->next_port_id++, is_opener);

  one_time_message_handler_.SendMessage(script_context, port_id, target,
                                        method_name, include_tls_channel_id,
                                        message, response_callback);
}

void NativeRendererMessagingService::PostMessageToPort(
    v8::Local<v8::Context> context,
    const PortId& port_id,
    int routing_id,
    std::unique_ptr<Message> message) {
  ScriptContext* script_context =
      ScriptContextSet::GetContextByV8Context(context);
  CHECK(script_context);
  if (!ScriptContextIsValid(script_context))
    return;

  bindings_system_->GetIPCMessageSender()->SendPostMessageToPort(
      routing_id, port_id, *message);
}

void NativeRendererMessagingService::ClosePort(v8::Local<v8::Context> context,
                                               const PortId& port_id,
                                               int routing_id) {
  ScriptContext* script_context =
      ScriptContextSet::GetContextByV8Context(context);
  CHECK(script_context);

  MessagingPerContextData* data = GetPerContextData<MessagingPerContextData>(
      script_context->v8_context(), kDontCreateIfMissing);
  if (!data)
    return;

  size_t erased = data->ports.erase(port_id);
  DCHECK_GT(erased, 0u);

  if (!ScriptContextIsValid(script_context))
    return;

  bool close_channel = true;
  bindings_system_->GetIPCMessageSender()->SendCloseMessagePort(
      routing_id, port_id, close_channel);
}

gin::Handle<GinPort> NativeRendererMessagingService::CreatePortForTesting(
    ScriptContext* script_context,
    const std::string& channel_name,
    const PortId& port_id) {
  return CreatePort(script_context, channel_name, port_id);
}

gin::Handle<GinPort> NativeRendererMessagingService::GetPortForTesting(
    ScriptContext* script_context,
    const PortId& port_id) {
  return GetPort(script_context, port_id);
}

bool NativeRendererMessagingService::HasPortForTesting(
    ScriptContext* script_context,
    const PortId& port_id) {
  return ContextHasMessagePort(script_context, port_id);
}

bool NativeRendererMessagingService::ContextHasMessagePort(
    ScriptContext* script_context,
    const PortId& port_id) {
  if (one_time_message_handler_.HasPort(script_context, port_id))
    return true;
  v8::HandleScope handle_scope(script_context->isolate());
  MessagingPerContextData* data = GetPerContextData<MessagingPerContextData>(
      script_context->v8_context(), kDontCreateIfMissing);
  return data && base::ContainsKey(data->ports, port_id);
}

void NativeRendererMessagingService::DispatchOnConnectToListeners(
    ScriptContext* script_context,
    const PortId& target_port_id,
    const ExtensionId& target_extension_id,
    const std::string& channel_name,
    const ExtensionMsg_TabConnectionInfo* source,
    const ExtensionMsg_ExternalConnectionInfo& info,
    const std::string& tls_channel_id,
    const std::string& event_name) {
  v8::Isolate* isolate = script_context->isolate();
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> v8_context = script_context->v8_context();
  v8::Context::Scope context_scope(v8_context);

  gin::DataObjectBuilder sender_builder(isolate);
  if (!info.source_id.empty())
    sender_builder.Set("id", info.source_id);
  if (!info.source_url.is_empty())
    sender_builder.Set("url", info.source_url.spec());
  if (info.source_origin)
    sender_builder.Set("origin", info.source_origin->Serialize());
  if (source->frame_id >= 0)
    sender_builder.Set("frameId", source->frame_id);

  const Extension* extension = script_context->extension();
  if (extension) {
    if (!source->tab.empty() && !extension->is_platform_app()) {
      sender_builder.Set("tab", content::V8ValueConverter::Create()->ToV8Value(
                                    &source->tab, v8_context));
    }

    ExternallyConnectableInfo* externally_connectable =
        ExternallyConnectableInfo::Get(extension);
    if (externally_connectable &&
        externally_connectable->accepts_tls_channel_id) {
      sender_builder.Set("tlsChannelId", tls_channel_id);
    }

    if (info.guest_process_id != content::ChildProcessHost::kInvalidUniqueID) {
      CHECK(Manifest::IsComponentLocation(extension->location()))
          << "GuestProcessId can only be exposed to component extensions.";
      sender_builder.Set("guestProcessId", info.guest_process_id)
          .Set("guestRenderFrameRoutingId", info.guest_render_frame_routing_id);
    }
  }

  v8::Local<v8::Object> sender = sender_builder.Build();

  if (channel_name == "chrome.extension.sendRequest" ||
      channel_name == "chrome.runtime.sendMessage") {
    one_time_message_handler_.AddReceiver(script_context, target_port_id,
                                          sender, event_name);
  } else {
    gin::Handle<GinPort> port =
        CreatePort(script_context, channel_name, target_port_id);
    port->SetSender(v8_context, sender);
    std::vector<v8::Local<v8::Value>> args = {port.ToV8()};
    bindings_system_->api_system()->event_handler()->FireEventInContext(
        event_name, v8_context, &args, nullptr, JSRunner::ResultCallback());
  }
  // Note: Arbitrary JS may have run; the context may now be deleted.

  if (binding::IsContextValid(v8_context) &&
      APIActivityLogger::IsLoggingEnabled()) {
    auto activity_logging_args =
        std::make_unique<base::Value>(base::Value::Type::LIST);
    auto& list = activity_logging_args->GetList();
    list.reserve(2u);
    if (!info.source_id.empty())
      list.emplace_back(info.source_id);
    else
      list.emplace_back();

    if (!info.source_url.is_empty())
      list.emplace_back(info.source_url.spec());
    else
      list.emplace_back();

    APIActivityLogger::LogEvent(
        script_context, event_name,
        base::ListValue::From(std::move(activity_logging_args)));
  }
}

void NativeRendererMessagingService::DispatchOnMessageToListeners(
    ScriptContext* script_context,
    const Message& message,
    const PortId& target_port_id) {
  v8::Isolate* isolate = script_context->isolate();
  v8::HandleScope handle_scope(isolate);
  v8::Context::Scope context_scope(script_context->v8_context());

  if (one_time_message_handler_.DeliverMessage(script_context, message,
                                               target_port_id)) {
    return;
  }

  gin::Handle<GinPort> port = GetPort(script_context, target_port_id);
  DCHECK(!port.IsEmpty());

  port->DispatchOnMessage(script_context->v8_context(), message);
  // Note: Arbitrary JS may have run; the context may now be deleted.
}

void NativeRendererMessagingService::DispatchOnDisconnectToListeners(
    ScriptContext* script_context,
    const PortId& port_id,
    const std::string& error_message) {
  v8::Isolate* isolate = script_context->isolate();
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> v8_context = script_context->v8_context();
  v8::Context::Scope context_scope(v8_context);

  if (one_time_message_handler_.Disconnect(script_context, port_id,
                                           error_message)) {
    return;
  }

  gin::Handle<GinPort> port = GetPort(script_context, port_id);
  DCHECK(!port.IsEmpty());
  if (!error_message.empty()) {
    // TODO(devlin): Subtle: If the JS event to disconnect the port happens
    // asynchronously because JS is suspended, this last error won't be
    // correctly set for listeners. Given this exceedingly rare, and shouldn't
    // behave too strangely, this is somewhat low priority.
    bindings_system_->api_system()->request_handler()->last_error()->SetError(
        v8_context, error_message);
  }

  port->DispatchOnDisconnect(v8_context);
  // Note: Arbitrary JS may have run; the context may now be deleted.

  if (!binding::IsContextValid(v8_context))
    return;

  if (!error_message.empty()) {
    bindings_system_->api_system()->request_handler()->last_error()->ClearError(
        v8_context, true);
  }

  MessagingPerContextData* data = GetPerContextData<MessagingPerContextData>(
      v8_context, kDontCreateIfMissing);
  DCHECK(data);
  data->ports.erase(port_id);
}

gin::Handle<GinPort> NativeRendererMessagingService::CreatePort(
    ScriptContext* script_context,
    const std::string& channel_name,
    const PortId& port_id) {
  // Note: no HandleScope because it would invalidate the gin::Handle::wrapper_.
  v8::Isolate* isolate = script_context->isolate();
  v8::Local<v8::Context> context = script_context->v8_context();
  // Note: needed because gin::CreateHandle infers the context from the active
  // context on the isolate.
  v8::Context::Scope context_scope(context);

  // If this port is an opener, then it should have been created in this
  // context. Otherwise, it should have been created in another context, because
  // we don't support intra-context message passing.
  if (port_id.is_opener)
    DCHECK_EQ(port_id.context_id, script_context->context_id());
  else
    DCHECK_NE(port_id.context_id, script_context->context_id());

  content::RenderFrame* render_frame = script_context->GetRenderFrame();
  int routing_id =
      render_frame ? render_frame->GetRoutingID() : MSG_ROUTING_NONE;

  MessagingPerContextData* data =
      GetPerContextData<MessagingPerContextData>(context, kCreateIfMissing);
  DCHECK(data);
  DCHECK(!base::ContainsKey(data->ports, port_id));

  gin::Handle<GinPort> port_handle = gin::CreateHandle(
      isolate,
      new GinPort(context, port_id, routing_id, channel_name,
                  bindings_system_->api_system()->event_handler(), this));

  v8::Local<v8::Object> port_object = port_handle.ToV8().As<v8::Object>();
  data->ports[port_id].Reset(isolate, port_object);

  return port_handle;
}

gin::Handle<GinPort> NativeRendererMessagingService::GetPort(
    ScriptContext* script_context,
    const PortId& port_id) {
  // Note: no HandleScope because it would invalidate the gin::Handle::wrapper_.
  v8::Isolate* isolate = script_context->isolate();
  v8::Local<v8::Context> context = script_context->v8_context();

  MessagingPerContextData* data = GetPerContextData<MessagingPerContextData>(
      script_context->v8_context(), kDontCreateIfMissing);
  DCHECK(data);
  DCHECK(base::ContainsKey(data->ports, port_id));

  GinPort* port = nullptr;
  gin::Converter<GinPort*>::FromV8(context->GetIsolate(),
                                   data->ports[port_id].Get(isolate), &port);
  CHECK(port);

  return gin::CreateHandle(isolate, port);
}

}  // namespace extensions
