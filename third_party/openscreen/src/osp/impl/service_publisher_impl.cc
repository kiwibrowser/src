// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "osp/impl/service_publisher_impl.h"

#include "platform/api/logging.h"

namespace openscreen {
namespace {

bool IsTransitionValid(ServicePublisher::State from,
                       ServicePublisher::State to) {
  using State = ServicePublisher::State;
  switch (from) {
    case State::kStopped:
      return to == State::kStarting || to == State::kStopping;
    case State::kStarting:
      return to == State::kRunning || to == State::kStopping ||
             to == State::kSuspended;
    case State::kRunning:
      return to == State::kSuspended || to == State::kStopping;
    case State::kStopping:
      return to == State::kStopped;
    case State::kSuspended:
      return to == State::kRunning || to == State::kStopping;
    default:
      OSP_DCHECK(false) << "unknown State value: " << static_cast<int>(from);
      break;
  }
  return false;
}

}  // namespace

ServicePublisherImpl::Delegate::Delegate() = default;
ServicePublisherImpl::Delegate::~Delegate() = default;

void ServicePublisherImpl::Delegate::SetPublisherImpl(
    ServicePublisherImpl* publisher) {
  OSP_DCHECK(!publisher_);
  publisher_ = publisher;
}

ServicePublisherImpl::ServicePublisherImpl(Observer* observer,
                                           Delegate* delegate)
    : ServicePublisher(observer), delegate_(delegate) {
  delegate_->SetPublisherImpl(this);
}

ServicePublisherImpl::~ServicePublisherImpl() = default;

bool ServicePublisherImpl::Start() {
  if (state_ != State::kStopped)
    return false;
  state_ = State::kStarting;
  delegate_->StartPublisher();
  return true;
}
bool ServicePublisherImpl::StartAndSuspend() {
  if (state_ != State::kStopped)
    return false;
  state_ = State::kStarting;
  delegate_->StartAndSuspendPublisher();
  return true;
}
bool ServicePublisherImpl::Stop() {
  if (state_ == State::kStopped || state_ == State::kStopping)
    return false;

  state_ = State::kStopping;
  delegate_->StopPublisher();
  return true;
}
bool ServicePublisherImpl::Suspend() {
  if (state_ != State::kRunning && state_ != State::kStarting)
    return false;

  delegate_->SuspendPublisher();
  return true;
}
bool ServicePublisherImpl::Resume() {
  if (state_ != State::kSuspended)
    return false;

  delegate_->ResumePublisher();
  return true;
}

void ServicePublisherImpl::RunTasks() {
  delegate_->RunTasksPublisher();
}

void ServicePublisherImpl::SetState(State state) {
  OSP_DCHECK(IsTransitionValid(state_, state));
  state_ = state;
  if (observer_)
    MaybeNotifyObserver();
}

void ServicePublisherImpl::MaybeNotifyObserver() {
  OSP_DCHECK(observer_);
  switch (state_) {
    case State::kRunning:
      observer_->OnStarted();
      break;
    case State::kStopped:
      observer_->OnStopped();
      break;
    case State::kSuspended:
      observer_->OnSuspended();
      break;
    default:
      break;
  }
}

}  // namespace openscreen
