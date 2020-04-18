// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/router/issues_observer.h"

#include "base/logging.h"
#include "chrome/browser/media/router/issue_manager.h"

namespace media_router {

IssuesObserver::IssuesObserver(IssueManager* issue_manager)
    : issue_manager_(issue_manager), initialized_(false) {
  DCHECK(issue_manager_);
}

IssuesObserver::~IssuesObserver() {
  if (initialized_)
    issue_manager_->UnregisterObserver(this);
}

void IssuesObserver::Init() {
  if (initialized_)
    return;

  issue_manager_->RegisterObserver(this);
  initialized_ = true;
}

}  // namespace media_router
