// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef OSP_IMPL_SERVICE_PUBLISHER_IMPL_H_
#define OSP_IMPL_SERVICE_PUBLISHER_IMPL_H_

#include "osp/public/service_publisher.h"
#include "osp_base/macros.h"
#include "osp_base/with_destruction_callback.h"

namespace openscreen {

class ServicePublisherImpl final : public ServicePublisher,
                                   public WithDestructionCallback {
 public:
  class Delegate {
   public:
    Delegate();
    virtual ~Delegate();

    void SetPublisherImpl(ServicePublisherImpl* publisher);

    virtual void StartPublisher() = 0;
    virtual void StartAndSuspendPublisher() = 0;
    virtual void StopPublisher() = 0;
    virtual void SuspendPublisher() = 0;
    virtual void ResumePublisher() = 0;
    virtual void RunTasksPublisher() = 0;

   protected:
    void SetState(State state) { publisher_->SetState(state); }

    ServicePublisherImpl* publisher_ = nullptr;
  };

  // |observer| is optional.  If it is provided, it will receive appropriate
  // notifications about this ServicePublisher.  |delegate| is required and
  // is used to implement state transitions.
  ServicePublisherImpl(Observer* observer, Delegate* delegate);
  ~ServicePublisherImpl() override;

  // ServicePublisher overrides.
  bool Start() override;
  bool StartAndSuspend() override;
  bool Stop() override;
  bool Suspend() override;
  bool Resume() override;

  void RunTasks() override;

 private:
  // Called by |delegate_| to transition the state machine (except kStarting and
  // kStopping which are done automatically).
  void SetState(State state);

  // Notifies |observer_| if the transition to |state_| is one that is watched
  // by the observer interface.
  void MaybeNotifyObserver();

  Delegate* const delegate_;

  OSP_DISALLOW_COPY_AND_ASSIGN(ServicePublisherImpl);
};

}  // namespace openscreen

#endif  // OSP_IMPL_SERVICE_PUBLISHER_IMPL_H_
