// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DOWNLOAD_INTERNAL_BACKGROUND_SERVICE_SCHEDULER_NETWORK_STATUS_LISTENER_H_
#define COMPONENTS_DOWNLOAD_INTERNAL_BACKGROUND_SERVICE_SCHEDULER_NETWORK_STATUS_LISTENER_H_

#include "net/base/network_change_notifier.h"

namespace download {

// Monitor and propagate network status change events.
// Base class only manages the observer pointer, derived class should override
// to provide actual network hook to monitor the changes, and call base class
// virtual functions.
class NetworkStatusListener {
 public:
  // Observer to receive network connection type change notifications.
  class Observer {
   public:
    virtual void OnNetworkChanged(
        net::NetworkChangeNotifier::ConnectionType type) = 0;

   protected:
    virtual ~Observer() {}
  };

  // Starts to listen to network changes.
  virtual void Start(Observer* observer);

  // Stops to listen to network changes.
  virtual void Stop();

  // Gets the current connection type.
  virtual net::NetworkChangeNotifier::ConnectionType GetConnectionType() = 0;

  virtual ~NetworkStatusListener() {}

 protected:
  NetworkStatusListener();

  // The only observer that listens to connection type change.
  Observer* observer_ = nullptr;

  // The current network status.
  net::NetworkChangeNotifier::ConnectionType network_status_ =
      net::NetworkChangeNotifier::ConnectionType::CONNECTION_NONE;

 private:
  DISALLOW_COPY_AND_ASSIGN(NetworkStatusListener);
};

// Default implementation of NetworkStatusListener using
// net::NetworkChangeNotifier to listen to connectivity changes.
class NetworkStatusListenerImpl
    : public net::NetworkChangeNotifier::NetworkChangeObserver,
      public NetworkStatusListener {
 public:
  NetworkStatusListenerImpl();
  ~NetworkStatusListenerImpl() override;

  // NetworkStatusListener implementation.
  void Start(NetworkStatusListener::Observer* observer) override;
  void Stop() override;
  net::NetworkChangeNotifier::ConnectionType GetConnectionType() override;

 private:
  // net::NetworkChangeNotifier::NetworkChangeObserver implementation.
  void OnNetworkChanged(
      net::NetworkChangeNotifier::ConnectionType type) override;

  DISALLOW_COPY_AND_ASSIGN(NetworkStatusListenerImpl);
};

}  // namespace download

#endif  // COMPONENTS_DOWNLOAD_INTERNAL_BACKGROUND_SERVICE_SCHEDULER_NETWORK_STATUS_LISTENER_H_
