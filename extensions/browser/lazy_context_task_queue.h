// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_BROWSER_LAZY_CONTEXT_TASK_QUEUE_H_
#define EXTENSIONS_BROWSER_LAZY_CONTEXT_TASK_QUEUE_H_

#include "base/callback.h"
#include "extensions/common/extension_id.h"
#include "url/gurl.h"

namespace content {
class BrowserContext;
class RenderProcessHost;
}  // namespace content

namespace extensions {
class Extension;
class LazyContextId;

// Interface for performing tasks after loading lazy contexts of an extension.
//
// Lazy contexts are non-persistent, so they can unload any time and this
// interface exposes an async mechanism to perform tasks after loading the
// context.
class LazyContextTaskQueue {
 public:
  // Represents information about an extension lazy context, which is passed to
  // consumers that add tasks to LazyContextTaskQueue.
  struct ContextInfo {
    const ExtensionId extension_id;
    content::RenderProcessHost* const render_process_host;
    const int worker_thread_id;
    const GURL url;
    ContextInfo(const ExtensionId& extension_id,
                content::RenderProcessHost* render_process_host,
                int worker_thread_id,
                const GURL& url)
        : extension_id(extension_id),
          render_process_host(render_process_host),
          worker_thread_id(worker_thread_id),
          url(url) {}
  };
  using PendingTask =
      base::OnceCallback<void(std::unique_ptr<ContextInfo> params)>;

  // Returns true if the task should be added to the queue (that is, if the
  // extension has a lazy background page or service worker that isn't ready
  // yet).
  virtual bool ShouldEnqueueTask(content::BrowserContext* context,
                                 const Extension* extension) = 0;

  // Adds a task to the queue for a given extension. If this is the first
  // task added for the extension, its "lazy context" (i.e. lazy background
  // page for event pages, service worker for extension service workers) will
  // be loaded. The task will be called either when the page is loaded,
  // or when the page fails to load for some reason (e.g. a crash or browser
  // shutdown). In the latter case, the ContextInfo will be nullptr.
  //
  // TODO(lazyboy): Remove "ToDispatchEvent" suffix and simply call this
  // AddPendingTask. Issues:
  // 1. We already have LazyBackgroundTaskQueue::AddPendingTask. Moreover, that
  //    is heavily used thoughout the codebase.
  // 2. LazyBackgroundTaskQueue::AddPendingTask is tied to ExtensionHost. This
  //    class should be ExtensionHost agnostic.
  virtual void AddPendingTaskToDispatchEvent(LazyContextId* context_id,
                                             PendingTask task) = 0;
};

}  // namespace extensions

#endif  // EXTENSIONS_BROWSER_LAZY_CONTEXT_TASK_QUEUE_H_
