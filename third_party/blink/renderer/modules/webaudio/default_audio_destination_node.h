/*
 * Copyright (C) 2011, Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_WEBAUDIO_DEFAULT_AUDIO_DESTINATION_NODE_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_WEBAUDIO_DEFAULT_AUDIO_DESTINATION_NODE_H_

#include <memory>
#include "third_party/blink/public/platform/web_audio_latency_hint.h"
#include "third_party/blink/renderer/modules/webaudio/audio_destination_node.h"
#include "third_party/blink/renderer/platform/audio/audio_destination.h"

namespace blink {

class BaseAudioContext;
class ExceptionState;
class WebAudioLatencyHint;

class DefaultAudioDestinationHandler final : public AudioDestinationHandler {
 public:
  static scoped_refptr<DefaultAudioDestinationHandler> Create(
      AudioNode&,
      const WebAudioLatencyHint&);
  ~DefaultAudioDestinationHandler() override;

  // AudioHandler
  void Dispose() override;
  void Initialize() override;
  void Uninitialize() override;
  void SetChannelCount(unsigned long, ExceptionState&) override;

  // AudioDestinationHandler
  void StartRendering() override;
  void StopRendering() override;
  void RestartRendering() override;
  unsigned long MaxChannelCount() const override;
  // Returns the rendering callback buffer size.
  size_t CallbackBufferSize() const override;
  double SampleRate() const override;
  int FramesPerBuffer() const override;

  double TailTime() const override { return 0; }
  double LatencyTime() const override { return 0; }
  bool RequiresTailProcessing() const final { return false; }

 private:
  explicit DefaultAudioDestinationHandler(AudioNode&,
                                          const WebAudioLatencyHint&);
  void CreateDestination();

  // Starts platform/AudioDestination. If the runtime flag for AudioWorklet is
  // set, uses the AudioWorkletThread's backing thread for the rendering.
  void StartDestination();

  void StopDestination();

  // Uses |RefPtr| to keep the AudioDestination alive until all the cross-thread
  // tasks are completed.
  scoped_refptr<AudioDestination> destination_;

  String input_device_id_;
  unsigned number_of_input_channels_;
  const WebAudioLatencyHint latency_hint_;
};

class DefaultAudioDestinationNode final : public AudioDestinationNode {
 public:
  static DefaultAudioDestinationNode* Create(BaseAudioContext*,
                                             const WebAudioLatencyHint&);

  size_t CallbackBufferSize() const { return Handler().CallbackBufferSize(); };

 private:
  explicit DefaultAudioDestinationNode(BaseAudioContext&,
                                       const WebAudioLatencyHint&);
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_WEBAUDIO_DEFAULT_AUDIO_DESTINATION_NODE_H_
