// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "headless/public/util/testing/fake_managed_dispatch_url_request_job.h"

#include "headless/public/util/url_request_dispatcher.h"

namespace headless {

void FakeManagedDispatchURLRequestJob::OnHeadersComplete() {
  notifications_->push_back(base::StringPrintf("id: %d OK", id_));
}

void FakeManagedDispatchURLRequestJob::OnStartError(net::Error error) {
  notifications_->push_back(base::StringPrintf("id: %d err: %d", id_, error));
}

void FakeManagedDispatchURLRequestJob::Kill() {
  url_request_dispatcher_->JobKilled(this);
}

}  // namespace headless
