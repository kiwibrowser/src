// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/background/save_page_request.h"

namespace offline_pages {

SavePageRequest::SavePageRequest(int64_t request_id,
                                 const GURL& url,
                                 const ClientId& client_id,
                                 const base::Time& creation_time,
                                 const bool user_requested)
    : request_id_(request_id),
      url_(url),
      client_id_(client_id),
      creation_time_(creation_time),
      started_attempt_count_(0),
      completed_attempt_count_(0),
      user_requested_(user_requested),
      state_(RequestState::AVAILABLE),
      fail_state_(FailState::NO_FAILURE),
      pending_state_(PendingState::NOT_PENDING) {}

SavePageRequest::SavePageRequest(const SavePageRequest& other)
    : request_id_(other.request_id_),
      url_(other.url_),
      client_id_(other.client_id_),
      creation_time_(other.creation_time_),
      started_attempt_count_(other.started_attempt_count_),
      completed_attempt_count_(other.completed_attempt_count_),
      last_attempt_time_(other.last_attempt_time_),
      user_requested_(other.user_requested_),
      state_(other.state_),
      fail_state_(other.fail_state_),
      pending_state_(other.pending_state_),
      original_url_(other.original_url_),
      request_origin_(other.request_origin_) {}

SavePageRequest::~SavePageRequest() {}

bool SavePageRequest::operator==(const SavePageRequest& other) const {
  return request_id_ == other.request_id_ && url_ == other.url_ &&
         client_id_ == other.client_id_ &&
         creation_time_ == other.creation_time_ &&
         started_attempt_count_ == other.started_attempt_count_ &&
         completed_attempt_count_ == other.completed_attempt_count_ &&
         last_attempt_time_ == other.last_attempt_time_ &&
         state_ == other.state_ && original_url_ == other.original_url_ &&
         request_origin_ == other.request_origin_;
}

void SavePageRequest::MarkAttemptStarted(const base::Time& start_time) {
  last_attempt_time_ = start_time;
  ++started_attempt_count_;
  state_ = RequestState::OFFLINING;
}

void SavePageRequest::MarkAttemptCompleted(FailState fail_state) {
  ++completed_attempt_count_;
  state_ = RequestState::AVAILABLE;
  UpdateFailState(fail_state);
}

void SavePageRequest::MarkAttemptAborted() {
  // We intentinally do not increment the completed_attempt_count_, since this
  // was killed before it completed, so we could use the phone or browser for
  // other things.
  if (state_ == RequestState::OFFLINING) {
    DCHECK_GT(started_attempt_count_, 0);
    state_ = RequestState::AVAILABLE;
  }
}

void SavePageRequest::MarkAttemptPaused() {
  state_ = RequestState::PAUSED;
}

void SavePageRequest::UpdateFailState(FailState fail_state) {
  // The order of precedence for failure errors related to offline page
  // downloads is as follows: NO_FAILURE, CANNOT_DOWNLOAD and
  // NETWORK_INSTABILITY.
  switch (fail_state) {
    case FailState::NO_FAILURE:  // Intentional fallthrough.
    case FailState::CANNOT_DOWNLOAD:
      fail_state_ = fail_state;
      break;
    case FailState::NETWORK_INSTABILITY:
      if (fail_state_ != FailState::CANNOT_DOWNLOAD) {
        fail_state_ = fail_state;
      }
      break;
  }
}

}  // namespace offline_pages
