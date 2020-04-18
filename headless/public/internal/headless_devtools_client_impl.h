// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef HEADLESS_PUBLIC_INTERNAL_HEADLESS_DEVTOOLS_CLIENT_IMPL_H_
#define HEADLESS_PUBLIC_INTERNAL_HEADLESS_DEVTOOLS_CLIENT_IMPL_H_

#include <unordered_map>

#include "base/memory/weak_ptr.h"
#include "base/sequenced_task_runner.h"
#include "content/public/browser/devtools_agent_host_client.h"
#include "headless/public/devtools/domains/accessibility.h"
#include "headless/public/devtools/domains/animation.h"
#include "headless/public/devtools/domains/application_cache.h"
#include "headless/public/devtools/domains/browser.h"
#include "headless/public/devtools/domains/cache_storage.h"
#include "headless/public/devtools/domains/console.h"
#include "headless/public/devtools/domains/css.h"
#include "headless/public/devtools/domains/database.h"
#include "headless/public/devtools/domains/debugger.h"
#include "headless/public/devtools/domains/device_orientation.h"
#include "headless/public/devtools/domains/dom.h"
#include "headless/public/devtools/domains/dom_debugger.h"
#include "headless/public/devtools/domains/dom_snapshot.h"
#include "headless/public/devtools/domains/dom_storage.h"
#include "headless/public/devtools/domains/emulation.h"
#include "headless/public/devtools/domains/headless_experimental.h"
#include "headless/public/devtools/domains/heap_profiler.h"
#include "headless/public/devtools/domains/indexeddb.h"
#include "headless/public/devtools/domains/input.h"
#include "headless/public/devtools/domains/inspector.h"
#include "headless/public/devtools/domains/io.h"
#include "headless/public/devtools/domains/layer_tree.h"
#include "headless/public/devtools/domains/log.h"
#include "headless/public/devtools/domains/memory.h"
#include "headless/public/devtools/domains/network.h"
#include "headless/public/devtools/domains/page.h"
#include "headless/public/devtools/domains/performance.h"
#include "headless/public/devtools/domains/profiler.h"
#include "headless/public/devtools/domains/runtime.h"
#include "headless/public/devtools/domains/security.h"
#include "headless/public/devtools/domains/service_worker.h"
#include "headless/public/devtools/domains/target.h"
#include "headless/public/devtools/domains/tracing.h"
#include "headless/public/headless_devtools_client.h"
#include "headless/public/headless_export.h"
#include "headless/public/internal/message_dispatcher.h"

namespace base {
class DictionaryValue;
}

namespace content {
class DevToolsAgentHost;
}

