// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/workers/worklet_module_responses_map.h"

#include "base/optional.h"
#include "third_party/blink/renderer/core/loader/modulescript/document_module_script_fetcher.h"

namespace blink {

namespace {

bool IsValidURL(const KURL& url) {
  return !url.IsEmpty() && url.IsValid();
}

}  // namespace

class WorkletModuleResponsesMap::Entry final
    : public GarbageCollectedFinalized<Entry>,
      public ModuleScriptFetcher::Client {
  USING_GARBAGE_COLLECTED_MIXIN(WorkletModuleResponsesMap::Entry);

 public:
  enum class State { kInitial, kFetching, kFetched, kFailed };

  Entry() = default;
  ~Entry() = default;

  void Fetch(FetchParameters& fetch_params, ResourceFetcher* fetcher) {
    AdvanceState(State::kFetching);
    module_fetcher_ = new DocumentModuleScriptFetcher(fetcher);
    module_fetcher_->Fetch(fetch_params, this);
  }

  State GetState() const { return state_; }
  const ModuleScriptCreationParams& GetParams() const { return *params_; }

  void AddClient(WorkletModuleResponsesMap::Client* client) {
    // Clients can be added only while a module script is being fetched.
    DCHECK(state_ == State::kInitial || state_ == State::kFetching);
    clients_.push_back(client);
  }

  // Implements ModuleScriptFetcher::Client.
  //
  // Implementation of the second half of the custom fetch defined in the
  // "fetch a worklet script" algorithm:
  // https://drafts.css-houdini.org/worklets/#fetch-a-worklet-script
  void NotifyFetchFinished(
      const base::Optional<ModuleScriptCreationParams>& params,
      const HeapVector<Member<ConsoleMessage>>& error_messages) override {
    // The entry can be disposed of during the resource fetch.
    if (state_ == State::kFailed)
      return;

    if (!params) {
      // TODO(nhiroki): Add |error_messages| to the context's message storage.
      NotifyFailure();
      return;
    }

    AdvanceState(State::kFetched);

    // Step 7: "Let response be the result of fetch when it asynchronously
    // completes."
    // Step 8: "Set the value of the entry in cache whose key is url to
    // response, and asynchronously complete this algorithm with response."
    params_.emplace(*params);
    for (WorkletModuleResponsesMap::Client* client : clients_)
      client->OnFetched(*params);
    clients_.clear();
    module_fetcher_.Clear();
  }

  void NotifyFailure() {
    AdvanceState(State::kFailed);
    for (WorkletModuleResponsesMap::Client* client : clients_)
      client->OnFailed();
    clients_.clear();
    module_fetcher_.Clear();
  }

  void Trace(blink::Visitor* visitor) override {
    visitor->Trace(module_fetcher_);
    visitor->Trace(clients_);
  }

 private:
  void AdvanceState(State new_state) {
    switch (state_) {
      case State::kInitial:
        DCHECK_EQ(new_state, State::kFetching);
        break;
      case State::kFetching:
        DCHECK(new_state == State::kFetched || new_state == State::kFailed);
        break;
      case State::kFetched:
      case State::kFailed:
        NOTREACHED();
        break;
    }
    state_ = new_state;
  }

  State state_ = State::kInitial;

  Member<DocumentModuleScriptFetcher> module_fetcher_;

  base::Optional<ModuleScriptCreationParams> params_;
  HeapVector<Member<WorkletModuleResponsesMap::Client>> clients_;
};

WorkletModuleResponsesMap::WorkletModuleResponsesMap(ResourceFetcher* fetcher)
    : fetcher_(fetcher) {}

// Implementation of the first half of the custom fetch defined in the
// "fetch a worklet script" algorithm:
// https://drafts.css-houdini.org/worklets/#fetch-a-worklet-script
//
// "To perform the fetch given request, perform the following steps:"
// Step 1: "Let cache be the moduleResponsesMap."
// Step 2: "Let url be request's url."
void WorkletModuleResponsesMap::Fetch(FetchParameters& fetch_params,
                                      Client* client) {
  DCHECK(IsMainThread());
  if (!is_available_ || !IsValidURL(fetch_params.Url())) {
    client->OnFailed();
    return;
  }

  auto it = entries_.find(fetch_params.Url());
  if (it != entries_.end()) {
    Entry* entry = it->value;
    switch (entry->GetState()) {
      case Entry::State::kInitial:
        NOTREACHED();
        return;
      case Entry::State::kFetching:
        // Step 3: "If cache contains an entry with key url whose value is
        // "fetching", wait until that entry's value changes, then proceed to
        // the next step."
        entry->AddClient(client);
        return;
      case Entry::State::kFetched:
        // Step 4: "If cache contains an entry with key url, asynchronously
        // complete this algorithm with that entry's value, and abort these
        // steps."
        client->OnFetched(entry->GetParams());
        return;
      case Entry::State::kFailed:
        // Module fetching failed before. Abort following steps.
        client->OnFailed();
        return;
    }
    NOTREACHED();
  }

  // Step 5: "Create an entry in cache with key url and value "fetching"."
  Entry* entry = new Entry;
  entry->AddClient(client);
  entries_.insert(fetch_params.Url(), entry);

  // Step 6: "Fetch request."
  // Running the callback with an empty params will make the fetcher to fallback
  // to regular module loading and Write() will be called once the fetch is
  // complete.
  entry->Fetch(fetch_params, fetcher_.Get());
}

void WorkletModuleResponsesMap::Invalidate(const KURL& url) {
  DCHECK(IsMainThread());
  DCHECK(IsValidURL(url));
  if (!is_available_)
    return;

  DCHECK(entries_.Contains(url));
  Entry* entry = entries_.find(url)->value;
  entry->NotifyFailure();
}

void WorkletModuleResponsesMap::Dispose() {
  is_available_ = false;
  for (auto it : entries_) {
    switch (it.value->GetState()) {
      case Entry::State::kInitial:
        NOTREACHED();
        break;
      case Entry::State::kFetching:
        it.value->NotifyFailure();
        break;
      case Entry::State::kFetched:
      case Entry::State::kFailed:
        break;
    }
  }
  entries_.clear();
}

void WorkletModuleResponsesMap::Trace(blink::Visitor* visitor) {
  visitor->Trace(fetcher_);
  visitor->Trace(entries_);
}

}  // namespace blink
