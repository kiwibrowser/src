// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/service_worker/thread_safe_script_container.h"

#include "base/stl_util.h"

namespace content {

ThreadSafeScriptContainer::ThreadSafeScriptContainer()
    : lock_(), waiting_cv_(&lock_), are_all_data_added_(false) {}

void ThreadSafeScriptContainer::AddOnIOThread(const GURL& url,
                                              std::unique_ptr<Data> data) {
  base::AutoLock lock(lock_);
  script_data_[url] = std::move(data);
  if (url == waiting_url_)
    waiting_cv_.Signal();
}

ThreadSafeScriptContainer::ScriptStatus
ThreadSafeScriptContainer::GetStatusOnWorkerThread(const GURL& url) {
  base::AutoLock lock(lock_);
  auto it = script_data_.find(url);
  if (it == script_data_.end())
    return ScriptStatus::kPending;
  if (!it->second)
    return ScriptStatus::kTaken;
  if (!it->second->IsValid())
    return ScriptStatus::kFailed;
  return ScriptStatus::kReceived;
}

void ThreadSafeScriptContainer::ResetOnWorkerThread(const GURL& url) {
  base::AutoLock lock(lock_);
  script_data_.erase(url);
}

bool ThreadSafeScriptContainer::WaitOnWorkerThread(const GURL& url) {
  base::AutoLock lock(lock_);
  DCHECK(waiting_url_.is_empty())
      << "The script container is unexpectedly shared among worker threads.";
  waiting_url_ = url;
  while (script_data_.find(url) == script_data_.end()) {
    // If waiting script hasn't been added yet though all data are received,
    // that means something went wrong.
    if (are_all_data_added_) {
      waiting_url_ = GURL();
      return false;
    }
    // This is possible to be waken up spuriously, so that it's necessary to
    // check if the entry is really added.
    waiting_cv_.Wait();
  }
  waiting_url_ = GURL();
  // TODO(shimazu): Keep the status for each entries instead of using IsValid().
  const auto& data = script_data_[url];
  return !data || data->IsValid();
}

std::unique_ptr<ThreadSafeScriptContainer::Data>
ThreadSafeScriptContainer::TakeOnWorkerThread(const GURL& url) {
  base::AutoLock lock(lock_);
  DCHECK(base::ContainsKey(script_data_, url))
      << "Script should be added before calling Take.";
  return std::move(script_data_[url]);
}

void ThreadSafeScriptContainer::OnAllDataAddedOnIOThread() {
  base::AutoLock lock(lock_);
  are_all_data_added_ = true;
  waiting_cv_.Broadcast();
}

ThreadSafeScriptContainer::~ThreadSafeScriptContainer() = default;

}  // namespace content
