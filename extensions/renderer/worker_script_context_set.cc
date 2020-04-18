// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/renderer/worker_script_context_set.h"

#include <algorithm>
#include <utility>

#include "extensions/renderer/script_context.h"

namespace extensions {

using ContextVector = std::vector<std::unique_ptr<ScriptContext>>;

namespace {

// Returns an iterator to the ScriptContext associated with |v8_context| from
// |contexts|, or |contexts|->end() if not found.
ContextVector::iterator FindContext(ContextVector* contexts,
                                    v8::Local<v8::Context> v8_context) {
  auto context_matches =
      [&v8_context](const std::unique_ptr<ScriptContext>& context) {
        v8::HandleScope handle_scope(context->isolate());
        v8::Context::Scope context_scope(context->v8_context());
        return context->v8_context() == v8_context;
      };
  return std::find_if(contexts->begin(), contexts->end(), context_matches);
}

}  // namespace

WorkerScriptContextSet::WorkerScriptContextSet() {}

WorkerScriptContextSet::~WorkerScriptContextSet() {}

void WorkerScriptContextSet::Insert(std::unique_ptr<ScriptContext> context) {
  DCHECK_GT(content::WorkerThread::GetCurrentId(), 0)
      << "Must be called on a worker thread";
  ContextVector* contexts = contexts_tls_.Get();
  if (!contexts) {
    // First context added for this thread. Create a new set, then wait for
    // this thread's shutdown.
    contexts = new ContextVector();
    contexts_tls_.Set(contexts);
    content::WorkerThread::AddObserver(this);
  }
  CHECK(FindContext(contexts, context->v8_context()) == contexts->end())
      << "Worker for " << context->url() << " is already in this set";
  contexts->push_back(std::move(context));
}

void WorkerScriptContextSet::Remove(v8::Local<v8::Context> v8_context,
                                    const GURL& url) {
  DCHECK_GT(content::WorkerThread::GetCurrentId(), 0)
      << "Must be called on a worker thread";
  ContextVector* contexts = contexts_tls_.Get();
  if (!contexts) {
    // Thread has already been torn down, and |v8_context| removed. I'm not
    // sure this can actually happen (depends on in what order blink fires
    // events), but SW lifetime has bitten us before, so be cautious.
    return;
  }
  auto context_it = FindContext(contexts, v8_context);
  CHECK(context_it != contexts->end()) << "Worker for " << url
                                       << " is not in this set";
  DCHECK_EQ(url, (*context_it)->url());
  (*context_it)->Invalidate();
  contexts->erase(context_it);
}

void WorkerScriptContextSet::WillStopCurrentWorkerThread() {
  content::WorkerThread::RemoveObserver(this);
  ContextVector* contexts = contexts_tls_.Get();
  DCHECK(contexts);
  for (const auto& context : *contexts)
    context->Invalidate();
  contexts_tls_.Set(nullptr);
  delete contexts;
}

}  // namespace extensions
