/*
 *  Copyright (c) 2019 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef TEST_PC_E2E_TEST_PEER_H_
#define TEST_PC_E2E_TEST_PEER_H_

#include <memory>
#include <string>

#include "absl/memory/memory.h"
#include "api/array_view.h"
#include "media/base/media_engine.h"
#include "modules/audio_device/include/test_audio_device.h"
#include "pc/peer_connection_wrapper.h"
#include "pc/test/mock_peer_connection_observers.h"
#include "rtc_base/network.h"
#include "rtc_base/thread.h"
#include "test/pc/e2e/analyzer/video/encoded_image_id_injector.h"
#include "test/pc/e2e/analyzer/video/video_quality_analyzer_injection_helper.h"
#include "test/pc/e2e/api/peerconnection_quality_test_fixture.h"

namespace webrtc {
namespace test {

// Describes a single participant in the call.
class TestPeer final : public PeerConnectionWrapper {
 public:
  using PeerConnectionWrapper::PeerConnectionWrapper;
  using Params = PeerConnectionE2EQualityTestFixture::Params;
  using VideoConfig = PeerConnectionE2EQualityTestFixture::VideoConfig;
  using AudioConfig = PeerConnectionE2EQualityTestFixture::AudioConfig;
  using InjectableComponents =
      PeerConnectionE2EQualityTestFixture::InjectableComponents;

  // Setups all components, that should be provided to WebRTC
  // PeerConnectionFactory and PeerConnection creation methods,
  // also will setup dependencies, that are required for media analyzers
  // injection.
  //
  // We require |worker_thread| here, because TestPeer can't own worker thread,
  // because in such case it will be destroyed before peer connection.
  // |signaling_thread| will be provided by test fixture implementation.
  // |params| - describes current peer paramters, like current peer video
  // streams and audio streams
  // |audio_outpu_file_name| - the name of output file, where incoming audio
  // stream should be written. It should be provided from remote peer
  // |params.audio_config.output_file_name|
  static std::unique_ptr<TestPeer> CreateTestPeer(
      std::unique_ptr<InjectableComponents> components,
      std::unique_ptr<Params> params,
      VideoQualityAnalyzerInjectionHelper* video_analyzer_helper,
      rtc::Thread* signaling_thread,
      rtc::Thread* worker_thread,
      absl::optional<std::string> audio_output_file_name);

  Params* params() const { return params_.get(); }

  // Adds provided |candidates| to the owned peer connection.
  bool AddIceCandidates(
      rtc::ArrayView<const IceCandidateInterface* const> candidates);

 private:
  TestPeer(rtc::scoped_refptr<PeerConnectionFactoryInterface> pc_factory,
           rtc::scoped_refptr<PeerConnectionInterface> pc,
           std::unique_ptr<MockPeerConnectionObserver> observer,
           std::unique_ptr<Params> params,
           std::unique_ptr<rtc::NetworkManager> network_manager);

  std::unique_ptr<Params> params_;
  // Test peer will take ownership of network manager and keep it during the
  // call. Network manager will be deleted before peer connection, but
  // connection will be closed before destruction, so it should be ok.
  std::unique_ptr<rtc::NetworkManager> network_manager_;
};

}  // namespace test
}  // namespace webrtc

#endif  // TEST_PC_E2E_TEST_PEER_H_
