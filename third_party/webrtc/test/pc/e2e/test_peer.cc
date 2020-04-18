/*
 *  Copyright (c) 2019 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */
#include "test/pc/e2e/test_peer.h"

#include <utility>

#include "absl/memory/memory.h"
#include "absl/types/optional.h"
#include "api/audio_codecs/builtin_audio_decoder_factory.h"
#include "api/audio_codecs/builtin_audio_encoder_factory.h"
#include "api/scoped_refptr.h"
#include "api/video_codecs/builtin_video_decoder_factory.h"
#include "api/video_codecs/builtin_video_encoder_factory.h"
#include "logging/rtc_event_log/rtc_event_log_factory.h"
#include "media/engine/webrtc_media_engine.h"
#include "modules/audio_device/include/audio_device.h"
#include "modules/audio_device/include/test_audio_device.h"
#include "modules/audio_processing/include/audio_processing.h"
#include "p2p/client/basic_port_allocator.h"
#include "pc/test/mock_peer_connection_observers.h"
#include "rtc_base/bind.h"
#include "rtc_base/location.h"
#include "rtc_base/network.h"
#include "test/frame_generator_capturer.h"
#include "test/pc/e2e/analyzer/video/default_encoded_image_id_injector.h"
#include "test/pc/e2e/analyzer/video/example_video_quality_analyzer.h"
#include "test/testsupport/copy_to_file_audio_capturer.h"

