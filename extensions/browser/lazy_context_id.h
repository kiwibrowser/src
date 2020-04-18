// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_BROWSER_LAZY_CONTEXT_ID_H_
#define EXTENSIONS_BROWSER_LAZY_CONTEXT_ID_H_

#include "extensions/common/extension_id.h"
#include "url/gurl.h"

namespace content {
class BrowserContext;
}

namespace extensions {
class LazyContextTaskQueue;

class LazyContextId {
 public:
  enum class Type {
    kEventPage,
    kServiceWorker,
  };

  // An event page (lazy background) context.
  LazyContextId(content::BrowserContext* context,
                const ExtensionId& extension_id);

  // An extension service worker context.
  LazyContextId(content::BrowserContext* context,
                const ExtensionId& extension_id,
                const GURL& service_worker_scope);

  bool is_for_event_page() const { return type_ == Type::kEventPage; }
  bool is_for_service_worker() const { return type_ == Type::kServiceWorker; }

  content::BrowserContext* browser_context() const { return context_; }
  void set_browser_context(content::BrowserContext* context) {
    context_ = context;
  }

  const ExtensionId& extension_id() const { return extension_id_; }

  const GURL& service_worker_scope() const {
    DCHECK(is_for_service_worker());
    return service_worker_scope_;
  }

  LazyContextTaskQueue* GetTaskQueue();

 private:
  const Type type_;
  content::BrowserContext* context_;
  const ExtensionId extension_id_;
  const GURL service_worker_scope_;

  DISALLOW_COPY_AND_ASSIGN(LazyContextId);
};

}  // namespace extensions

#endif  // EXTENSIONS_BROWSER_LAZY_CONTEXT_ID_H_
