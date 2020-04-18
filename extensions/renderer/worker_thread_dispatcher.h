// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_RENDERER_WORKER_THREAD_DISPATCHER_H_
#define EXTENSIONS_RENDERER_WORKER_THREAD_DISPATCHER_H_

#include "base/synchronization/lock.h"
#include "base/threading/platform_thread.h"
#include "content/public/renderer/render_thread_observer.h"
#include "content/public/renderer/worker_thread.h"
#include "extensions/renderer/event_bookkeeper.h"
#include "ipc/ipc_sync_message_filter.h"

namespace base {
class ListValue;
}

namespace content {
class RenderThread;
}

struct ExtensionMsg_DispatchEvent_Params;
namespace extensions {
class ExtensionBindingsSystem;
class ScriptContext;
class V8SchemaRegistry;

// Sends and receives IPC in an extension Service Worker.
// TODO(lazyboy): This class should really be a combination of the following
// two:
// 1) A content::WorkerThreadMessageFilter, so that we can receive IPC directly
// on worker thread.
// 2) A content::ThreadSafeSender, so we can safely send IPC from worker thread.
class WorkerThreadDispatcher : public content::RenderThreadObserver,
                               public IPC::Sender {
 public:
  WorkerThreadDispatcher();
  ~WorkerThreadDispatcher() override;

  // Thread safe.
  static WorkerThreadDispatcher* Get();
  static ExtensionBindingsSystem* GetBindingsSystem();
  static V8SchemaRegistry* GetV8SchemaRegistry();
  static ScriptContext* GetScriptContext();

  void Init(content::RenderThread* render_thread);

  // IPC::Sender:
  bool Send(IPC::Message* message) override;

  void AddWorkerData(int64_t service_worker_version_id,
                     ScriptContext* context,
                     std::unique_ptr<ExtensionBindingsSystem> bindings_system);
  void RemoveWorkerData(int64_t service_worker_version_id);

  EventBookkeeper* event_bookkeeper() { return &event_bookkeeper_; }

  // content::RenderThreadObserver:
  bool OnControlMessageReceived(const IPC::Message& message) override;

 private:
  static bool HandlesMessageOnWorkerThread(const IPC::Message& message);
  static void ForwardIPC(int worker_thread_id, const IPC::Message& message);

  void OnMessageReceivedOnWorkerThread(int worker_thread_id,
                                       const IPC::Message& message);

  base::TaskRunner* GetTaskRunnerFor(int worker_thread_id);

  // IPC handlers.
  void OnResponseWorker(int worker_thread_id,
                        int request_id,
                        bool succeeded,
                        const base::ListValue& response,
                        const std::string& error);
  void OnDispatchEvent(const ExtensionMsg_DispatchEvent_Params& params,
                       const base::ListValue& event_args);

  // IPC sender. Belongs to the render thread, but thread safe.
  scoped_refptr<IPC::SyncMessageFilter> message_filter_;

  using IDToTaskRunnerMap = std::map<base::PlatformThreadId, base::TaskRunner*>;
  IDToTaskRunnerMap task_runner_map_;
  base::Lock task_runner_map_lock_;

  EventBookkeeper event_bookkeeper_;

  DISALLOW_COPY_AND_ASSIGN(WorkerThreadDispatcher);
};

}  // namespace extensions

#endif  // EXTENSIONS_RENDERER_WORKER_THREAD_DISPATCHER_H_