namespace webrtc {
namespace test {
namespace {

constexpr int16_t kGeneratedAudioMaxAmplitude = 32000;
constexpr int kSamplingFrequencyInHz = 48000;

using Params = PeerConnectionE2EQualityTestFixture::Params;
using InjectableComponents =
    PeerConnectionE2EQualityTestFixture::InjectableComponents;
using PeerConnectionFactoryComponents =
    PeerConnectionE2EQualityTestFixture::PeerConnectionFactoryComponents;
using PeerConnectionComponents =
    PeerConnectionE2EQualityTestFixture::PeerConnectionComponents;
using AudioConfig = PeerConnectionE2EQualityTestFixture::AudioConfig;

// Sets mandatory entities in injectable components like |pcf_dependencies|
// and |pc_dependencies| if they are omitted. Also setup required
// dependencies, that won't be specially provided by factory and will be just
// transferred to peer connection creation code.
void SetMandatoryEntities(InjectableComponents* components) {
  if (components->pcf_dependencies == nullptr) {
    components->pcf_dependencies =
        absl::make_unique<PeerConnectionFactoryComponents>();
  }
  if (components->pc_dependencies == nullptr) {
    components->pc_dependencies = absl::make_unique<PeerConnectionComponents>();
  }

  // Setup required peer connection factory dependencies.
  if (components->pcf_dependencies->call_factory == nullptr) {
    components->pcf_dependencies->call_factory = webrtc::CreateCallFactory();
  }
  if (components->pcf_dependencies->event_log_factory == nullptr) {
    components->pcf_dependencies->event_log_factory =
        webrtc::CreateRtcEventLogFactory();
  }
}

std::unique_ptr<TestAudioDeviceModule::Capturer> CreateAudioCapturer(
    AudioConfig audio_config) {
  if (audio_config.mode == AudioConfig::Mode::kGenerated) {
    return TestAudioDeviceModule::CreatePulsedNoiseCapturer(
        kGeneratedAudioMaxAmplitude, kSamplingFrequencyInHz);
  } else if (audio_config.mode == AudioConfig::Mode::kFile) {
    RTC_DCHECK(audio_config.input_file_name);
    return TestAudioDeviceModule::CreateWavFileReader(
        audio_config.input_file_name.value());
  } else {
    RTC_NOTREACHED() << "Unknown audio_config->mode";
    return nullptr;
  }
}

rtc::scoped_refptr<AudioDeviceModule> CreateAudioDeviceModule(
    absl::optional<AudioConfig> audio_config,
    absl::optional<std::string> audio_output_file_name) {
  std::unique_ptr<TestAudioDeviceModule::Capturer> capturer;
  if (audio_config) {
    capturer = CreateAudioCapturer(audio_config.value());
  } else {
    // If we have no audio config we still need to provide some audio device.
    // In such case use generated capturer. Despite of we provided audio here,
    // in test media setup audio stream won't be added into peer connection.
    capturer = TestAudioDeviceModule::CreatePulsedNoiseCapturer(
        kGeneratedAudioMaxAmplitude, kSamplingFrequencyInHz);
  }
  RTC_DCHECK(capturer);

  if (audio_config && audio_config->input_dump_file_name) {
    capturer = absl::make_unique<CopyToFileAudioCapturer>(
        std::move(capturer), audio_config->input_dump_file_name.value());
  }

  std::unique_ptr<TestAudioDeviceModule::Renderer> renderer;
  if (audio_output_file_name) {
    renderer = TestAudioDeviceModule::CreateBoundedWavFileWriter(
        audio_output_file_name.value(), kSamplingFrequencyInHz);
  } else {
    renderer =
        TestAudioDeviceModule::CreateDiscardRenderer(kSamplingFrequencyInHz);
  }

  return TestAudioDeviceModule::CreateTestAudioDeviceModule(
      std::move(capturer), std::move(renderer), /*speed=*/1.f);
}

std::unique_ptr<VideoEncoderFactory> CreateVideoEncoderFactory(
    PeerConnectionFactoryComponents* pcf_dependencies,
    VideoQualityAnalyzerInjectionHelper* video_analyzer_helper) {
  std::unique_ptr<VideoEncoderFactory> video_encoder_factory;
  if (pcf_dependencies->video_encoder_factory != nullptr) {
    video_encoder_factory = std::move(pcf_dependencies->video_encoder_factory);
  } else {
    video_encoder_factory = CreateBuiltinVideoEncoderFactory();
  }
  return video_analyzer_helper->WrapVideoEncoderFactory(
      std::move(video_encoder_factory));
}

std::unique_ptr<VideoDecoderFactory> CreateVideoDecoderFactory(
    PeerConnectionFactoryComponents* pcf_dependencies,
    VideoQualityAnalyzerInjectionHelper* video_analyzer_helper) {
  std::unique_ptr<VideoDecoderFactory> video_decoder_factory;
  if (pcf_dependencies->video_decoder_factory != nullptr) {
    video_decoder_factory = std::move(pcf_dependencies->video_decoder_factory);
  } else {
    video_decoder_factory = CreateBuiltinVideoDecoderFactory();
  }
  return video_analyzer_helper->WrapVideoDecoderFactory(
      std::move(video_decoder_factory));
}

std::unique_ptr<cricket::MediaEngineInterface> CreateMediaEngine(
    PeerConnectionFactoryComponents* pcf_dependencies,
    absl::optional<AudioConfig> audio_config,
    VideoQualityAnalyzerInjectionHelper* video_analyzer_helper,
    absl::optional<std::string> audio_output_file_name) {
  rtc::scoped_refptr<AudioDeviceModule> adm = CreateAudioDeviceModule(
      std::move(audio_config), std::move(audio_output_file_name));

  std::unique_ptr<VideoEncoderFactory> video_encoder_factory =
      CreateVideoEncoderFactory(pcf_dependencies, video_analyzer_helper);
  std::unique_ptr<VideoDecoderFactory> video_decoder_factory =
      CreateVideoDecoderFactory(pcf_dependencies, video_analyzer_helper);

  return cricket::WebRtcMediaEngineFactory::Create(
      adm, webrtc::CreateBuiltinAudioEncoderFactory(),
      webrtc::CreateBuiltinAudioDecoderFactory(),
      std::move(video_encoder_factory), std::move(video_decoder_factory),
      /*audio_mixer=*/nullptr, webrtc::AudioProcessingBuilder().Create());
}

// Creates PeerConnectionFactoryDependencies objects, providing entities
// from InjectableComponents::PeerConnectionFactoryComponents and also
// creating entities, that are required for correct injection of media quality
// analyzers.
PeerConnectionFactoryDependencies CreatePCFDependencies(
    std::unique_ptr<PeerConnectionFactoryComponents> pcf_dependencies,
    absl::optional<AudioConfig> audio_config,
    VideoQualityAnalyzerInjectionHelper* video_analyzer_helper,
    rtc::Thread* network_thread,
    rtc::Thread* signaling_thread,
    rtc::Thread* worker_thread,
    absl::optional<std::string> audio_output_file_name) {
  PeerConnectionFactoryDependencies pcf_deps;
  pcf_deps.network_thread = network_thread;
  pcf_deps.signaling_thread = signaling_thread;
  pcf_deps.worker_thread = worker_thread;
  pcf_deps.media_engine = CreateMediaEngine(
      pcf_dependencies.get(), std::move(audio_config), video_analyzer_helper,
      std::move(audio_output_file_name));

  pcf_deps.call_factory = std::move(pcf_dependencies->call_factory);
  pcf_deps.event_log_factory = std::move(pcf_dependencies->event_log_factory);

  if (pcf_dependencies->fec_controller_factory != nullptr) {
    pcf_deps.fec_controller_factory =
        std::move(pcf_dependencies->fec_controller_factory);
  }
  if (pcf_dependencies->network_controller_factory != nullptr) {
    pcf_deps.network_controller_factory =
        std::move(pcf_dependencies->network_controller_factory);
  }
  if (pcf_dependencies->media_transport_factory != nullptr) {
    pcf_deps.media_transport_factory =
        std::move(pcf_dependencies->media_transport_factory);
  }

  return pcf_deps;
}

// Creates PeerConnectionDependencies objects, providing entities
// from InjectableComponents::PeerConnectionComponents.
PeerConnectionDependencies CreatePCDependencies(
    PeerConnectionComponents* pc_dependencies,
    PeerConnectionObserver* observer) {
  PeerConnectionDependencies pc_deps(observer);

  // We need to create network manager, because it is required for port
  // allocator. TestPeer will take ownership of this object and will store it
  // until the end of the test.
  if (pc_dependencies->network_manager == nullptr) {
    pc_dependencies->network_manager =
        // TODO(titovartem) have network manager integrated with emulated
        // network layer.
        absl::make_unique<rtc::BasicNetworkManager>();
  }
  auto port_allocator = absl::make_unique<cricket::BasicPortAllocator>(
      pc_dependencies->network_manager.get());

  // This test does not support TCP
  int flags = cricket::PORTALLOCATOR_DISABLE_TCP;
  port_allocator->set_flags(port_allocator->flags() | flags);

  pc_deps.allocator = std::move(port_allocator);

  if (pc_dependencies->async_resolver_factory != nullptr) {
    pc_deps.async_resolver_factory =
        std::move(pc_dependencies->async_resolver_factory);
  }
  if (pc_dependencies->cert_generator != nullptr) {
    pc_deps.cert_generator = std::move(pc_dependencies->cert_generator);
  }
  if (pc_dependencies->tls_cert_verifier != nullptr) {
    pc_deps.tls_cert_verifier = std::move(pc_dependencies->tls_cert_verifier);
  }
  return pc_deps;
}

}  // namespace

std::unique_ptr<TestPeer> TestPeer::CreateTestPeer(
    std::unique_ptr<InjectableComponents> components,
    std::unique_ptr<Params> params,
    VideoQualityAnalyzerInjectionHelper* video_analyzer_helper,
    rtc::Thread* signaling_thread,
    rtc::Thread* worker_thread,
    absl::optional<std::string> audio_output_file_name) {
  RTC_DCHECK(components);
  RTC_DCHECK(params);
  SetMandatoryEntities(components.get());
  params->rtc_configuration.sdp_semantics = SdpSemantics::kUnifiedPlan;

  std::unique_ptr<MockPeerConnectionObserver> observer =
      absl::make_unique<MockPeerConnectionObserver>();

  // Create peer connection factory.
  PeerConnectionFactoryDependencies pcf_deps = CreatePCFDependencies(
      std::move(components->pcf_dependencies), params->audio_config,
      video_analyzer_helper, components->network_thread, signaling_thread,
      worker_thread, std::move(audio_output_file_name));
  rtc::scoped_refptr<PeerConnectionFactoryInterface> pcf =
      CreateModularPeerConnectionFactory(std::move(pcf_deps));

  // Create peer connection.
  PeerConnectionDependencies pc_deps =
      CreatePCDependencies(components->pc_dependencies.get(), observer.get());
  rtc::scoped_refptr<PeerConnectionInterface> pc =
      pcf->CreatePeerConnection(params->rtc_configuration, std::move(pc_deps));

  return absl::WrapUnique(
      new TestPeer(pcf, pc, std::move(observer), std::move(params),
                   std::move(components->pc_dependencies->network_manager)));
}

bool TestPeer::AddIceCandidates(
    rtc::ArrayView<const IceCandidateInterface* const> candidates) {
  bool success = true;
  for (const auto* candidate : candidates) {
    if (!pc()->AddIceCandidate(candidate)) {
      success = false;
    }
  }
  return success;
}

TestPeer::TestPeer(
    rtc::scoped_refptr<PeerConnectionFactoryInterface> pc_factory,
    rtc::scoped_refptr<PeerConnectionInterface> pc,
    std::unique_ptr<MockPeerConnectionObserver> observer,
    std::unique_ptr<Params> params,
    std::unique_ptr<rtc::NetworkManager> network_manager)
    : PeerConnectionWrapper::PeerConnectionWrapper(pc_factory,
                                                   pc,
                                                   std::move(observer)),
      params_(std::move(params)),
      network_manager_(std::move(network_manager)) {}

}  // namespace test
}  // namespace webrtc
