// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_P2P_EMPTY_NETWORK_MANAGER_H_
#define CONTENT_RENDERER_P2P_EMPTY_NETWORK_MANAGER_H_

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/threading/thread_checker.h"
#include "content/common/content_export.h"
#include "third_party/webrtc/rtc_base/network.h"
#include "third_party/webrtc/rtc_base/sigslot.h"

namespace rtc {
class IPAddress;
}  // namespace rtc

namespace content {

// A NetworkManager implementation which handles the case where local address
// enumeration is not requested and just returns empty network lists. This class
// is not thread safe and should only be used by WebRTC's network thread.
class EmptyNetworkManager : public rtc::NetworkManagerBase,
                            public sigslot::has_slots<> {
 public:
  // This class is created by WebRTC's signaling thread but used by WebRTC's
  // worker thread |task_runner|.
  CONTENT_EXPORT explicit EmptyNetworkManager(
      rtc::NetworkManager* network_manager);
  CONTENT_EXPORT ~EmptyNetworkManager() override;

  // rtc::NetworkManager:
  void StartUpdating() override;
  void StopUpdating() override;
  void GetNetworks(NetworkList* networks) const override;
  bool GetDefaultLocalAddress(int family,
                              rtc::IPAddress* ipaddress) const override;

 private:
  // Receive callback from the wrapped NetworkManager when the underneath
  // network list is changed.
  //
  // We wait for this so that we don't signal networks changed before we have
  // default IP addresses.
  void OnNetworksChanged();

  base::ThreadChecker thread_checker_;

  // Whether we have fired the first SignalNetworksChanged.
  // Used to ensure we only report metrics once.
  bool sent_first_update_ = false;

  // SignalNetworksChanged will only be fired if there is any outstanding
  // StartUpdating.
  int start_count_ = 0;

  // |network_manager_| is just a reference, owned by
  // PeerConnectionDependencyFactory.
  rtc::NetworkManager* network_manager_;

  base::WeakPtrFactory<EmptyNetworkManager> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(EmptyNetworkManager);
};

}  // namespace content

#endif  // CONTENT_RENDERER_P2P_EMPTY_NETWORK_MANAGER_H_
