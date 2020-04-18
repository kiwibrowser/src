// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/service_worker_task_queue.h"

#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/service_worker_context.h"
#include "content/public/browser/storage_partition.h"
#include "extensions/browser/event_router.h"
#include "extensions/browser/lazy_context_id.h"
#include "extensions/browser/service_worker_task_queue_factory.h"
#include "extensions/common/constants.h"

using content::BrowserContext;
using content::BrowserThread;

namespace extensions {

namespace {

void FinishTask(LazyContextTaskQueue::PendingTask task,
                const ExtensionId& extension_id,
                int process_id,
                int thread_id) {
  auto params = std::make_unique<LazyContextTaskQueue::ContextInfo>(
      extension_id, content::RenderProcessHost::FromID(process_id), thread_id,
      GURL());
  std::move(task).Run(std::move(params));
}

void DidStartActiveWorkerForPattern(LazyContextTaskQueue::PendingTask task,
                                    const ExtensionId& extension_id,
                                    int process_id,
                                    int thread_id) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  content::BrowserThread::PostTask(
      content::BrowserThread::UI, FROM_HERE,
      base::BindOnce(FinishTask, std::move(task), extension_id, process_id,
                     thread_id));
}

void DidStartActiveWorkerFail() {
  DCHECK(false) << "DidStartActiveWorkerFail";
  // TODO(lazyboy): Handle failure case.
}

void GetServiceWorkerInfoOnIO(
    const GURL& pattern,
    const ExtensionId& extension_id,
    content::ServiceWorkerContext* service_worker_context,
    LazyContextTaskQueue::PendingTask task) {
  service_worker_context->StartActiveWorkerForPattern(
      pattern,
      base::BindOnce(&DidStartActiveWorkerForPattern, std::move(task),
                     extension_id),
      base::BindOnce(&DidStartActiveWorkerFail));
}

}  // namespace

ServiceWorkerTaskQueue::ServiceWorkerTaskQueue(
    content::BrowserContext* browser_context) {}

ServiceWorkerTaskQueue::~ServiceWorkerTaskQueue() {}

// static
ServiceWorkerTaskQueue* ServiceWorkerTaskQueue::Get(
    content::BrowserContext* context) {
  return ServiceWorkerTaskQueueFactory::GetForBrowserContext(context);
}

bool ServiceWorkerTaskQueue::ShouldEnqueueTask(content::BrowserContext* context,
                                               const Extension* extension) {
  // We call StartWorker every time we want to dispatch an event to an extension
  // Service worker.
  // TODO(lazyboy): Is that a problem?
  return true;
}

void ServiceWorkerTaskQueue::AddPendingTaskToDispatchEvent(
    LazyContextId* context_id,
    LazyContextTaskQueue::PendingTask task) {
  DCHECK(context_id->is_for_service_worker());
  content::StoragePartition* partition =
      BrowserContext::GetStoragePartitionForSite(
          context_id->browser_context(), context_id->service_worker_scope());
  content::ServiceWorkerContext* service_worker_context =
      partition->GetServiceWorkerContext();

  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::BindOnce(
          &GetServiceWorkerInfoOnIO, context_id->service_worker_scope(),
          context_id->extension_id(), service_worker_context, std::move(task)));
}

}  // namespace extensions
