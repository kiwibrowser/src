// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/browsing_data/conditional_cache_deletion_helper.h"

#include "base/callback.h"
#include "base/location.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "content/public/browser/browser_thread.h"

namespace {

bool EntryPredicateFromURLsAndTime(
    const base::Callback<bool(const GURL&)>& url_predicate,
    const base::Time& begin_time,
    const base::Time& end_time,
    const disk_cache::Entry* entry) {
  return (entry->GetLastUsed() >= begin_time &&
          entry->GetLastUsed() < end_time &&
          url_predicate.Run(GURL(entry->GetKey())));
}

}  // namespace

namespace content {

ConditionalCacheDeletionHelper::ConditionalCacheDeletionHelper(
    disk_cache::Backend* cache,
    const base::Callback<bool(const disk_cache::Entry*)>& condition)
    : cache_(cache),
      condition_(condition),
      current_entry_(nullptr),
      previous_entry_(nullptr) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
}

// static
base::Callback<bool(const disk_cache::Entry*)>
ConditionalCacheDeletionHelper::CreateURLAndTimeCondition(
    const base::Callback<bool(const GURL&)>& url_predicate,
    const base::Time& begin_time,
    const base::Time& end_time) {
  return base::Bind(&EntryPredicateFromURLsAndTime, url_predicate,
                    begin_time.is_null() ? base::Time() : begin_time,
                    end_time.is_null() ? base::Time::Max() : end_time);
}

int ConditionalCacheDeletionHelper::DeleteAndDestroySelfWhenFinished(
    const net::CompletionCallback& completion_callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  completion_callback_ = completion_callback;
  iterator_ = cache_->CreateIterator();

  IterateOverEntries(net::OK);
  return net::ERR_IO_PENDING;
}

ConditionalCacheDeletionHelper::~ConditionalCacheDeletionHelper() {}

void ConditionalCacheDeletionHelper::IterateOverEntries(int error) {
  while (error != net::ERR_IO_PENDING) {
    // If the entry obtained in the previous iteration matches the condition,
    // mark it for deletion. The iterator is already one step forward, so it
    // won't be invalidated. Always close the previous entry so it does not
    // leak.
    if (previous_entry_) {
      if (condition_.Run(previous_entry_))
        previous_entry_->Doom();
      previous_entry_->Close();
    }

    if (error == net::ERR_FAILED) {
      // The iteration finished successfully or we can no longer iterate
      // (e.g. the cache was destroyed). We cannot distinguish between the two,
      // but we know that there is nothing more that we can do, so we return OK.
      base::ThreadTaskRunnerHandle::Get()->PostTask(
          FROM_HERE, base::BindOnce(completion_callback_, net::OK));
      base::ThreadTaskRunnerHandle::Get()->DeleteSoon(FROM_HERE, this);
      return;
    }

    previous_entry_ = current_entry_;
    error = iterator_->OpenNextEntry(
        &current_entry_,
        base::Bind(&ConditionalCacheDeletionHelper::IterateOverEntries,
                   base::Unretained(this)));
  }
}

}  // namespace content
