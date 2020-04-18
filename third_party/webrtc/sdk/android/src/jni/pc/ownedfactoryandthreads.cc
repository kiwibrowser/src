/*
 *  Copyright 2017 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "sdk/android/src/jni/pc/ownedfactoryandthreads.h"

#include "sdk/android/src/jni/jni_helpers.h"
#include "sdk/android/src/jni/pc/peerconnectionfactory.h"

namespace webrtc {
namespace jni {

PeerConnectionFactoryInterface* factoryFromJava(jlong j_p) {
  return reinterpret_cast<OwnedFactoryAndThreads*>(j_p)->factory();
}

OwnedFactoryAndThreads::~OwnedFactoryAndThreads() {
  factory_->Release();
  if (network_monitor_factory_ != nullptr) {
    rtc::NetworkMonitorFactory::ReleaseFactory(network_monitor_factory_);
  }
}

void OwnedFactoryAndThreads::InvokeJavaCallbacksOnFactoryThreads() {
  RTC_LOG(LS_INFO) << "InvokeJavaCallbacksOnFactoryThreads.";
  network_thread_->Invoke<void>(RTC_FROM_HERE,
                                &PeerConnectionFactoryNetworkThreadReady);
  worker_thread_->Invoke<void>(RTC_FROM_HERE,
                               &PeerConnectionFactoryWorkerThreadReady);
  signaling_thread_->Invoke<void>(RTC_FROM_HERE,
                                  &PeerConnectionFactorySignalingThreadReady);
}

}  // namespace jni
}  // namespace webrtc
