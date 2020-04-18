// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/omnibox/browser/history_index_restore_observer.h"

HistoryIndexRestoreObserver::HistoryIndexRestoreObserver(
    const base::Closure& task)
    : task_(task),
      succeeded_(false) {
}

HistoryIndexRestoreObserver::~HistoryIndexRestoreObserver() {}

void HistoryIndexRestoreObserver::OnCacheRestoreFinished(bool success) {
  succeeded_ = success;
  task_.Run();
}
