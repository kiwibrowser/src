// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/loader/threadable_loading_context.h"

#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/loader/worker_fetch_context.h"
#include "third_party/blink/renderer/core/workers/worker_global_scope.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_fetcher.h"

namespace blink {

class DocumentThreadableLoadingContext final : public ThreadableLoadingContext {
 public:
  explicit DocumentThreadableLoadingContext(Document& document)
      : document_(&document) {}

  ~DocumentThreadableLoadingContext() override = default;

  ResourceFetcher* GetResourceFetcher() override {
    DCHECK(IsContextThread());
    return document_->Fetcher();
  }

  ExecutionContext* GetExecutionContext() override {
    DCHECK(IsContextThread());
    return document_.Get();
  }

  void Trace(blink::Visitor* visitor) override {
    visitor->Trace(document_);
    ThreadableLoadingContext::Trace(visitor);
  }

 private:
  bool IsContextThread() const { return document_->IsContextThread(); }

  Member<Document> document_;
};

class WorkerThreadableLoadingContext : public ThreadableLoadingContext {
 public:
  explicit WorkerThreadableLoadingContext(
      WorkerGlobalScope& worker_global_scope)
      : worker_global_scope_(&worker_global_scope) {}

  ~WorkerThreadableLoadingContext() override = default;

  ResourceFetcher* GetResourceFetcher() override {
    DCHECK(IsContextThread());
    return worker_global_scope_->EnsureFetcher();
  }

  ExecutionContext* GetExecutionContext() override {
    DCHECK(IsContextThread());
    return worker_global_scope_.Get();
  }

  void Trace(blink::Visitor* visitor) override {
    visitor->Trace(worker_global_scope_);
    ThreadableLoadingContext::Trace(visitor);
  }

 private:
  bool IsContextThread() const {
    DCHECK(worker_global_scope_);
    return worker_global_scope_->IsContextThread();
  }

  Member<WorkerGlobalScope> worker_global_scope_;
};

ThreadableLoadingContext* ThreadableLoadingContext::Create(
    ExecutionContext& context) {
  if (context.IsDocument())
    return new DocumentThreadableLoadingContext(ToDocument(context));
  if (context.IsWorkerGlobalScope())
    return new WorkerThreadableLoadingContext(ToWorkerGlobalScope(context));
  NOTREACHED();
  return nullptr;
}

BaseFetchContext* ThreadableLoadingContext::GetFetchContext() {
  return static_cast<BaseFetchContext*>(&GetResourceFetcher()->Context());
}

}  // namespace blink
