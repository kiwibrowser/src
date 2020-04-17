// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef OSP_IMPL_SERVICE_LISTENER_IMPL_H_
#define OSP_IMPL_SERVICE_LISTENER_IMPL_H_

#include <vector>

#include "osp/impl/receiver_list.h"
#include "osp/public/service_info.h"
#include "osp/public/service_listener.h"
#include "osp_base/macros.h"
#include "osp_base/with_destruction_callback.h"

namespace openscreen {

class ServiceListenerImpl final : public ServiceListener,
                                  public WithDestructionCallback {
 public:
  class Delegate {
   public:
    Delegate();
    virtual ~Delegate();

    void SetListenerImpl(ServiceListenerImpl* listener);

    virtual void StartListener() = 0;
    virtual void StartAndSuspendListener() = 0;
    virtual void StopListener() = 0;
    virtual void SuspendListener() = 0;
    virtual void ResumeListener() = 0;
    virtual void SearchNow(State from) = 0;
    virtual void RunTasksListener() = 0;

   protected:
    void SetState(State state) { listener_->SetState(state); }

    ServiceListenerImpl* listener_ = nullptr;
  };

  // |delegate| is used to implement state transitions.
  explicit ServiceListenerImpl(Delegate* delegate);
  ~ServiceListenerImpl() override;

  // Called by |delegate_| when there are updates to the available receivers.
  void OnReceiverAdded(const ServiceInfo& info);
  void OnReceiverChanged(const ServiceInfo& info);
  void OnReceiverRemoved(const ServiceInfo& info);
  void OnAllReceiversRemoved();

  // Called by |delegate_| when an internal error occurs.
  void OnError(ServiceListenerError error);

  // ServiceListener overrides.
  bool Start() override;
  bool StartAndSuspend() override;
  bool Stop() override;
  bool Suspend() override;
  bool Resume() override;
  bool SearchNow() override;

  void RunTasks() override;

  void AddObserver(Observer* observer) override;
  void RemoveObserver(Observer* observer) override;

  const std::vector<ServiceInfo>& GetReceivers() const override;

 private:
  // Called by |delegate_| to transition the state machine (except kStarting and
  // kStopping which are done automatically).
  void SetState(State state);

  // Notifies each observer in |observers_| if the transition to |state_| is one
  // that is watched by the observer interface.
  void MaybeNotifyObservers();

  Delegate* const delegate_;
  ReceiverList receiver_list_;

  OSP_DISALLOW_COPY_AND_ASSIGN(ServiceListenerImpl);
};

}  // namespace openscreen

#endif  // OSP_IMPL_SERVICE_LISTENER_IMPL_H_
