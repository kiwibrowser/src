// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// NetworkListManager interface is introduced to enable unit test on
// IpcNetworkManager such that it doesn't depend on implementation of
// P2PSocketDispatcher.

#ifndef CONTENT_RENDERER_P2P_NETWORK_LIST_MANAGER_H_
#define CONTENT_RENDERER_P2P_NETWORK_LIST_MANAGER_H_

namespace content {

class NetworkListObserver;

class CONTENT_EXPORT NetworkListManager {
 public:
  // Add a new network list observer. Each observer is called
  // immidiately after it is registered and then later whenever
  // network configuration changes. Can be called on any thread. The
  // observer is always called on the thread it was added.
  virtual void AddNetworkListObserver(
      NetworkListObserver* network_list_observer) = 0;

  // Removes network list observer. Must be called on the thread on
  // which the observer was added.
  virtual void RemoveNetworkListObserver(
      NetworkListObserver* network_list_observer) = 0;

 protected:
  // Marked as protected to prevent explicit deletion, as
  // P2PSocketDispatcher is not owned by IpcNetworkManager.
  virtual ~NetworkListManager() {}
};

}  // namespace content

#endif  // CONTENT_RENDERER_P2P_NETWORK_LIST_MANAGER_H_
