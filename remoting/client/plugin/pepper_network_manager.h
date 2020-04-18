// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_CLIENT_PLUGIN_PEPPER_NETWORK_MANAGER_H_
#define REMOTING_CLIENT_PLUGIN_PEPPER_NETWORK_MANAGER_H_

#include <stdint.h>

#include "base/callback.h"
#include "base/compiler_specific.h"
#include "base/memory/weak_ptr.h"
#include "ppapi/cpp/instance_handle.h"
#include "ppapi/cpp/network_monitor.h"
#include "ppapi/utility/completion_callback_factory.h"
#include "third_party/webrtc/rtc_base/network.h"

namespace pp {
class NetworkList;
}  // namespace pp

namespace remoting {

// PepperNetworkManager uses the PPB_NetworkMonitor API to
// implement the NetworkManager interface that libjingle uses to
// monitor the host system's network interfaces.
class PepperNetworkManager : public rtc::NetworkManagerBase {
 public:
  typedef base::RepeatingCallback<void()> NetworkInfoCallback;

  PepperNetworkManager(const pp::InstanceHandle& instance);
  ~PepperNetworkManager() override;

  // NetworkManager interface.
  void StartUpdating() override;
  void StopUpdating() override;

  void set_network_info_callback(NetworkInfoCallback callback) {
    network_info_callback_ = callback;
  }

 private:
  static void OnNetworkListCallbackHandler(void* user_data,
                                           PP_Resource list_resource);

  void OnNetworkList(int32_t result, const pp::NetworkList& list);

  void SendNetworksChangedSignal();

  pp::NetworkMonitor monitor_;
  int start_count_;
  bool network_list_received_;
  NetworkInfoCallback network_info_callback_;

  pp::CompletionCallbackFactory<PepperNetworkManager> callback_factory_;

  base::WeakPtrFactory<PepperNetworkManager> weak_factory_;
};

}  // namespace remoting

#endif  // REMOTING_CLIENT_PLUGIN_PEPPER_NETWORK_MANAGER_H_
