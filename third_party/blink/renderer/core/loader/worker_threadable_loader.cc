/*
 * Copyright (C) 2009, 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/core/loader/worker_threadable_loader.h"

#include <memory>

#include "base/debug/alias.h"
#include "third_party/blink/public/platform/task_type.h"
#include "third_party/blink/renderer/core/loader/document_threadable_loader.h"
#include "third_party/blink/renderer/core/loader/threadable_loading_context.h"
#include "third_party/blink/renderer/core/timing/worker_global_scope_performance.h"
#include "third_party/blink/renderer/core/workers/worker_global_scope.h"
#include "third_party/blink/renderer/core/workers/worker_thread.h"
#include "third_party/blink/renderer/core/workers/worker_thread_lifecycle_context.h"
#include "third_party/blink/renderer/platform/cross_thread_functional.h"
#include "third_party/blink/renderer/platform/heap/safe_point.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_error.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_request.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_response.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_timing_info.h"
#include "third_party/blink/renderer/platform/weborigin/kurl.h"
#include "third_party/blink/renderer/platform/weborigin/security_policy.h"
#include "third_party/blink/renderer/platform/wtf/functional.h"

namespace blink {

namespace {

std::unique_ptr<Vector<char>> CreateVectorFromMemoryRegion(
    const char* data,
    unsigned data_length) {
  std::unique_ptr<Vector<char>> buffer =
      std::make_unique<Vector<char>>(data_length);
  memcpy(buffer->data(), data, data_length);
  return buffer;
}

}  // namespace

struct WorkerThreadableLoader::TaskWithLocation final {
  TaskWithLocation(const base::Location& location, CrossThreadClosure task)
      : location_(location), task_(std::move(task)) {}
  TaskWithLocation(TaskWithLocation&& task)
      : TaskWithLocation(task.location_, std::move(task.task_)) {}
  ~TaskWithLocation() = default;

  base::Location location_;
  CrossThreadClosure task_;
};

// Observing functions and wait() need to be called on the worker thread.
// Setting functions and signal() need to be called on the parent thread.
// All observing functions must be called after wait() returns, and all
// setting functions must be called before signal() is called.
class WorkerThreadableLoader::WaitableEventWithTasks final
    : public ThreadSafeRefCounted<WaitableEventWithTasks> {
 public:
  static scoped_refptr<WaitableEventWithTasks> Create(
      scoped_refptr<base::SingleThreadTaskRunner> worker_loading_task_runner) {
    return base::AdoptRef(
        new WaitableEventWithTasks(worker_loading_task_runner));
  }

  void Signal() {
#if DCHECK_IS_ON()
    DCHECK(!worker_loading_task_runner_->BelongsToCurrentThread());
#endif
    CHECK(!is_signal_called_);
    is_signal_called_ = true;
    event_.Signal();
  }
  void Wait() {
#if DCHECK_IS_ON()
    DCHECK(worker_loading_task_runner_->BelongsToCurrentThread());
#endif
    CHECK(!is_wait_done_);
    event_.Wait();
    is_wait_done_ = true;
  }

  // Observing functions
  bool IsAborted() const {
#if DCHECK_IS_ON()
    DCHECK(worker_loading_task_runner_->BelongsToCurrentThread());
#endif
    CHECK(is_wait_done_);
    return is_aborted_;
  }
  Vector<TaskWithLocation> Take() {
#if DCHECK_IS_ON()
    DCHECK(worker_loading_task_runner_->BelongsToCurrentThread());
#endif
    CHECK(is_wait_done_);
    return std::move(tasks_);
  }

  // Setting functions
  void Append(TaskWithLocation task) {
#if DCHECK_IS_ON()
    DCHECK(!worker_loading_task_runner_->BelongsToCurrentThread());
#endif
    CHECK(!is_signal_called_);
    tasks_.push_back(std::move(task));
  }
  void SetIsAborted() {
#if DCHECK_IS_ON()
    DCHECK(!worker_loading_task_runner_->BelongsToCurrentThread());
#endif
    CHECK(!is_signal_called_);
    is_aborted_ = true;
  }

#if DCHECK_IS_ON()
  // A task runner from the WorkerGlobalScope that requested the fetch.
  // Used for thread correctness DCHECKs.
  scoped_refptr<base::SingleThreadTaskRunner> worker_loading_task_runner_;
#endif

 private:
  explicit WaitableEventWithTasks(
      scoped_refptr<base::SingleThreadTaskRunner> worker_loading_task_runner) {
#if DCHECK_IS_ON()
    worker_loading_task_runner_ = worker_loading_task_runner;
#endif
  }

  WaitableEvent event_;
  Vector<TaskWithLocation> tasks_;
  bool is_aborted_ = false;
  bool is_signal_called_ = false;
  bool is_wait_done_ = false;
};

WorkerThreadableLoader::TaskForwarder::TaskForwarder(
    scoped_refptr<WaitableEventWithTasks> event_with_tasks)
    : event_with_tasks_(std::move(event_with_tasks)) {
#if DCHECK_IS_ON()
  DCHECK(!event_with_tasks_->worker_loading_task_runner_
              ->BelongsToCurrentThread());
#endif
}

void WorkerThreadableLoader::TaskForwarder::ForwardTask(
    const base::Location& location,
    CrossThreadClosure task) {
#if DCHECK_IS_ON()
  DCHECK(!event_with_tasks_->worker_loading_task_runner_
              ->BelongsToCurrentThread());
#endif
  event_with_tasks_->Append(TaskWithLocation(location, std::move(task)));
}

void WorkerThreadableLoader::TaskForwarder::ForwardTaskWithDoneSignal(
    const base::Location& location,
    CrossThreadClosure task) {
#if DCHECK_IS_ON()
  DCHECK(!event_with_tasks_->worker_loading_task_runner_
              ->BelongsToCurrentThread());
#endif
  event_with_tasks_->Append(TaskWithLocation(location, std::move(task)));
  event_with_tasks_->Signal();
}

void WorkerThreadableLoader::TaskForwarder::Abort() {
#if DCHECK_IS_ON()
  DCHECK(!event_with_tasks_->worker_loading_task_runner_
              ->BelongsToCurrentThread());
#endif
  event_with_tasks_->SetIsAborted();
  event_with_tasks_->Signal();
}

WorkerThreadableLoader::WorkerThreadableLoader(
    WorkerGlobalScope& worker_global_scope,
    ThreadableLoaderClient* client,
    const ThreadableLoaderOptions& options,
    const ResourceLoaderOptions& resource_loader_options)
    : worker_global_scope_(&worker_global_scope),
      parent_execution_context_task_runners_(
          worker_global_scope.GetThread()
              ->GetParentExecutionContextTaskRunners()),
      client_(client),
      threadable_loader_options_(options),
      resource_loader_options_(resource_loader_options) {
  DCHECK(client);
}

void WorkerThreadableLoader::LoadResourceSynchronously(
    WorkerGlobalScope& worker_global_scope,
    const ResourceRequest& request,
    ThreadableLoaderClient& client,
    const ThreadableLoaderOptions& options,
    const ResourceLoaderOptions& resource_loader_options) {
  (new WorkerThreadableLoader(worker_global_scope, &client, options,
                              resource_loader_options))
      ->Start(request);
}

WorkerThreadableLoader::~WorkerThreadableLoader() {
  DCHECK(!parent_thread_loader_holder_);
  DCHECK(!client_);
}

void WorkerThreadableLoader::Start(const ResourceRequest& original_request) {
  DCHECK(worker_global_scope_->IsContextThread());
  ResourceRequest request(original_request);
  if (!request.DidSetHTTPReferrer()) {
    request.SetHTTPReferrer(SecurityPolicy::GenerateReferrer(
        worker_global_scope_->GetReferrerPolicy(), request.Url(),
        worker_global_scope_->OutgoingReferrer()));
  }


  WorkerThread* worker_thread = worker_global_scope_->GetThread();
  scoped_refptr<base::SingleThreadTaskRunner> worker_loading_task_runner =
      worker_global_scope_->GetTaskRunner(TaskType::kInternalLoading);
  scoped_refptr<WaitableEventWithTasks> event_with_tasks =
      WaitableEventWithTasks::Create(worker_loading_task_runner);
  PostCrossThreadTask(
      *parent_execution_context_task_runners_->Get(TaskType::kInternalLoading),
      FROM_HERE,
      CrossThreadBind(
          &ParentThreadLoaderHolder::CreateAndStart,
          WrapCrossThreadPersistent(this),
          WrapCrossThreadPersistent(worker_thread->GetLoadingContext()),
          std::move(worker_loading_task_runner),
          WrapCrossThreadPersistent(
              worker_thread->GetWorkerThreadLifecycleContext()),
          request, threadable_loader_options_, resource_loader_options_,
          event_with_tasks));

  event_with_tasks->Wait();

  if (event_with_tasks->IsAborted()) {
    // This thread is going to terminate.
    Cancel();
    return;
  }

  for (auto& task : event_with_tasks->Take()) {
    // Store the program counter where the task is posted from, and alias
    // it to ensure it is stored in the crash dump.
    const void* program_counter = task.location_.program_counter();
    base::debug::Alias(&program_counter);

    std::move(task.task_).Run();
  }
}

void WorkerThreadableLoader::OverrideTimeout(
    unsigned long timeout_milliseconds) {
  DCHECK(worker_global_scope_->IsContextThread());
  if (!parent_thread_loader_holder_)
    return;
  PostCrossThreadTask(
      *parent_execution_context_task_runners_->Get(TaskType::kInternalLoading),
      FROM_HERE,
      CrossThreadBind(&ParentThreadLoaderHolder::OverrideTimeout,
                      parent_thread_loader_holder_, timeout_milliseconds));
}

void WorkerThreadableLoader::Cancel() {
  DCHECK(worker_global_scope_->IsContextThread());
  if (parent_thread_loader_holder_) {
    PostCrossThreadTask(*parent_execution_context_task_runners_->Get(
                            TaskType::kInternalLoading),
                        FROM_HERE,
                        CrossThreadBind(&ParentThreadLoaderHolder::Cancel,
                                        parent_thread_loader_holder_));
    parent_thread_loader_holder_ = nullptr;
  }

  if (!client_)
    return;

  // If the client hasn't reached a termination state, then transition it
  // by sending a cancellation error.
  // Note: no more client callbacks will be done after this method -- the
  // clearClient() call ensures that.
  DidFail(ResourceError::CancelledError(KURL()));
  DCHECK(!client_);
}

void WorkerThreadableLoader::Detach() {
  // NOTREACHED
  // Currently only "synchronous" requests are using this class and we will
  // deprecate it in the future. As this method cannot be called for such
  // requests, we don't implement it.
  CHECK(false);
}

void WorkerThreadableLoader::DidStart(
    ParentThreadLoaderHolder* parent_thread_loader_holder) {
  DCHECK(worker_global_scope_->IsContextThread());
  DCHECK(!parent_thread_loader_holder_);
  DCHECK(parent_thread_loader_holder);
  if (!client_) {
    // The thread is terminating.
    PostCrossThreadTask(*parent_execution_context_task_runners_->Get(
                            TaskType::kInternalLoading),
                        FROM_HERE,
                        CrossThreadBind(&ParentThreadLoaderHolder::Cancel,
                                        WrapCrossThreadPersistent(
                                            parent_thread_loader_holder)));
    return;
  }

  parent_thread_loader_holder_ = parent_thread_loader_holder;
}

void WorkerThreadableLoader::DidSendData(
    unsigned long long bytes_sent,
    unsigned long long total_bytes_to_be_sent) {
  DCHECK(worker_global_scope_->IsContextThread());
  if (!client_)
    return;
  client_->DidSendData(bytes_sent, total_bytes_to_be_sent);
}

void WorkerThreadableLoader::DidReceiveRedirectTo(const KURL& url) {
  DCHECK(worker_global_scope_->IsContextThread());
  if (!client_)
    return;
  client_->DidReceiveRedirectTo(url);
}

void WorkerThreadableLoader::DidReceiveResponse(
    unsigned long identifier,
    std::unique_ptr<CrossThreadResourceResponseData> response_data,
    std::unique_ptr<WebDataConsumerHandle> handle) {
  DCHECK(worker_global_scope_->IsContextThread());
  if (!client_)
    return;
  ResourceResponse response(response_data.get());
  client_->DidReceiveResponse(identifier, response, std::move(handle));
}

void WorkerThreadableLoader::DidReceiveData(
    std::unique_ptr<Vector<char>> data) {
  DCHECK(worker_global_scope_->IsContextThread());
  CHECK_LE(data->size(), std::numeric_limits<unsigned>::max());
  if (!client_)
    return;
  client_->DidReceiveData(data->data(), data->size());
}

void WorkerThreadableLoader::DidReceiveCachedMetadata(
    std::unique_ptr<Vector<char>> data) {
  DCHECK(worker_global_scope_->IsContextThread());
  if (!client_)
    return;
  client_->DidReceiveCachedMetadata(data->data(), data->size());
}

void WorkerThreadableLoader::DidFinishLoading(unsigned long identifier) {
  DCHECK(worker_global_scope_->IsContextThread());
  if (!client_)
    return;
  auto* client = client_;
  client_ = nullptr;
  parent_thread_loader_holder_ = nullptr;
  client->DidFinishLoading(identifier);
}

void WorkerThreadableLoader::DidFail(const ResourceError& error) {
  DCHECK(worker_global_scope_->IsContextThread());
  if (!client_)
    return;
  auto* client = client_;
  client_ = nullptr;
  parent_thread_loader_holder_ = nullptr;
  client->DidFail(error);
}

void WorkerThreadableLoader::DidFailRedirectCheck() {
  DCHECK(worker_global_scope_->IsContextThread());
  if (!client_)
    return;
  auto* client = client_;
  client_ = nullptr;
  parent_thread_loader_holder_ = nullptr;
  client->DidFailRedirectCheck();
}

void WorkerThreadableLoader::DidDownloadData(int data_length) {
  DCHECK(worker_global_scope_->IsContextThread());
  if (!client_)
    return;
  client_->DidDownloadData(data_length);
}

void WorkerThreadableLoader::DidDownloadToBlob(
    scoped_refptr<BlobDataHandle> blob) {
  DCHECK(worker_global_scope_->IsContextThread());
  if (!client_)
    return;
  client_->DidDownloadToBlob(std::move(blob));
}

void WorkerThreadableLoader::DidReceiveResourceTiming(
    std::unique_ptr<CrossThreadResourceTimingInfoData> timing_data) {
  DCHECK(worker_global_scope_->IsContextThread());
  if (!client_)
    return;
  scoped_refptr<ResourceTimingInfo> info(
      ResourceTimingInfo::Adopt(std::move(timing_data)));
  WorkerGlobalScopePerformance::performance(*worker_global_scope_)
      ->GenerateAndAddResourceTiming(*info);
  client_->DidReceiveResourceTiming(*info);
}

void WorkerThreadableLoader::Trace(blink::Visitor* visitor) {
  visitor->Trace(worker_global_scope_);
  ThreadableLoader::Trace(visitor);
}

void WorkerThreadableLoader::ParentThreadLoaderHolder::CreateAndStart(
    WorkerThreadableLoader* worker_loader,
    ThreadableLoadingContext* loading_context,
    scoped_refptr<base::SingleThreadTaskRunner> worker_loading_task_runner,
    WorkerThreadLifecycleContext* worker_thread_lifecycle_context,
    std::unique_ptr<CrossThreadResourceRequestData> request,
    const ThreadableLoaderOptions& options,
    const ResourceLoaderOptions& resource_loader_options,
    scoped_refptr<WaitableEventWithTasks> event_with_tasks) {
  DCHECK(loading_context->GetExecutionContext()->IsContextThread());
  DCHECK(event_with_tasks);
  TaskForwarder* forwarder = new TaskForwarder(std::move(event_with_tasks));

  ParentThreadLoaderHolder* parent_thread_loader_holder =
      new ParentThreadLoaderHolder(forwarder, worker_thread_lifecycle_context);
  if (parent_thread_loader_holder
          ->WasContextDestroyedBeforeObserverCreation()) {
    // The thread is already terminating.
    forwarder->Abort();
    parent_thread_loader_holder->forwarder_ = nullptr;
    return;
  }
  parent_thread_loader_holder->worker_loader_ = worker_loader;
  forwarder->ForwardTask(
      FROM_HERE,
      CrossThreadBind(&WorkerThreadableLoader::DidStart,
                      WrapCrossThreadPersistent(worker_loader),
                      WrapCrossThreadPersistent(parent_thread_loader_holder)));
  parent_thread_loader_holder->Start(*loading_context, std::move(request),
                                     options, resource_loader_options);
}

WorkerThreadableLoader::ParentThreadLoaderHolder::~ParentThreadLoaderHolder() {
  DCHECK(!worker_loader_);
}

void WorkerThreadableLoader::ParentThreadLoaderHolder::OverrideTimeout(
    unsigned long timeout_milliseconds) {
  if (!parent_thread_loader_)
    return;
  DCHECK(parent_thread_loader_->GetExecutionContext()->IsContextThread());
  parent_thread_loader_->OverrideTimeout(timeout_milliseconds);
}

void WorkerThreadableLoader::ParentThreadLoaderHolder::Cancel() {
  worker_loader_ = nullptr;
  if (!parent_thread_loader_)
    return;
  DCHECK(parent_thread_loader_->GetExecutionContext()->IsContextThread());
  parent_thread_loader_->Cancel();
  parent_thread_loader_ = nullptr;
}

void WorkerThreadableLoader::ParentThreadLoaderHolder::DidSendData(
    unsigned long long bytes_sent,
    unsigned long long total_bytes_to_be_sent) {
  DCHECK(parent_thread_loader_->GetExecutionContext()->IsContextThread());
  CrossThreadPersistent<WorkerThreadableLoader> worker_loader =
      worker_loader_.Get();
  if (!worker_loader || !forwarder_)
    return;
  forwarder_->ForwardTask(
      FROM_HERE,
      CrossThreadBind(&WorkerThreadableLoader::DidSendData, worker_loader,
                      bytes_sent, total_bytes_to_be_sent));
}

void WorkerThreadableLoader::ParentThreadLoaderHolder::DidReceiveRedirectTo(
    const KURL& url) {
  DCHECK(parent_thread_loader_->GetExecutionContext()->IsContextThread());
  CrossThreadPersistent<WorkerThreadableLoader> worker_loader =
      worker_loader_.Get();
  if (!worker_loader || !forwarder_)
    return;
  forwarder_->ForwardTask(
      FROM_HERE, CrossThreadBind(&WorkerThreadableLoader::DidReceiveRedirectTo,
                                 worker_loader, url));
}

void WorkerThreadableLoader::ParentThreadLoaderHolder::DidReceiveResponse(
    unsigned long identifier,
    const ResourceResponse& response,
    std::unique_ptr<WebDataConsumerHandle> handle) {
  DCHECK(parent_thread_loader_->GetExecutionContext()->IsContextThread());
  CrossThreadPersistent<WorkerThreadableLoader> worker_loader =
      worker_loader_.Get();
  if (!worker_loader || !forwarder_)
    return;
  forwarder_->ForwardTask(
      FROM_HERE, CrossThreadBind(&WorkerThreadableLoader::DidReceiveResponse,
                                 worker_loader, identifier, response,
                                 WTF::Passed(std::move(handle))));
}

void WorkerThreadableLoader::ParentThreadLoaderHolder::DidReceiveData(
    const char* data,
    unsigned data_length) {
  DCHECK(parent_thread_loader_->GetExecutionContext()->IsContextThread());
  CrossThreadPersistent<WorkerThreadableLoader> worker_loader =
      worker_loader_.Get();
  if (!worker_loader || !forwarder_)
    return;
  forwarder_->ForwardTask(
      FROM_HERE,
      CrossThreadBind(
          &WorkerThreadableLoader::DidReceiveData, worker_loader,
          WTF::Passed(CreateVectorFromMemoryRegion(data, data_length))));
}

void WorkerThreadableLoader::ParentThreadLoaderHolder::DidDownloadData(
    int data_length) {
  DCHECK(parent_thread_loader_->GetExecutionContext()->IsContextThread());
  CrossThreadPersistent<WorkerThreadableLoader> worker_loader =
      worker_loader_.Get();
  if (!worker_loader || !forwarder_)
    return;
  forwarder_->ForwardTask(
      FROM_HERE, CrossThreadBind(&WorkerThreadableLoader::DidDownloadData,
                                 worker_loader, data_length));
}

void WorkerThreadableLoader::ParentThreadLoaderHolder::DidDownloadToBlob(
    scoped_refptr<BlobDataHandle> blob) {
  DCHECK(parent_thread_loader_->GetExecutionContext()->IsContextThread());
  CrossThreadPersistent<WorkerThreadableLoader> worker_loader =
      worker_loader_.Get();
  if (!worker_loader || !forwarder_)
    return;
  forwarder_->ForwardTask(
      FROM_HERE, CrossThreadBind(&WorkerThreadableLoader::DidDownloadToBlob,
                                 worker_loader, std::move(blob)));
}

void WorkerThreadableLoader::ParentThreadLoaderHolder::DidReceiveCachedMetadata(
    const char* data,
    int data_length) {
  DCHECK(parent_thread_loader_->GetExecutionContext()->IsContextThread());
  CrossThreadPersistent<WorkerThreadableLoader> worker_loader =
      worker_loader_.Get();
  if (!worker_loader || !forwarder_)
    return;
  forwarder_->ForwardTask(
      FROM_HERE,
      CrossThreadBind(
          &WorkerThreadableLoader::DidReceiveCachedMetadata, worker_loader,
          WTF::Passed(CreateVectorFromMemoryRegion(data, data_length))));
}

void WorkerThreadableLoader::ParentThreadLoaderHolder::DidFinishLoading(
    unsigned long identifier) {
  DCHECK(parent_thread_loader_->GetExecutionContext()->IsContextThread());
  CrossThreadPersistent<WorkerThreadableLoader> worker_loader =
      worker_loader_.Release();
  if (!worker_loader || !forwarder_)
    return;
  forwarder_->ForwardTaskWithDoneSignal(
      FROM_HERE, CrossThreadBind(&WorkerThreadableLoader::DidFinishLoading,
                                 worker_loader, identifier));
  forwarder_ = nullptr;
}

void WorkerThreadableLoader::ParentThreadLoaderHolder::DidFail(
    const ResourceError& error) {
  DCHECK(parent_thread_loader_->GetExecutionContext()->IsContextThread());
  CrossThreadPersistent<WorkerThreadableLoader> worker_loader =
      worker_loader_.Release();
  if (!worker_loader || !forwarder_)
    return;
  forwarder_->ForwardTaskWithDoneSignal(
      FROM_HERE,
      CrossThreadBind(&WorkerThreadableLoader::DidFail, worker_loader, error));
  forwarder_ = nullptr;
}

void WorkerThreadableLoader::ParentThreadLoaderHolder::DidFailRedirectCheck() {
  DCHECK(parent_thread_loader_->GetExecutionContext()->IsContextThread());
  CrossThreadPersistent<WorkerThreadableLoader> worker_loader =
      worker_loader_.Release();
  if (!worker_loader || !forwarder_)
    return;
  forwarder_->ForwardTaskWithDoneSignal(
      FROM_HERE, CrossThreadBind(&WorkerThreadableLoader::DidFailRedirectCheck,
                                 worker_loader));
  forwarder_ = nullptr;
}

void WorkerThreadableLoader::ParentThreadLoaderHolder::DidReceiveResourceTiming(
    const ResourceTimingInfo& info) {
  DCHECK(parent_thread_loader_->GetExecutionContext()->IsContextThread());
  CrossThreadPersistent<WorkerThreadableLoader> worker_loader =
      worker_loader_.Get();
  if (!worker_loader || !forwarder_)
    return;
  forwarder_->ForwardTask(
      FROM_HERE,
      CrossThreadBind(&WorkerThreadableLoader::DidReceiveResourceTiming,
                      worker_loader, info));
}

void WorkerThreadableLoader::ParentThreadLoaderHolder::ContextDestroyed(
    WorkerThreadLifecycleContext*) {
  DCHECK(parent_thread_loader_->GetExecutionContext()->IsContextThread());
  if (forwarder_) {
    forwarder_->Abort();
    forwarder_ = nullptr;
  }
  Cancel();
}

void WorkerThreadableLoader::ParentThreadLoaderHolder::Trace(
    blink::Visitor* visitor) {
  visitor->Trace(forwarder_);
  visitor->Trace(parent_thread_loader_);
  WorkerThreadLifecycleObserver::Trace(visitor);
}

WorkerThreadableLoader::ParentThreadLoaderHolder::ParentThreadLoaderHolder(
    TaskForwarder* forwarder,
    WorkerThreadLifecycleContext* context)
    : WorkerThreadLifecycleObserver(context), forwarder_(forwarder) {}

void WorkerThreadableLoader::ParentThreadLoaderHolder::Start(
    ThreadableLoadingContext& loading_context,
    std::unique_ptr<CrossThreadResourceRequestData> request,
    const ThreadableLoaderOptions& options,
    const ResourceLoaderOptions& original_resource_loader_options) {
  DCHECK(loading_context.GetExecutionContext()->IsContextThread());
  ResourceLoaderOptions resource_loader_options =
      original_resource_loader_options;
  resource_loader_options.request_initiator_context = kWorkerContext;
  parent_thread_loader_ = DocumentThreadableLoader::Create(
      loading_context, this, options, resource_loader_options);
  parent_thread_loader_->Start(ResourceRequest(request.get()));
}

}  // namespace blink
