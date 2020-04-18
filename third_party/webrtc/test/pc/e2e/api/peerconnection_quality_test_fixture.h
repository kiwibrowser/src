/*
 *  Copyright (c) 2018 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */
#ifndef TEST_PC_E2E_API_PEERCONNECTION_QUALITY_TEST_FIXTURE_H_
#define TEST_PC_E2E_API_PEERCONNECTION_QUALITY_TEST_FIXTURE_H_

#include <memory>
#include <string>
#include <vector>

#include "api/async_resolver_factory.h"
#include "api/call/call_factory_interface.h"
#include "api/fec_controller.h"
#include "api/media_transport_interface.h"
#include "api/peer_connection_interface.h"
#include "api/test/simulated_network.h"
#include "api/transport/network_control.h"
#include "api/units/time_delta.h"
#include "api/video_codecs/video_decoder_factory.h"
#include "api/video_codecs/video_encoder.h"
#include "api/video_codecs/video_encoder_factory.h"
#include "logging/rtc_event_log/rtc_event_log_factory_interface.h"
#include "rtc_base/network.h"
#include "rtc_base/rtc_certificate_generator.h"
#include "rtc_base/ssl_certificate.h"
#include "rtc_base/thread.h"
#include "test/pc/e2e/api/audio_quality_analyzer_interface.h"
#include "test/pc/e2e/api/video_quality_analyzer_interface.h"

namespace webrtc {

// TODO(titovartem) move to API when it will be stabilized.
class PeerConnectionE2EQualityTestFixture {
 public:
  // Contains most part from PeerConnectionFactoryDependencies. Also all fields
  // are optional and defaults will be provided by fixture implementation if
  // any will be omitted.
  //
  // Separate class was introduced to clarify which components can be
  // overridden. For example worker and signaling threads will be provided by
  // fixture implementation. The same is applicable to the media engine. So user
  // can override only some parts of media engine like video encoder/decoder
  // factories.
  struct PeerConnectionFactoryComponents {
    std::unique_ptr<CallFactoryInterface> call_factory;
    std::unique_ptr<RtcEventLogFactoryInterface> event_log_factory;
    std::unique_ptr<FecControllerFactoryInterface> fec_controller_factory;
    std::unique_ptr<NetworkControllerFactoryInterface>
        network_controller_factory;
    std::unique_ptr<MediaTransportFactory> media_transport_factory;

    // Will be passed to MediaEngineInterface, that will be used in
    // PeerConnectionFactory.
    std::unique_ptr<VideoEncoderFactory> video_encoder_factory;
    std::unique_ptr<VideoDecoderFactory> video_decoder_factory;
  };

  // Contains most parts from PeerConnectionDependencies. Also all fields are
  // optional and defaults will be provided by fixture implementation if any
  // will be omitted.
  //
  // Separate class was introduced to clarify which components can be
  // overridden. For example observer, which is required to
  // PeerConnectionDependencies, will be provided by fixture implementation,
  // so client can't inject its own. Also only network manager can be overridden
  // inside port allocator.
  struct PeerConnectionComponents {
    std::unique_ptr<rtc::NetworkManager> network_manager;
    std::unique_ptr<webrtc::AsyncResolverFactory> async_resolver_factory;
    std::unique_ptr<rtc::RTCCertificateGeneratorInterface> cert_generator;
    std::unique_ptr<rtc::SSLCertificateVerifier> tls_cert_verifier;
  };

  // Contains all components, that can be overridden in peer connection. Also
  // has a network thread, that will be used to communicate with another peers.
  struct InjectableComponents {
    explicit InjectableComponents(rtc::Thread* network_thread)
        : network_thread(network_thread) {
      RTC_CHECK(network_thread);
    }

    rtc::Thread* const network_thread;

    std::unique_ptr<PeerConnectionFactoryComponents> pcf_dependencies;
    std::unique_ptr<PeerConnectionComponents> pc_dependencies;
  };

  // Contains screen share video stream properties.
  struct ScreenShareConfig {
    // If true, slides will be generated programmatically.
    bool generate_slides;
    // Shows how long one slide should be presented on the screen during
    // slide generation.
    TimeDelta slide_change_interval;
    // If equal to 0, no scrolling will be applied.
    TimeDelta scroll_duration;
    // If empty, default set of slides will be used.
    std::vector<std::string> slides_yuv_file_names;
  };

  // Contains properties of single video stream.
  struct VideoConfig {
    size_t width;
    size_t height;
    int32_t fps;
    // Have to be unique among all specified configs for all peers in the call.
    absl::optional<std::string> stream_label;
    // Only single from 3 next fields can be specified.
    // If specified generator with this name will be used as input.
    absl::optional<std::string> generator_name;
    // If specified this file will be used as input.
    absl::optional<std::string> input_file_name;
    // If specified screen share video stream will be created as input.
    absl::optional<ScreenShareConfig> screen_share_config;
    // If specified the input stream will be also copied to specified file.
    absl::optional<std::string> input_dump_file_name;
    // If specified this file will be used as output on the receiver side for
    // this stream. If multiple streams will be produced by input stream,
    // output files will be appended with indexes.
    absl::optional<std::string> output_file_name;
  };

  // Contains properties for audio in the call.
  struct AudioConfig {
    enum Mode {
      kGenerated,
      kFile,
    };
    Mode mode;
    // Have to be specified only if mode = kFile
    absl::optional<std::string> input_file_name;
    // If specified the input stream will be also copied to specified file.
    absl::optional<std::string> input_dump_file_name;
    // If specified the output stream will be copied to specified file.
    absl::optional<std::string> output_file_name;
    // Audio options to use.
    cricket::AudioOptions audio_options;
  };

  // Contains information about call media streams (up to 1 audio stream and
  // unlimited amount of video streams) and rtc configuration, that will be used
  // to set up peer connection.
  struct Params {
    // If |video_configs| is empty - no video should be added to the test call.
    std::vector<VideoConfig> video_configs;
    // If |audio_config| is presented audio stream will be configured
    absl::optional<AudioConfig> audio_config;

    PeerConnectionInterface::RTCConfiguration rtc_configuration;
  };

  // Contains analyzers for audio and video stream. Both of them are optional
  // and default implementations will be provided, if any will be omitted.
  struct Analyzers {
    std::unique_ptr<AudioQualityAnalyzerInterface> audio_quality_analyzer;
    std::unique_ptr<VideoQualityAnalyzerInterface> video_quality_analyzer;
  };

  virtual void Run() = 0;
  virtual ~PeerConnectionE2EQualityTestFixture() = default;
};

}  // namespace webrtc

#endif  // TEST_PC_E2E_API_PEERCONNECTION_QUALITY_TEST_FIXTURE_H_
