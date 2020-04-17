// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef OSP_PUBLIC_SERVICE_LISTENER_H_
#define OSP_PUBLIC_SERVICE_LISTENER_H_

#include <cstdint>
#include <string>
#include <vector>

#include "osp/public/service_info.h"
#include "osp_base/macros.h"
#include "osp_base/time.h"

namespace openscreen {

// Used to report an error from a ServiceListener implementation.
struct ServiceListenerError {
 public:
  // TODO(mfoltz): Add additional error types, as implementations progress.
  enum class Code {
    kNone = 0,
  };

  ServiceListenerError();
  ServiceListenerError(Code error, const std::string& message);
  ServiceListenerError(const ServiceListenerError& other);
  ~ServiceListenerError();

  ServiceListenerError& operator=(const ServiceListenerError& other);

  Code error;
  std::string message;
};

class ServiceListener {
 public:
  enum class State {
    kStopped = 0,
    kStarting,
    kRunning,
    kStopping,
    kSearching,
    kSuspended,
  };

  // Holds a set of metrics, captured over a specific range of time, about the
  // behavior of a ServiceListener instance.
  struct Metrics {
    Metrics();
    ~Metrics();

    // The range of time over which the metrics were collected; end_timestamp >
    // start_timestamp
    timestamp_t start_timestamp = 0;
    timestamp_t end_timestamp = 0;

    // The number of packets and bytes sent over the timestamp range.
    uint64_t num_packets_sent = 0;
    uint64_t num_bytes_sent = 0;

    // The number of packets and bytes received over the timestamp range.
    uint64_t num_packets_received = 0;
    uint64_t num_bytes_received = 0;

    // The maximum number of receivers discovered over the timestamp range.  The
    // latter two fields break this down by receivers advertising ipv4 and ipv6
    // endpoints.
    size_t num_receivers = 0;
    size_t num_ipv4_receivers = 0;
    size_t num_ipv6_receivers = 0;
  };

  class Observer {
   public:
    virtual ~Observer() = default;

    // Called when the state becomes kRunning.
    virtual void OnStarted() = 0;
    // Called when the state becomes kStopped.
    virtual void OnStopped() = 0;
    // Called when the state becomes kSuspended.
    virtual void OnSuspended() = 0;
    // Called when the state becomes kSearching.
    virtual void OnSearching() = 0;

    // Notifications to changes to the listener's receiver list.
    virtual void OnReceiverAdded(const ServiceInfo&) = 0;
    virtual void OnReceiverChanged(const ServiceInfo&) = 0;
    virtual void OnReceiverRemoved(const ServiceInfo&) = 0;
    // Called if all receivers are no longer available, e.g. all network
    // interfaces have been disabled.
    virtual void OnAllReceiversRemoved() = 0;

    // Reports an error.
    virtual void OnError(ServiceListenerError) = 0;

    // Reports metrics.
    virtual void OnMetrics(Metrics) = 0;
  };

  virtual ~ServiceListener();

  // Starts listening for receivers using the config object.
  // Returns true if state() == kStopped and the service will be started, false
  // otherwise.
  virtual bool Start() = 0;

  // Starts the listener in kSuspended mode.  This could be used to enable
  // immediate search via SearchNow() in the future.
  // Returns true if state() == kStopped and the service will be started, false
  // otherwise.
  virtual bool StartAndSuspend() = 0;

  // Stops listening and cancels any search in progress.
  // Returns true if state() != (kStopped|kStopping).
  virtual bool Stop() = 0;

  // Suspends background listening. For example, the tab wanting receiver
  // availability might go in the background, meaning we can suspend listening
  // to save power.
  // Returns true if state() == (kRunning|kSearching|kStarting), meaning the
  // suspension will take effect.
  virtual bool Suspend() = 0;

  // Resumes listening.  Returns true if state() == (kSuspended|kSearching).
  virtual bool Resume() = 0;

  // Asks the listener to search for receivers now, even if the listener is
  // is currently suspended.  If a background search is already in
  // progress, this has no effect.  Returns true if state() ==
  // (kRunning|kSuspended).
  virtual bool SearchNow() = 0;

  virtual void RunTasks() = 0;

  virtual void AddObserver(Observer* observer) = 0;
  virtual void RemoveObserver(Observer* observer) = 0;

  // Returns the current state of the listener.
  State state() const { return state_; }

  // Returns the last error reported by this listener.
  const ServiceListenerError& last_error() const { return last_error_; }

  // Returns the current list of receivers known to the ServiceListener.
  virtual const std::vector<ServiceInfo>& GetReceivers() const = 0;

 protected:
  ServiceListener();

  State state_ = State::kStopped;
  ServiceListenerError last_error_;
  std::vector<Observer*> observers_;

  OSP_DISALLOW_COPY_AND_ASSIGN(ServiceListener);
};

}  // namespace openscreen

#endif  // OSP_PUBLIC_SERVICE_LISTENER_H_
