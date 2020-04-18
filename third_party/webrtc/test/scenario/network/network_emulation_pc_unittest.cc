/*
 *  Copyright 2019 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include <cstdint>
#include <memory>

#include "absl/memory/memory.h"
#include "api/audio_codecs/builtin_audio_decoder_factory.h"
#include "api/audio_codecs/builtin_audio_encoder_factory.h"
#include "api/call/call_factory_interface.h"
#include "api/peer_connection_interface.h"
#include "api/scoped_refptr.h"
#include "api/video_codecs/builtin_video_decoder_factory.h"
#include "api/video_codecs/builtin_video_encoder_factory.h"
#include "call/simulated_network.h"
#include "logging/rtc_event_log/rtc_event_log_factory.h"
#include "media/engine/webrtc_media_engine.h"
#include "modules/audio_device/include/test_audio_device.h"
#include "p2p/client/basic_port_allocator.h"
#include "pc/peer_connection_wrapper.h"
#include "pc/test/mock_peer_connection_observers.h"
#include "rtc_base/async_invoker.h"
#include "rtc_base/fake_network.h"
#include "rtc_base/gunit.h"
#include "test/gmock.h"
#include "test/gtest.h"
#include "test/scenario/network/network_emulation.h"
#include "test/scenario/network/network_emulation_manager.h"

namespace webrtc {
namespace test {
namespace {

constexpr int kDefaultTimeoutMs = 1000;
constexpr int kMaxAptitude = 32000;
constexpr int kSamplingFrequency = 48000;
constexpr char kSignalThreadName[] = "signaling_thread";

bool AddIceCandidates(PeerConnectionWrapper* peer,
                      std::vector<const IceCandidateInterface*> candidates) {
  bool success = true;
  for (const auto candidate : candidates) {
    if (!peer->pc()->AddIceCandidate(candidate)) {
      success = false;
    }
  }
  return success;
}

rtc::scoped_refptr<PeerConnectionFactoryInterface> CreatePeerConnectionFactory(
    rtc::Thread* signaling_thread,
    rtc::Thread* network_thread) {
  PeerConnectionFactoryDependencies pcf_deps;
  pcf_deps.call_factory = webrtc::CreateCallFactory();
  pcf_deps.event_log_factory = webrtc::CreateRtcEventLogFactory();
  pcf_deps.network_thread = network_thread;
  pcf_deps.signaling_thread = signaling_thread;
  pcf_deps.media_engine = cricket::WebRtcMediaEngineFactory::Create(
      TestAudioDeviceModule::CreateTestAudioDeviceModule(
          TestAudioDeviceModule::CreatePulsedNoiseCapturer(kMaxAptitude,
                                                           kSamplingFrequency),
          TestAudioDeviceModule::CreateDiscardRenderer(kSamplingFrequency)),
      webrtc::CreateBuiltinAudioEncoderFactory(),
      webrtc::CreateBuiltinAudioDecoderFactory(),
      webrtc::CreateBuiltinVideoEncoderFactory(),
      webrtc::CreateBuiltinVideoDecoderFactory(), /*audio_mixer=*/nullptr,
      webrtc::AudioProcessingBuilder().Create());
  return CreateModularPeerConnectionFactory(std::move(pcf_deps));
}

rtc::scoped_refptr<PeerConnectionInterface> CreatePeerConnection(
    const rtc::scoped_refptr<PeerConnectionFactoryInterface>& pcf,
    PeerConnectionObserver* observer,
    rtc::NetworkManager* network_manager) {
  PeerConnectionDependencies pc_deps(observer);
  auto port_allocator =
      absl::make_unique<cricket::BasicPortAllocator>(network_manager);

  // This test does not support TCP
  int flags = cricket::PORTALLOCATOR_DISABLE_TCP;
  port_allocator->set_flags(port_allocator->flags() | flags);

  pc_deps.allocator = std::move(port_allocator);
  PeerConnectionInterface::RTCConfiguration rtc_configuration;
  rtc_configuration.sdp_semantics = SdpSemantics::kUnifiedPlan;

  return pcf->CreatePeerConnection(rtc_configuration, std::move(pc_deps));
}

}  // namespace

