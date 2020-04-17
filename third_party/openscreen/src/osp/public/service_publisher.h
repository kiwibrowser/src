// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef OSP_PUBLIC_SERVICE_PUBLISHER_H_
#define OSP_PUBLIC_SERVICE_PUBLISHER_H_

#include <cstdint>
#include <string>
#include <vector>

#include "osp_base/macros.h"
#include "osp_base/time.h"
#include "platform/api/network_interface.h"

namespace openscreen {

// Used to report an error from a ServiceListener implementation.
struct ServicePublisherError {
  // TODO(mfoltz): Add additional error types, as implementations progress.
  enum class Code {
    kNone = 0,
  };

  ServicePublisherError();
  ServicePublisherError(Code error, const std::string& message);
  ServicePublisherError(const ServicePublisherError& other);
  ~ServicePublisherError();

  ServicePublisherError& operator=(const ServicePublisherError& other);

  Code error;
  std::string message;
};

class ServicePublisher {
 public:
  enum class State {
    kStopped = 0,
    kStarting,
    kRunning,
    kStopping,
    kSuspended,
  };

  struct Metrics {
    Metrics();
    ~Metrics();

    // The range of time over which the metrics were collected; end_timestamp >
    // start_timestamp
    timestamp_t start_timestamp = 0;
    timestamp_t end_timestamp = 0;

    // The number of packets and bytes sent since the service started.
    uint64_t num_packets_sent = 0;
    uint64_t num_bytes_sent = 0;

    // The number of packets and bytes received since the service started.
    uint64_t num_packets_received = 0;
    uint64_t num_bytes_received = 0;
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

    // Reports an error.
    virtual void OnError(ServicePublisherError) = 0;

    // Reports metrics.
    virtual void OnMetrics(Metrics) = 0;
  };

  struct Config {
    Config();
    ~Config();

    // The human readable friendly name of the service being published in
    // UTF-8.
    std::string friendly_name;

    // The DNS hostname (as a single label) that should be used to advertise the
    // host's interface addresses.
    std::string hostname;

    // The DNS domain name label that should be used to identify this service
    // within the openscreen service type.
    // TODO(btolsch): This could be derived from |friendly_name| but we will
    // leave it as an arbitrary name until the spec is finalized.
    std::string service_instance_name;

    // The port where openscreen connections are accepted.
    // Normally this should not be set, and must be identical to the port
    // configured in the ProtocolConnectionServer.
    uint16_t connection_server_port = 0;

    // A list of network interface names that the publisher should use.
    // By default, all enabled Ethernet and WiFi interfaces are used.
    // This configuration must be identical to the interfaces configured
    // in the ScreenConnectionServer.
    std::vector<platform::NetworkInterfaceIndex> network_interface_indices;
  };

  virtual ~ServicePublisher();

  // Starts publishing this service using the config object.
  // Returns true if state() == kStopped and the service will be started, false
  // otherwise.
  virtual bool Start() = 0;

  // Starts publishing this service, but then immediately suspends the
  // publisher. No announcements will be sent until Resume() is called. Returns
  // true if state() == kStopped and the service will be started, false
  // otherwise.
  virtual bool StartAndSuspend() = 0;

  // Stops publishing this service.
  // Returns true if state() != (kStopped|kStopping).
  virtual bool Stop() = 0;

  // Suspends publishing, for example, if the service is in a power saving
  // mode. Returns true if state() == (kRunning|kStarting), meaning the
  // suspension will take effect.
  virtual bool Suspend() = 0;

  // Resumes publishing.  Returns true if state() == kSuspended.
  virtual bool Resume() = 0;

  virtual void RunTasks() = 0;

  // Returns the current state of the publisher.
  State state() const { return state_; }

  // Returns the last error reported by this publisher.
  ServicePublisherError last_error() const { return last_error_; }

 protected:
  explicit ServicePublisher(Observer* observer);

  State state_ = State::kStopped;
  ServicePublisherError last_error_;
  Observer* observer_;

  OSP_DISALLOW_COPY_AND_ASSIGN(ServicePublisher);
};

}  // namespace openscreen

#endif  // OSP_PUBLIC_SERVICE_PUBLISHER_H_
