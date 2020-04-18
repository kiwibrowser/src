// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_BROWSER_EXTENSION_SERVICE_WORKER_MESSAGE_FILTER_H_
#define EXTENSIONS_BROWSER_EXTENSION_SERVICE_WORKER_MESSAGE_FILTER_H_

#include "base/macros.h"
#include "content/public/browser/browser_message_filter.h"
#include "content/public/browser/browser_thread.h"

struct ExtensionHostMsg_Request_Params;

namespace content {
class BrowserContext;
class ServiceWorkerContext;
}

namespace extensions {

class ExtensionFunctionDispatcher;

// IPC handler class for extension service worker.
class ExtensionServiceWorkerMessageFilter
    : public content::BrowserMessageFilter {
 public:
  ExtensionServiceWorkerMessageFilter(
      int render_process_id,
      content::BrowserContext* context,
      content::ServiceWorkerContext* service_worker_context);

  // content::BrowserMessageFilter:
  bool OnMessageReceived(const IPC::Message& message) override;
  void OverrideThreadForMessage(const IPC::Message& message,
                                content::BrowserThread::ID* thread) override;

 private:
  ~ExtensionServiceWorkerMessageFilter() override;

  // Message handlers.
  void OnRequestWorker(const ExtensionHostMsg_Request_Params& params);
  void OnIncrementServiceWorkerActivity(int64_t service_worker_version_id,
                                        const std::string& request_uuid);
  void OnDecrementServiceWorkerActivity(int64_t service_worker_version_id,
                                        const std::string& request_uuid);

  const int render_process_id_;

  // Owned by the StoragePartition of our profile.
  content::ServiceWorkerContext* service_worker_context_;

  std::unique_ptr<ExtensionFunctionDispatcher,
                  content::BrowserThread::DeleteOnUIThread>
      dispatcher_;

  DISALLOW_COPY_AND_ASSIGN(ExtensionServiceWorkerMessageFilter);
};

}  // namespace extensions

#endif  // EXTENSIONS_BROWSER_EXTENSION_SERVICE_WORKER_MESSAGE_FILTER_H_