TEST(NetworkEmulationManagerPCTest, Run) {
  std::unique_ptr<rtc::Thread> signaling_thread = rtc::Thread::Create();
  signaling_thread->SetName(kSignalThreadName, nullptr);
  signaling_thread->Start();

  // Setup emulated network
  NetworkEmulationManager network_manager(Clock::GetRealTimeClock());

  EmulatedNetworkNode* alice_node = network_manager.CreateEmulatedNode(
      absl::make_unique<SimulatedNetwork>(BuiltInNetworkBehaviorConfig()));
  EmulatedNetworkNode* bob_node = network_manager.CreateEmulatedNode(
      absl::make_unique<SimulatedNetwork>(BuiltInNetworkBehaviorConfig()));
  rtc::IPAddress alice_ip(1);
  EndpointNode* alice_endpoint = network_manager.CreateEndpoint(alice_ip);
  rtc::IPAddress bob_ip(2);
  EndpointNode* bob_endpoint = network_manager.CreateEndpoint(bob_ip);
  network_manager.CreateRoute(alice_endpoint, {alice_node}, bob_endpoint);
  network_manager.CreateRoute(bob_endpoint, {bob_node}, alice_endpoint);

  rtc::Thread* alice_network_thread =
      network_manager.CreateNetworkThread({alice_endpoint});
  rtc::Thread* bob_network_thread =
      network_manager.CreateNetworkThread({bob_endpoint});

  // Setup peer connections.
  rtc::scoped_refptr<PeerConnectionFactoryInterface> alice_pcf;
  rtc::scoped_refptr<PeerConnectionInterface> alice_pc;
  std::unique_ptr<MockPeerConnectionObserver> alice_observer =
      absl::make_unique<MockPeerConnectionObserver>();
  std::unique_ptr<rtc::FakeNetworkManager> alice_network_manager =
      absl::make_unique<rtc::FakeNetworkManager>();
  alice_network_manager->AddInterface(rtc::SocketAddress(alice_ip, 0));

  rtc::scoped_refptr<PeerConnectionFactoryInterface> bob_pcf;
  rtc::scoped_refptr<PeerConnectionInterface> bob_pc;
  std::unique_ptr<MockPeerConnectionObserver> bob_observer =
      absl::make_unique<MockPeerConnectionObserver>();
  std::unique_ptr<rtc::FakeNetworkManager> bob_network_manager =
      absl::make_unique<rtc::FakeNetworkManager>();
  bob_network_manager->AddInterface(rtc::SocketAddress(bob_ip, 0));

  signaling_thread->Invoke<void>(RTC_FROM_HERE, [&]() {
    alice_pcf = CreatePeerConnectionFactory(signaling_thread.get(),
                                            alice_network_thread);
    alice_pc = CreatePeerConnection(alice_pcf, alice_observer.get(),
                                    alice_network_manager.get());

    bob_pcf =
        CreatePeerConnectionFactory(signaling_thread.get(), bob_network_thread);
    bob_pc = CreatePeerConnection(bob_pcf, bob_observer.get(),
                                  bob_network_manager.get());
  });

  std::unique_ptr<PeerConnectionWrapper> alice =
      absl::make_unique<PeerConnectionWrapper>(alice_pcf, alice_pc,
                                               std::move(alice_observer));
  std::unique_ptr<PeerConnectionWrapper> bob =
      absl::make_unique<PeerConnectionWrapper>(bob_pcf, bob_pc,
                                               std::move(bob_observer));

  network_manager.Start();

  signaling_thread->Invoke<void>(RTC_FROM_HERE, [&]() {
    rtc::scoped_refptr<webrtc::AudioSourceInterface> source =
        alice_pcf->CreateAudioSource(cricket::AudioOptions());
    rtc::scoped_refptr<AudioTrackInterface> track =
        alice_pcf->CreateAudioTrack("audio", source);
    alice->AddTransceiver(track);

    // Connect peers.
    ASSERT_TRUE(alice->ExchangeOfferAnswerWith(bob.get()));
    // Do the SDP negotiation, and also exchange ice candidates.
    ASSERT_TRUE_WAIT(
        alice->signaling_state() == PeerConnectionInterface::kStable,
        kDefaultTimeoutMs);
    ASSERT_TRUE_WAIT(alice->IsIceGatheringDone(), kDefaultTimeoutMs);
    ASSERT_TRUE_WAIT(bob->IsIceGatheringDone(), kDefaultTimeoutMs);

    // Connect an ICE candidate pairs.
    ASSERT_TRUE(
        AddIceCandidates(bob.get(), alice->observer()->GetAllCandidates()));
    ASSERT_TRUE(
        AddIceCandidates(alice.get(), bob->observer()->GetAllCandidates()));
    // This means that ICE and DTLS are connected.
    ASSERT_TRUE_WAIT(bob->IsIceConnected(), kDefaultTimeoutMs);
    ASSERT_TRUE_WAIT(alice->IsIceConnected(), kDefaultTimeoutMs);

    // Close peer connections
    alice->pc()->Close();
    bob->pc()->Close();

    // Delete peers.
    alice.reset();
    bob.reset();
  });

  network_manager.Stop();
}

}  // namespace test
}  // namespace webrtc
