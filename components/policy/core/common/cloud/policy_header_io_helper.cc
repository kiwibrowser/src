// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/policy/core/common/cloud/policy_header_io_helper.h"

#include "base/bind.h"
#include "base/location.h"
#include "base/sequenced_task_runner.h"
#include "components/policy/core/common/cloud/cloud_policy_constants.h"
#include "net/url_request/url_request.h"

namespace policy {

PolicyHeaderIOHelper::PolicyHeaderIOHelper(
    const std::string& server_url,
    const std::string& initial_header_value,
    const scoped_refptr<base::SequencedTaskRunner>& task_runner)
    : server_url_(server_url),
      io_task_runner_(task_runner),
      policy_header_(initial_header_value) {
}

PolicyHeaderIOHelper::~PolicyHeaderIOHelper() {
}

// Sets any necessary policy headers on the passed request.
void PolicyHeaderIOHelper::AddPolicyHeaders(const GURL& url,
                                            net::URLRequest* request) const {
  DCHECK(io_task_runner_->RunsTasksInCurrentSequence());
  if (!policy_header_.empty() &&
      url.spec().compare(0, server_url_.size(), server_url_) == 0) {
    request->SetExtraRequestHeaderByName(kChromePolicyHeader,
                                         policy_header_,
                                         true /* overwrite */);
  }
}

void PolicyHeaderIOHelper::UpdateHeader(const std::string& new_header) {
  // Post a task to the IO thread to modify this.
  io_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&PolicyHeaderIOHelper::UpdateHeaderOnIOThread,
                 base::Unretained(this), new_header));
}

void PolicyHeaderIOHelper::UpdateHeaderOnIOThread(
    const std::string& new_header) {
  DCHECK(io_task_runner_->RunsTasksInCurrentSequence());
  policy_header_ = new_header;
}

void PolicyHeaderIOHelper::SetServerURLForTest(const std::string& server_url) {
  io_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&PolicyHeaderIOHelper::SetServerURLOnIOThread,
                 base::Unretained(this), server_url));
}

void PolicyHeaderIOHelper::SetServerURLOnIOThread(
    const std::string& server_url) {
  DCHECK(io_task_runner_->RunsTasksInCurrentSequence());
  server_url_ = server_url;
}

}  // namespace policy
