// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/renderer/worker_thread_dispatcher.h"

#include "base/feature_list.h"
#include "base/lazy_instance.h"
#include "base/threading/platform_thread.h"
#include "base/threading/thread_local.h"
#include "base/values.h"
#include "content/public/renderer/render_thread.h"
#include "content/public/renderer/worker_thread.h"
#include "extensions/common/constants.h"
#include "extensions/common/extension_features.h"
#include "extensions/common/extension_messages.h"
#include "extensions/renderer/dispatcher.h"
#include "extensions/renderer/extension_bindings_system.h"
#include "extensions/renderer/extensions_renderer_client.h"
#include "extensions/renderer/js_extension_bindings_system.h"
#include "extensions/renderer/native_extension_bindings_system.h"
#include "extensions/renderer/service_worker_data.h"

namespace extensions {

namespace {

base::LazyInstance<WorkerThreadDispatcher>::DestructorAtExit
    g_worker_thread_dispatcher_instance = LAZY_INSTANCE_INITIALIZER;
base::LazyInstance<base::ThreadLocalPointer<extensions::ServiceWorkerData>>::
    DestructorAtExit g_data_tls = LAZY_INSTANCE_INITIALIZER;

ServiceWorkerData* GetServiceWorkerData() {
  ServiceWorkerData* data = g_data_tls.Pointer()->Get();
  DCHECK(data);
  return data;
}

}  // namespace

WorkerThreadDispatcher::WorkerThreadDispatcher() {}
WorkerThreadDispatcher::~WorkerThreadDispatcher() {}

WorkerThreadDispatcher* WorkerThreadDispatcher::Get() {
  return g_worker_thread_dispatcher_instance.Pointer();
}

void WorkerThreadDispatcher::Init(content::RenderThread* render_thread) {
  DCHECK(render_thread);
  DCHECK_EQ(content::RenderThread::Get(), render_thread);
  DCHECK(!message_filter_);
  message_filter_ = render_thread->GetSyncMessageFilter();
  render_thread->AddObserver(this);
}

// static
ExtensionBindingsSystem* WorkerThreadDispatcher::GetBindingsSystem() {
  return GetServiceWorkerData()->bindings_system();
}

// static
V8SchemaRegistry* WorkerThreadDispatcher::GetV8SchemaRegistry() {
  return GetServiceWorkerData()->v8_schema_registry();
}

// static
ScriptContext* WorkerThreadDispatcher::GetScriptContext() {
  return GetServiceWorkerData()->context();
}

// static
bool WorkerThreadDispatcher::HandlesMessageOnWorkerThread(
    const IPC::Message& message) {
  return message.type() == ExtensionMsg_ResponseWorker::ID ||
         message.type() == ExtensionMsg_DispatchEvent::ID;
}

// static
void WorkerThreadDispatcher::ForwardIPC(int worker_thread_id,
                                        const IPC::Message& message) {
  WorkerThreadDispatcher::Get()->OnMessageReceivedOnWorkerThread(
      worker_thread_id, message);
}

bool WorkerThreadDispatcher::OnControlMessageReceived(
    const IPC::Message& message) {
  if (HandlesMessageOnWorkerThread(message)) {
    int worker_thread_id = base::kInvalidThreadId;
    // TODO(lazyboy): Route |message| directly to the child thread using routed
    // IPC. Probably using mojo?
    bool found = base::PickleIterator(message).ReadInt(&worker_thread_id);
    CHECK(found);
    if (worker_thread_id == kMainThreadId)
      return false;
    base::TaskRunner* runner = GetTaskRunnerFor(worker_thread_id);
    bool task_posted = runner->PostTask(
        FROM_HERE, base::Bind(&WorkerThreadDispatcher::ForwardIPC,
                              worker_thread_id, message));
    DCHECK(task_posted) << "Could not PostTask IPC to worker thread.";
    return true;
  }
  return false;
}

void WorkerThreadDispatcher::OnMessageReceivedOnWorkerThread(
    int worker_thread_id,
    const IPC::Message& message) {
  CHECK_EQ(content::WorkerThread::GetCurrentId(), worker_thread_id);
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(WorkerThreadDispatcher, message)
    IPC_MESSAGE_HANDLER(ExtensionMsg_ResponseWorker, OnResponseWorker)
    IPC_MESSAGE_HANDLER(ExtensionMsg_DispatchEvent, OnDispatchEvent)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  CHECK(handled);
}

base::TaskRunner* WorkerThreadDispatcher::GetTaskRunnerFor(
    int worker_thread_id) {
  base::AutoLock lock(task_runner_map_lock_);
  return task_runner_map_[worker_thread_id];
}

bool WorkerThreadDispatcher::Send(IPC::Message* message) {
  return message_filter_->Send(message);
}

void WorkerThreadDispatcher::OnResponseWorker(int worker_thread_id,
                                              int request_id,
                                              bool succeeded,
                                              const base::ListValue& response,
                                              const std::string& error) {
  ServiceWorkerData* data = g_data_tls.Pointer()->Get();
  data->bindings_system()->HandleResponse(request_id, succeeded, response,
                                          error);
}

void WorkerThreadDispatcher::OnDispatchEvent(
    const ExtensionMsg_DispatchEvent_Params& params,
    const base::ListValue& event_args) {
  ServiceWorkerData* data = g_data_tls.Pointer()->Get();
  DCHECK(data);
  data->bindings_system()->DispatchEventInContext(
      params.event_name, &event_args, &params.filtering_info, data->context());
}

void WorkerThreadDispatcher::AddWorkerData(
    int64_t service_worker_version_id,
    ScriptContext* context,
    std::unique_ptr<ExtensionBindingsSystem> bindings_system) {
  ServiceWorkerData* data = g_data_tls.Pointer()->Get();
  if (!data) {
    ServiceWorkerData* new_data = new ServiceWorkerData(
        service_worker_version_id, context, std::move(bindings_system));
    g_data_tls.Pointer()->Set(new_data);
  }

  int worker_thread_id = base::PlatformThread::CurrentId();
  DCHECK_EQ(content::WorkerThread::GetCurrentId(), worker_thread_id);
  {
    base::AutoLock lock(task_runner_map_lock_);
    auto* task_runner = base::ThreadTaskRunnerHandle::Get().get();
    CHECK(task_runner);
    task_runner_map_[worker_thread_id] = task_runner;
  }
}

void WorkerThreadDispatcher::RemoveWorkerData(
    int64_t service_worker_version_id) {
  ServiceWorkerData* data = g_data_tls.Pointer()->Get();
  if (data) {
    DCHECK_EQ(service_worker_version_id, data->service_worker_version_id());
    delete data;
    g_data_tls.Pointer()->Set(nullptr);
  }

  int worker_thread_id = base::PlatformThread::CurrentId();
  DCHECK_EQ(content::WorkerThread::GetCurrentId(), worker_thread_id);
  {
    base::AutoLock lock(task_runner_map_lock_);
    task_runner_map_.erase(worker_thread_id);
  }
}

}  // namespace extensions