namespace headless {

class HEADLESS_EXPORT HeadlessDevToolsClientImpl
    : public HeadlessDevToolsClient,
      public content::DevToolsAgentHostClient,
      public internal::MessageDispatcher {
 public:
  HeadlessDevToolsClientImpl();
  ~HeadlessDevToolsClientImpl() override;

  static HeadlessDevToolsClientImpl* From(HeadlessDevToolsClient* client);

  // HeadlessDevToolsClient implementation:
  accessibility::Domain* GetAccessibility() override;
  animation::Domain* GetAnimation() override;
  application_cache::Domain* GetApplicationCache() override;
  browser::Domain* GetBrowser() override;
  cache_storage::Domain* GetCacheStorage() override;
  console::Domain* GetConsole() override;
  css::Domain* GetCSS() override;
  database::Domain* GetDatabase() override;
  debugger::Domain* GetDebugger() override;
  device_orientation::Domain* GetDeviceOrientation() override;
  dom::Domain* GetDOM() override;
  dom_debugger::Domain* GetDOMDebugger() override;
  dom_snapshot::Domain* GetDOMSnapshot() override;
  dom_storage::Domain* GetDOMStorage() override;
  emulation::Domain* GetEmulation() override;
  headless_experimental::Domain* GetHeadlessExperimental() override;
  heap_profiler::Domain* GetHeapProfiler() override;
  indexeddb::Domain* GetIndexedDB() override;
  input::Domain* GetInput() override;
  inspector::Domain* GetInspector() override;
  io::Domain* GetIO() override;
  layer_tree::Domain* GetLayerTree() override;
  log::Domain* GetLog() override;
  memory::Domain* GetMemory() override;
  network::Domain* GetNetwork() override;
  page::Domain* GetPage() override;
  performance::Domain* GetPerformance() override;
  profiler::Domain* GetProfiler() override;
  runtime::Domain* GetRuntime() override;
  security::Domain* GetSecurity() override;
  service_worker::Domain* GetServiceWorker() override;
  target::Domain* GetTarget() override;
  tracing::Domain* GetTracing() override;
  void SetRawProtocolListener(
      RawProtocolListener* raw_protocol_listener) override;
  int GetNextRawDevToolsMessageId() override;
  void SendRawDevToolsMessage(const std::string& json_message) override;
  void SendRawDevToolsMessage(const base::DictionaryValue& message) override;
  void DispatchMessageFromExternalHost(
      const std::string& json_message) override;

  // content::DevToolsAgentHostClient implementation:
  void DispatchProtocolMessage(content::DevToolsAgentHost* agent_host,
                               const std::string& json_message) override;
  void AgentHostClosed(content::DevToolsAgentHost* agent_host) override;

  // internal::MessageDispatcher implementation:
  void SendMessage(
      const char* method,
      std::unique_ptr<base::Value> params,
      base::OnceCallback<void(const base::Value&)> callback) override;
  void SendMessage(const char* method,
                   std::unique_ptr<base::Value> params,
                   base::OnceClosure callback) override;
  void RegisterEventHandler(
      const char* method,
      base::RepeatingCallback<void(const base::Value&)> callback) override;

  void AttachToHost(content::DevToolsAgentHost* agent_host);
  void DetachFromHost(content::DevToolsAgentHost* agent_host);

  void AttachToExternalHost(ExternalHost* external_host);

  void SetTaskRunnerForTests(
      scoped_refptr<base::SequencedTaskRunner> task_runner) {
    browser_main_thread_ = task_runner;
  }

 private:
  // Contains a callback with or without a result parameter depending on the
  // message type.
  struct Callback {
    Callback();
    Callback(Callback&& other);
    explicit Callback(base::OnceClosure callback);
    explicit Callback(base::OnceCallback<void(const base::Value&)> callback);
    ~Callback();

    Callback& operator=(Callback&& other);

    base::OnceClosure callback;
    base::OnceCallback<void(const base::Value&)> callback_with_result;
  };

  void DispatchProtocolMessage(const std::string& host_id,
                               const std::string& json_message);

  template <typename CallbackType>
  void FinalizeAndSendMessage(base::DictionaryValue* message,
                              CallbackType callback);

  template <typename CallbackType>
  void SendMessageWithParams(const char* method,
                             std::unique_ptr<base::Value> params,
                             CallbackType callback);

  bool DispatchMessageReply(std::unique_ptr<base::Value> owning_message,
                            const base::DictionaryValue& message_dict);
  void DispatchMessageReplyWithResultTask(
      std::unique_ptr<base::Value> owning_message,
      base::OnceCallback<void(const base::Value&)> callback,
      const base::Value* result_dict);
  using EventHandler = base::RepeatingCallback<void(const base::Value&)>;
  using EventHandlerMap = std::unordered_map<std::string, EventHandler>;

  bool DispatchEvent(std::unique_ptr<base::Value> owning_message,
                     const base::DictionaryValue& message_dict);
  void DispatchEventTask(std::unique_ptr<base::Value> owning_message,
                         const EventHandler* event_handler,
                         const base::DictionaryValue* result_dict);

  content::DevToolsAgentHost* agent_host_ = nullptr;
  ExternalHost* external_host_ = nullptr;
  RawProtocolListener* raw_protocol_listener_ = nullptr;

  int next_message_id_ = 0;
  int next_raw_message_id_ = 1;
  std::unordered_map<int, Callback> pending_messages_;
  EventHandlerMap event_handlers_;

  bool renderer_crashed_ = false;

  accessibility::ExperimentalDomain accessibility_domain_;
  animation::ExperimentalDomain animation_domain_;
  application_cache::ExperimentalDomain application_cache_domain_;
  browser::ExperimentalDomain browser_domain_;
  cache_storage::ExperimentalDomain cache_storage_domain_;
  console::ExperimentalDomain console_domain_;
  css::ExperimentalDomain css_domain_;
  database::ExperimentalDomain database_domain_;
  debugger::ExperimentalDomain debugger_domain_;
  device_orientation::ExperimentalDomain device_orientation_domain_;
  dom::ExperimentalDomain dom_domain_;
  dom_debugger::ExperimentalDomain dom_debugger_domain_;
  dom_snapshot::ExperimentalDomain dom_snapshot_domain_;
  dom_storage::ExperimentalDomain dom_storage_domain_;
  emulation::ExperimentalDomain emulation_domain_;
  headless_experimental::ExperimentalDomain headless_experimental_domain_;
  heap_profiler::ExperimentalDomain heap_profiler_domain_;
  indexeddb::ExperimentalDomain indexeddb_domain_;
  input::ExperimentalDomain input_domain_;
  inspector::ExperimentalDomain inspector_domain_;
  io::ExperimentalDomain io_domain_;
  layer_tree::ExperimentalDomain layer_tree_domain_;
  log::ExperimentalDomain log_domain_;
  memory::ExperimentalDomain memory_domain_;
  network::ExperimentalDomain network_domain_;
  page::ExperimentalDomain page_domain_;
  performance::ExperimentalDomain performance_domain_;
  profiler::ExperimentalDomain profiler_domain_;
  runtime::ExperimentalDomain runtime_domain_;
  security::ExperimentalDomain security_domain_;
  service_worker::ExperimentalDomain service_worker_domain_;
  target::ExperimentalDomain target_domain_;
  tracing::ExperimentalDomain tracing_domain_;
  scoped_refptr<base::SequencedTaskRunner> browser_main_thread_;
  base::WeakPtrFactory<HeadlessDevToolsClientImpl> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(HeadlessDevToolsClientImpl);
};

}  // namespace headless

#endif  // HEADLESS_PUBLIC_INTERNAL_HEADLESS_DEVTOOLS_CLIENT_IMPL_H_
