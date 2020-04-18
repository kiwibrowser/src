// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "headless/public/internal/headless_devtools_client_impl.h"

#include <memory>

#include "base/bind.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/memory/ptr_util.h"
#include "base/values.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/devtools_agent_host.h"

namespace headless {

// static
std::unique_ptr<HeadlessDevToolsClient> HeadlessDevToolsClient::Create() {
  return std::make_unique<HeadlessDevToolsClientImpl>();
}

// static
std::unique_ptr<HeadlessDevToolsClient>
HeadlessDevToolsClient::CreateWithExternalHost(ExternalHost* external_host) {
  auto result = std::make_unique<HeadlessDevToolsClientImpl>();
  result->AttachToExternalHost(external_host);
  return result;
}

// static
HeadlessDevToolsClientImpl* HeadlessDevToolsClientImpl::From(
    HeadlessDevToolsClient* client) {
  // This downcast is safe because there is only one implementation of
  // HeadlessDevToolsClient.
  return static_cast<HeadlessDevToolsClientImpl*>(client);
}

HeadlessDevToolsClientImpl::HeadlessDevToolsClientImpl()
    : accessibility_domain_(this),
      animation_domain_(this),
      application_cache_domain_(this),
      browser_domain_(this),
      cache_storage_domain_(this),
      console_domain_(this),
      css_domain_(this),
      database_domain_(this),
      debugger_domain_(this),
      device_orientation_domain_(this),
      dom_domain_(this),
      dom_debugger_domain_(this),
      dom_snapshot_domain_(this),
      dom_storage_domain_(this),
      emulation_domain_(this),
      headless_experimental_domain_(this),
      heap_profiler_domain_(this),
      indexeddb_domain_(this),
      input_domain_(this),
      inspector_domain_(this),
      io_domain_(this),
      layer_tree_domain_(this),
      log_domain_(this),
      memory_domain_(this),
      network_domain_(this),
      page_domain_(this),
      performance_domain_(this),
      profiler_domain_(this),
      runtime_domain_(this),
      security_domain_(this),
      service_worker_domain_(this),
      target_domain_(this),
      tracing_domain_(this),
      browser_main_thread_(content::BrowserThread::GetTaskRunnerForThread(
          content::BrowserThread::UI)),
      weak_ptr_factory_(this) {}

HeadlessDevToolsClientImpl::~HeadlessDevToolsClientImpl() = default;

void HeadlessDevToolsClientImpl::AttachToHost(
    content::DevToolsAgentHost* agent_host) {
  DCHECK(!agent_host_ && !external_host_);
  agent_host->AttachClient(this);
  agent_host_ = agent_host;
}

void HeadlessDevToolsClientImpl::AttachToExternalHost(
    ExternalHost* external_host) {
  DCHECK(!agent_host_ && !external_host_);
  external_host_ = external_host;
}

void HeadlessDevToolsClientImpl::DetachFromHost(
    content::DevToolsAgentHost* agent_host) {
  DCHECK_EQ(agent_host_, agent_host);
  if (!renderer_crashed_)
    agent_host_->DetachClient(this);
  agent_host_ = nullptr;
  pending_messages_.clear();
}

void HeadlessDevToolsClientImpl::SetRawProtocolListener(
    RawProtocolListener* raw_protocol_listener) {
  raw_protocol_listener_ = raw_protocol_listener;
}

int HeadlessDevToolsClientImpl::GetNextRawDevToolsMessageId() {
  int id = next_raw_message_id_;
  next_raw_message_id_ += 2;
  return id;
}

void HeadlessDevToolsClientImpl::SendRawDevToolsMessage(
    const std::string& json_message) {
#ifndef NDEBUG
  std::unique_ptr<base::Value> message =
      base::JSONReader::Read(json_message, base::JSON_PARSE_RFC);
  const base::Value* id_value = message->FindKey("id");
  if (!id_value) {
    NOTREACHED() << "Badly formed message " << json_message;
    return;
  }
#endif
  DCHECK(agent_host_ || external_host_);
  if (agent_host_)
    agent_host_->DispatchProtocolMessage(this, json_message);
  else
    external_host_->SendProtocolMessage(json_message);
}

void HeadlessDevToolsClientImpl::SendRawDevToolsMessage(
    const base::DictionaryValue& message) {
  std::string json_message;
  base::JSONWriter::Write(message, &json_message);
  SendRawDevToolsMessage(json_message);
}

void HeadlessDevToolsClientImpl::DispatchProtocolMessage(
    content::DevToolsAgentHost* agent_host,
    const std::string& json_message) {
  DCHECK_EQ(agent_host_, agent_host);
  DispatchProtocolMessage(agent_host->GetId(), json_message);
}

void HeadlessDevToolsClientImpl::DispatchMessageFromExternalHost(
    const std::string& json_message) {
  DCHECK(external_host_);
  DispatchProtocolMessage(std::string(), json_message);
}

void HeadlessDevToolsClientImpl::DispatchProtocolMessage(
    const std::string& host_id,
    const std::string& json_message) {
  std::unique_ptr<base::Value> message =
      base::JSONReader::Read(json_message, base::JSON_PARSE_RFC);
  const base::DictionaryValue* message_dict;
  if (!message || !message->GetAsDictionary(&message_dict)) {
    NOTREACHED() << "Badly formed reply " << json_message;
    return;
  }

  if (raw_protocol_listener_ && raw_protocol_listener_->OnProtocolMessage(
                                    host_id, json_message, *message_dict)) {
    return;
  }

  bool success = false;
  if (message_dict->HasKey("id"))
    success = DispatchMessageReply(std::move(message), *message_dict);
  else
    success = DispatchEvent(std::move(message), *message_dict);
  if (!success)
    DLOG(ERROR) << "Unhandled protocol message: " << json_message;
}

bool HeadlessDevToolsClientImpl::DispatchMessageReply(
    std::unique_ptr<base::Value> owning_message,
    const base::DictionaryValue& message_dict) {
  const base::Value* id_value = message_dict.FindKey("id");
  if (!id_value) {
    NOTREACHED() << "ID must be specified.";
    return false;
  }
  auto it = pending_messages_.find(id_value->GetInt());
  if (it == pending_messages_.end()) {
    NOTREACHED() << "Unexpected reply";
    return false;
  }
  Callback callback = std::move(it->second);
  pending_messages_.erase(it);
  if (!callback.callback_with_result.is_null()) {
    const base::DictionaryValue* result_dict;
    if (message_dict.GetDictionary("result", &result_dict)) {
      browser_main_thread_->PostTask(
          FROM_HERE,
          base::BindOnce(
              &HeadlessDevToolsClientImpl::DispatchMessageReplyWithResultTask,
              weak_ptr_factory_.GetWeakPtr(), std::move(owning_message),
              std::move(callback.callback_with_result), result_dict));
    } else if (message_dict.GetDictionary("error", &result_dict)) {
      auto null_value = std::make_unique<base::Value>();
      DLOG(ERROR) << "Error in method call result: " << *result_dict;
      browser_main_thread_->PostTask(
          FROM_HERE,
          base::BindOnce(
              &HeadlessDevToolsClientImpl::DispatchMessageReplyWithResultTask,
              weak_ptr_factory_.GetWeakPtr(), std::move(null_value),
              std::move(callback.callback_with_result), null_value.get()));
    } else {
      NOTREACHED() << "Reply has neither result nor error";
      return false;
    }
  } else if (!callback.callback.is_null()) {
    browser_main_thread_->PostTask(
        FROM_HERE,
        base::BindOnce(
            [](base::WeakPtr<HeadlessDevToolsClientImpl> self,
               base::OnceClosure callback) {
              if (self)
                std::move(callback).Run();
            },
            weak_ptr_factory_.GetWeakPtr(), std::move(callback.callback)));
  }
  return true;
}

void HeadlessDevToolsClientImpl::DispatchMessageReplyWithResultTask(
    std::unique_ptr<base::Value> owning_message,
    base::OnceCallback<void(const base::Value&)> callback,
    const base::Value* result_dict) {
  std::move(callback).Run(*result_dict);
}

bool HeadlessDevToolsClientImpl::DispatchEvent(
    std::unique_ptr<base::Value> owning_message,
    const base::DictionaryValue& message_dict) {
  const base::Value* method_value = message_dict.FindKey("method");
  if (!method_value)
    return false;
  const std::string& method = method_value->GetString();
  if (method == "Inspector.targetCrashed")
    renderer_crashed_ = true;
  EventHandlerMap::const_iterator it = event_handlers_.find(method);
  if (it == event_handlers_.end()) {
    if (method != "Inspector.targetCrashed")
      NOTREACHED() << "Unknown event: " << method;
    return false;
  }
  if (!it->second.is_null()) {
    const base::DictionaryValue* result_dict;
    if (!message_dict.GetDictionary("params", &result_dict)) {
      NOTREACHED() << "Badly formed event parameters";
      return false;
    }
    // DevTools assumes event handling is async so we must post a task here or
    // we risk breaking things.
    browser_main_thread_->PostTask(
        FROM_HERE,
        base::BindOnce(&HeadlessDevToolsClientImpl::DispatchEventTask,
                       weak_ptr_factory_.GetWeakPtr(),
                       std::move(owning_message), &it->second, result_dict));
  }
  return true;
}

void HeadlessDevToolsClientImpl::DispatchEventTask(
    std::unique_ptr<base::Value> owning_message,
    const EventHandler* event_handler,
    const base::DictionaryValue* result_dict) {
  event_handler->Run(*result_dict);
}

void HeadlessDevToolsClientImpl::AgentHostClosed(
    content::DevToolsAgentHost* agent_host) {
  DCHECK_EQ(agent_host_, agent_host);
  agent_host = nullptr;
  pending_messages_.clear();
}

accessibility::Domain* HeadlessDevToolsClientImpl::GetAccessibility() {
  return &accessibility_domain_;
}

animation::Domain* HeadlessDevToolsClientImpl::GetAnimation() {
  return &animation_domain_;
}

application_cache::Domain* HeadlessDevToolsClientImpl::GetApplicationCache() {
  return &application_cache_domain_;
}

browser::Domain* HeadlessDevToolsClientImpl::GetBrowser() {
  return &browser_domain_;
}

cache_storage::Domain* HeadlessDevToolsClientImpl::GetCacheStorage() {
  return &cache_storage_domain_;
}

console::Domain* HeadlessDevToolsClientImpl::GetConsole() {
  return &console_domain_;
}

css::Domain* HeadlessDevToolsClientImpl::GetCSS() {
  return &css_domain_;
}

database::Domain* HeadlessDevToolsClientImpl::GetDatabase() {
  return &database_domain_;
}

debugger::Domain* HeadlessDevToolsClientImpl::GetDebugger() {
  return &debugger_domain_;
}

device_orientation::Domain* HeadlessDevToolsClientImpl::GetDeviceOrientation() {
  return &device_orientation_domain_;
}

dom::Domain* HeadlessDevToolsClientImpl::GetDOM() {
  return &dom_domain_;
}

dom_debugger::Domain* HeadlessDevToolsClientImpl::GetDOMDebugger() {
  return &dom_debugger_domain_;
}

dom_snapshot::Domain* HeadlessDevToolsClientImpl::GetDOMSnapshot() {
  return &dom_snapshot_domain_;
}

dom_storage::Domain* HeadlessDevToolsClientImpl::GetDOMStorage() {
  return &dom_storage_domain_;
}

emulation::Domain* HeadlessDevToolsClientImpl::GetEmulation() {
  return &emulation_domain_;
}

headless_experimental::Domain*
HeadlessDevToolsClientImpl::GetHeadlessExperimental() {
  return &headless_experimental_domain_;
}

heap_profiler::Domain* HeadlessDevToolsClientImpl::GetHeapProfiler() {
  return &heap_profiler_domain_;
}

indexeddb::Domain* HeadlessDevToolsClientImpl::GetIndexedDB() {
  return &indexeddb_domain_;
}

input::Domain* HeadlessDevToolsClientImpl::GetInput() {
  return &input_domain_;
}

inspector::Domain* HeadlessDevToolsClientImpl::GetInspector() {
  return &inspector_domain_;
}

io::Domain* HeadlessDevToolsClientImpl::GetIO() {
  return &io_domain_;
}

layer_tree::Domain* HeadlessDevToolsClientImpl::GetLayerTree() {
  return &layer_tree_domain_;
}

log::Domain* HeadlessDevToolsClientImpl::GetLog() {
  return &log_domain_;
}

memory::Domain* HeadlessDevToolsClientImpl::GetMemory() {
  return &memory_domain_;
}

network::Domain* HeadlessDevToolsClientImpl::GetNetwork() {
  return &network_domain_;
}

page::Domain* HeadlessDevToolsClientImpl::GetPage() {
  return &page_domain_;
}

performance::Domain* HeadlessDevToolsClientImpl::GetPerformance() {
  return &performance_domain_;
}

profiler::Domain* HeadlessDevToolsClientImpl::GetProfiler() {
  return &profiler_domain_;
}

runtime::Domain* HeadlessDevToolsClientImpl::GetRuntime() {
  return &runtime_domain_;
}

security::Domain* HeadlessDevToolsClientImpl::GetSecurity() {
  return &security_domain_;
}

service_worker::Domain* HeadlessDevToolsClientImpl::GetServiceWorker() {
  return &service_worker_domain_;
}

target::Domain* HeadlessDevToolsClientImpl::GetTarget() {
  return &target_domain_;
}

tracing::Domain* HeadlessDevToolsClientImpl::GetTracing() {
  return &tracing_domain_;
}

template <typename CallbackType>
void HeadlessDevToolsClientImpl::FinalizeAndSendMessage(
    base::DictionaryValue* message,
    CallbackType callback) {
  if (renderer_crashed_)
    return;
  DCHECK(agent_host_ || external_host_);
  int id = next_message_id_;
  next_message_id_ += 2;  // We only send even numbered messages.
  message->SetInteger("id", id);
  std::string json_message;
  base::JSONWriter::Write(*message, &json_message);
  pending_messages_[id] = Callback(std::move(callback));
  if (agent_host_)
    agent_host_->DispatchProtocolMessage(this, json_message);
  else
    external_host_->SendProtocolMessage(json_message);
}

template <typename CallbackType>
void HeadlessDevToolsClientImpl::SendMessageWithParams(
    const char* method,
    std::unique_ptr<base::Value> params,
    CallbackType callback) {
  base::DictionaryValue message;
  message.SetString("method", method);
  message.Set("params", std::move(params));
  FinalizeAndSendMessage(&message, std::move(callback));
}

void HeadlessDevToolsClientImpl::SendMessage(
    const char* method,
    std::unique_ptr<base::Value> params,
    base::OnceCallback<void(const base::Value&)> callback) {
  SendMessageWithParams(method, std::move(params), std::move(callback));
}

void HeadlessDevToolsClientImpl::SendMessage(
    const char* method,
    std::unique_ptr<base::Value> params,
    base::OnceClosure callback) {
  SendMessageWithParams(method, std::move(params), std::move(callback));
}

void HeadlessDevToolsClientImpl::RegisterEventHandler(
    const char* method,
    base::RepeatingCallback<void(const base::Value&)> callback) {
  DCHECK(event_handlers_.find(method) == event_handlers_.end());
  event_handlers_[method] = std::move(callback);
}

HeadlessDevToolsClientImpl::Callback::Callback() = default;

HeadlessDevToolsClientImpl::Callback::Callback(Callback&& other) = default;

HeadlessDevToolsClientImpl::Callback::Callback(base::OnceClosure callback)
    : callback(std::move(callback)) {}

HeadlessDevToolsClientImpl::Callback::Callback(
    base::OnceCallback<void(const base::Value&)> callback)
    : callback_with_result(std::move(callback)) {}

HeadlessDevToolsClientImpl::Callback::~Callback() = default;

HeadlessDevToolsClientImpl::Callback& HeadlessDevToolsClientImpl::Callback::
operator=(Callback&& other) = default;

}  // namespace headless
