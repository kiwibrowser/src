/*
 *  Copyright 2018 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "api/create_peerconnection_factory.h"

#include <memory>
#include <utility>

#include "api/audio/audio_mixer.h"
#include "api/audio_codecs/audio_decoder_factory.h"
#include "api/audio_codecs/audio_encoder_factory.h"
#include "api/call/call_factory_interface.h"
#include "api/fec_controller.h"
#include "api/peer_connection_interface.h"
#include "api/scoped_refptr.h"
#include "api/transport/network_control.h"
#include "api/video_codecs/video_decoder_factory.h"
#include "api/video_codecs/video_encoder_factory.h"
#include "logging/rtc_event_log/rtc_event_log_factory.h"
#include "logging/rtc_event_log/rtc_event_log_factory_interface.h"
#include "media/base/media_engine.h"
#include "media/engine/webrtc_media_engine.h"
#include "modules/audio_device/include/audio_device.h"
#include "modules/audio_processing/include/audio_processing.h"
#include "rtc_base/thread.h"

namespace webrtc {

rtc::scoped_refptr<PeerConnectionFactoryInterface> CreatePeerConnectionFactory(
    rtc::Thread* network_thread,
    rtc::Thread* worker_thread,
    rtc::Thread* signaling_thread,
    rtc::scoped_refptr<AudioDeviceModule> default_adm,
    rtc::scoped_refptr<AudioEncoderFactory> audio_encoder_factory,
    rtc::scoped_refptr<AudioDecoderFactory> audio_decoder_factory,
    std::unique_ptr<VideoEncoderFactory> video_encoder_factory,
    std::unique_ptr<VideoDecoderFactory> video_decoder_factory,
    rtc::scoped_refptr<AudioMixer> audio_mixer,
    rtc::scoped_refptr<AudioProcessing> audio_processing) {
  if (!audio_processing)
    audio_processing = AudioProcessingBuilder().Create();

  std::unique_ptr<cricket::MediaEngineInterface> media_engine =
      cricket::WebRtcMediaEngineFactory::Create(
          default_adm, audio_encoder_factory, audio_decoder_factory,
          std::move(video_encoder_factory), std::move(video_decoder_factory),
          audio_mixer, audio_processing);

  std::unique_ptr<CallFactoryInterface> call_factory = CreateCallFactory();

  std::unique_ptr<RtcEventLogFactoryInterface> event_log_factory =
      CreateRtcEventLogFactory();
  PeerConnectionFactoryDependencies dependencies;
  dependencies.network_thread = network_thread;
  dependencies.worker_thread = worker_thread;
  dependencies.signaling_thread = signaling_thread;
  dependencies.media_engine = std::move(media_engine);
  dependencies.call_factory = std::move(call_factory);
  dependencies.event_log_factory = std::move(event_log_factory);
  return CreateModularPeerConnectionFactory(std::move(dependencies));
}

}  // namespace webrtc
