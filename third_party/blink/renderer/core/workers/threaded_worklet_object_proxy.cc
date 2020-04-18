// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/workers/threaded_worklet_object_proxy.h"

#include <memory>
#include <utility>

#include "base/memory/ptr_util.h"
#include "third_party/blink/renderer/core/workers/threaded_worklet_global_scope.h"
#include "third_party/blink/renderer/core/workers/threaded_worklet_messaging_proxy.h"
#include "third_party/blink/renderer/core/workers/worker_thread.h"

namespace blink {

std::unique_ptr<ThreadedWorkletObjectProxy> ThreadedWorkletObjectProxy::Create(
    ThreadedWorkletMessagingProxy* messaging_proxy_weak_ptr,
    ParentExecutionContextTaskRunners* parent_execution_context_task_runners) {
  DCHECK(messaging_proxy_weak_ptr);
  return base::WrapUnique(new ThreadedWorkletObjectProxy(
      messaging_proxy_weak_ptr, parent_execution_context_task_runners));
}

ThreadedWorkletObjectProxy::~ThreadedWorkletObjectProxy() = default;

void ThreadedWorkletObjectProxy::FetchAndInvokeScript(
    const KURL& module_url_record,
    network::mojom::FetchCredentialsMode credentials_mode,
    scoped_refptr<base::SingleThreadTaskRunner> outside_settings_task_runner,
    WorkletPendingTasks* pending_tasks,
    WorkerThread* worker_thread) {
  ThreadedWorkletGlobalScope* global_scope =
      ToThreadedWorkletGlobalScope(worker_thread->GlobalScope());
  global_scope->FetchAndInvokeScript(module_url_record, credentials_mode,
                                     std::move(outside_settings_task_runner),
                                     pending_tasks);
}

ThreadedWorkletObjectProxy::ThreadedWorkletObjectProxy(
    ThreadedWorkletMessagingProxy* messaging_proxy_weak_ptr,
    ParentExecutionContextTaskRunners* parent_execution_context_task_runners)
    : ThreadedObjectProxyBase(parent_execution_context_task_runners),
      messaging_proxy_weak_ptr_(messaging_proxy_weak_ptr) {}

CrossThreadWeakPersistent<ThreadedMessagingProxyBase>
ThreadedWorkletObjectProxy::MessagingProxyWeakPtr() {
  return messaging_proxy_weak_ptr_;
}

}  // namespace blink
