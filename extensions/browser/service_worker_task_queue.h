// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_BROWSER_SERVICE_WORKER_TASK_QUEUE_H_
#define EXTENSIONS_BROWSER_SERVICE_WORKER_TASK_QUEUE_H_

#include "components/keyed_service/core/keyed_service.h"
#include "extensions/browser/lazy_context_task_queue.h"
#include "extensions/common/extension_id.h"
#include "url/gurl.h"

namespace content {
class BrowserContext;
}

namespace extensions {
class Extension;
class LazyContextId;

// TODO(lazyboy): This class doesn't queue up any tasks, implement. See
// LazyBackgroundTaskQueue.
// TODO(lazyboy): Clean up queue when extension is unloaded/uninstalled.
class ServiceWorkerTaskQueue : public KeyedService,
                               public LazyContextTaskQueue {
 public:
  explicit ServiceWorkerTaskQueue(content::BrowserContext* browser_context);
  ~ServiceWorkerTaskQueue() override;

  // Convenience method to return the ServiceWorkerTaskQueue for a given
  // |context|.
  static ServiceWorkerTaskQueue* Get(content::BrowserContext* context);

  bool ShouldEnqueueTask(content::BrowserContext* context,
                         const Extension* extension) override;
  void AddPendingTaskToDispatchEvent(
      LazyContextId* context_id,
      LazyContextTaskQueue::PendingTask task) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(ServiceWorkerTaskQueue);
};

}  // namespace extensions

#endif  // EXTENSIONS_BROWSER_SERVICE_WORKER_TASK_QUEUE_H_
