/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_WEBAUDIO_AUDIO_SCHEDULED_SOURCE_NODE_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_WEBAUDIO_AUDIO_SCHEDULED_SOURCE_NODE_H_

#include "third_party/blink/renderer/bindings/core/v8/active_script_wrappable.h"
#include "third_party/blink/renderer/modules/webaudio/audio_node.h"

namespace blink {

class BaseAudioContext;
class AudioBus;

class AudioScheduledSourceHandler : public AudioHandler {
 public:
  // These are the possible states an AudioScheduledSourceNode can be in:
  //
  // UNSCHEDULED_STATE - Initial playback state. Created, but not yet scheduled.
  // SCHEDULED_STATE - Scheduled to play (via start()), but not yet playing.
  // PLAYING_STATE - Generating sound.
  // FINISHED_STATE - Finished generating sound.
  //
  // The state can only transition to the next state, except for the
  // FINISHED_STATE which can never be changed.
  enum PlaybackState {
    // These must be defined with the same names and values as in the .idl file.
    UNSCHEDULED_STATE = 0,
    SCHEDULED_STATE = 1,
    PLAYING_STATE = 2,
    FINISHED_STATE = 3
  };

  AudioScheduledSourceHandler(NodeType, AudioNode&, float sample_rate);

  // Scheduling.
  void Start(double when, ExceptionState&);
  void Stop(double when, ExceptionState&);

  // AudioNode
  double TailTime() const override { return 0; }
  double LatencyTime() const override { return 0; }

  PlaybackState GetPlaybackState() const {
    return static_cast<PlaybackState>(AcquireLoad(&playback_state_));
  }

  void SetPlaybackState(PlaybackState new_state) {
    ReleaseStore(&playback_state_, new_state);
  }

  bool IsPlayingOrScheduled() const {
    PlaybackState state = GetPlaybackState();
    return state == PLAYING_STATE || state == SCHEDULED_STATE;
  }

  bool HasFinished() const { return GetPlaybackState() == FINISHED_STATE; }

  // Source nodes don't have tail or latency times so no tail
  // processing needed.
  bool RequiresTailProcessing() const final { return false; }

 protected:
  // Get frame information for the current time quantum.
  // We handle the transition into PLAYING_STATE and FINISHED_STATE here,
  // zeroing out portions of the outputBus which are outside the range of
  // startFrame and endFrame.
  //
  // Each frame time is relative to the context's currentSampleFrame().
  // quantumFrameOffset    : Offset frame in this time quantum to start
  //                         rendering.
  // nonSilentFramesToProcess : Number of frames rendering non-silence (will be
  //                            <= quantumFrameSize).
  // startFrameOffset : The fractional frame offset from quantumFrameOffset
  //                    and the actual starting time of the source. This is
  //                    non-zero only when transitioning from the
  //                    SCHEDULED_STATE to the PLAYING_STATE.
  void UpdateSchedulingInfo(size_t quantum_frame_size,
                            AudioBus* output_bus,
                            size_t& quantum_frame_offset,
                            size_t& non_silent_frames_to_process,
                            double& start_frame_offset);

  // Called when we have no more sound to play or the stop() time has been
  // reached. No onEnded event is called.
  virtual void FinishWithoutOnEnded();

  // Like finishWithoutOnEnded(), but an onEnded (if specified) is called.
  virtual void Finish();

  void NotifyEnded();

  // This synchronizes with process() and any other method that needs to be
  // synchronized like setBuffer for AudioBufferSource.
  mutable Mutex process_lock_;

  // m_startTime is the time to start playing based on the context's timeline (0
  // or a time less than the context's current time means "now").
  double start_time_;  // in seconds

  // m_endTime is the time to stop playing based on the context's timeline (0 or
  // a time less than the context's current time means "now").  If it hasn't
  // been set explicitly, then the sound will not stop playing (if looping) or
  // will stop when the end of the AudioBuffer has been reached.
  double end_time_;  // in seconds

  static const double kUnknownTime;

 private:
  // This is accessed by both the main thread and audio thread.  Use the setter
  // and getter to protect the access to this.
  int playback_state_;

  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;
};

class AudioScheduledSourceNode
    : public AudioNode,
      public ActiveScriptWrappable<AudioScheduledSourceNode> {
  USING_GARBAGE_COLLECTED_MIXIN(AudioScheduledSourceNode);
  DEFINE_WRAPPERTYPEINFO();

 public:
  void start(ExceptionState&);
  void start(double when, ExceptionState&);
  void stop(ExceptionState&);
  void stop(double when, ExceptionState&);

  EventListener* onended();
  void setOnended(EventListener*);

  // ScriptWrappable:
  bool HasPendingActivity() const final;

  void Trace(blink::Visitor* visitor) override { AudioNode::Trace(visitor); }

 protected:
  explicit AudioScheduledSourceNode(BaseAudioContext&);
  AudioScheduledSourceHandler& GetAudioScheduledSourceHandler() const;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_WEBAUDIO_AUDIO_SCHEDULED_SOURCE_NODE_H_
