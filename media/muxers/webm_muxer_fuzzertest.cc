// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>
#include <stdint.h>

#include <memory>
#include <random>

#include "base/bind.h"
#include "base/logging.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/strings/string_piece.h"
#include "media/base/audio_parameters.h"
#include "media/base/video_frame.h"
#include "media/muxers/webm_muxer.h"

using namespace media;

// Min and max number of encodec video/audio packets to send in the WebmMuxer.
const int kMinNumIterations = 1;
const int kMaxNumIterations = 10;

static const int kSupportedVideoCodecs[] = {kCodecVP8, kCodecVP9, kCodecH264};
static const int kSupportedAudioCodecs[] = {kCodecOpus, kCodecPCM};

static const int kSampleRatesInKHz[] = {48, 24, 16, 12, 8};

static struct {
  bool has_video;
  bool has_audio;
} kVideoAudioInputTypes[] = {{true, false}, {false, true}, {true, true}};

struct Env {
  Env() { logging::SetMinLogLevel(logging::LOG_FATAL); }

  base::MessageLoop message_loop;
};
Env* env = new Env();

void OnWriteCallback(base::StringPiece data) {}

// Entry point for LibFuzzer.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  std::mt19937_64 rng;

  std::string str = std::string(reinterpret_cast<const char*>(data), size);
  {  // Seed rng from data.
    std::size_t data_hash = std::hash<std::string>()(str);
    rng.seed(data_hash);
  }

  for (const auto& input_type : kVideoAudioInputTypes) {
    const auto video_codec = static_cast<VideoCodec>(
        kSupportedVideoCodecs[rng() % arraysize(kSupportedVideoCodecs)]);
    const auto audio_codec = static_cast<AudioCodec>(
        kSupportedAudioCodecs[rng() % arraysize(kSupportedAudioCodecs)]);
    WebmMuxer muxer(video_codec, audio_codec, input_type.has_video,
                    input_type.has_audio, base::Bind(&OnWriteCallback));
    base::RunLoop run_loop;
    run_loop.RunUntilIdle();

    int num_iterations = kMinNumIterations + rng() % kMaxNumIterations;
    do {
      if (input_type.has_video) {
        // VideoFrames cannot be arbitrarily small.
        const auto visible_rect = gfx::Size(16 + rng() % 128, 16 + rng() % 128);
        const auto video_frame = VideoFrame::CreateBlackFrame(visible_rect);
        const auto is_key_frame = rng() % 2;
        const auto has_alpha_frame = rng() % 4;
        muxer.OnEncodedVideo(
            WebmMuxer::VideoParameters(video_frame),
            std::make_unique<std::string>(str),
            has_alpha_frame ? std::make_unique<std::string>(str) : nullptr,
            base::TimeTicks(), is_key_frame);
        base::RunLoop run_loop;
        run_loop.RunUntilIdle();
      }

      if (input_type.has_audio) {
        const ChannelLayout layout = rng() % 2 ? media::CHANNEL_LAYOUT_STEREO
                                               : media::CHANNEL_LAYOUT_MONO;
        const int sample_rate =
            kSampleRatesInKHz[rng() % arraysize(kSampleRatesInKHz)];

        const AudioParameters params(
            media::AudioParameters::AUDIO_PCM_LOW_LATENCY, layout, sample_rate,
            60 * sample_rate);
        muxer.OnEncodedAudio(params, std::make_unique<std::string>(str),
                             base::TimeTicks());
        base::RunLoop run_loop;
        run_loop.RunUntilIdle();
      }
    } while (num_iterations--);
  }

  return 0;
}
