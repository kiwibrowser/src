/*
 * Copyright (C) 2010, Google Inc. All rights reserved.
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

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_WEBAUDIO_SCRIPT_PROCESSOR_NODE_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_WEBAUDIO_SCRIPT_PROCESSOR_NODE_H_

#include "base/gtest_prod_util.h"
#include "base/memory/scoped_refptr.h"
#include "third_party/blink/renderer/bindings/core/v8/active_script_wrappable.h"
#include "third_party/blink/renderer/modules/webaudio/audio_node.h"
#include "third_party/blink/renderer/platform/audio/audio_bus.h"
#include "third_party/blink/renderer/platform/wtf/forward.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"

namespace blink {

class BaseAudioContext;
class AudioBuffer;
class WaitableEvent;

// ScriptProcessorNode is an AudioNode which allows for arbitrary synthesis or
// processing directly using JavaScript.  The API allows for a variable number
// of inputs and outputs, although it must have at least one input or output.
// This basic implementation supports no more than one input and output.  The
// "onaudioprocess" attribute is an event listener which will get called
// periodically with an AudioProcessingEvent which has AudioBuffers for each
// input and output.

class ScriptProcessorHandler final : public AudioHandler {
 public:
  static scoped_refptr<ScriptProcessorHandler> Create(
      AudioNode&,
      float sample_rate,
      size_t buffer_size,
      unsigned number_of_input_channels,
      unsigned number_of_output_channels);
  ~ScriptProcessorHandler() override;

  // AudioHandler
  void Process(size_t frames_to_process) override;
  void Initialize() override;

  size_t BufferSize() const { return buffer_size_; }

  void SetChannelCount(unsigned long, ExceptionState&) override;
  void SetChannelCountMode(const String&, ExceptionState&) override;

  unsigned NumberOfOutputChannels() const override {
    return number_of_output_channels_;
  }

 private:
  ScriptProcessorHandler(AudioNode&,
                         float sample_rate,
                         size_t buffer_size,
                         unsigned number_of_input_channels,
                         unsigned number_of_output_channels);
  double TailTime() const override;
  double LatencyTime() const override;
  bool RequiresTailProcessing() const final;

  void FireProcessEvent(unsigned);
  void FireProcessEventForOfflineAudioContext(unsigned, WaitableEvent*);

  // Double buffering
  unsigned DoubleBufferIndex() const { return double_buffer_index_; }
  void SwapBuffers() { double_buffer_index_ = 1 - double_buffer_index_; }
  unsigned double_buffer_index_;

  // These Persistent don't make reference cycles including the owner
  // ScriptProcessorNode.
  PersistentHeapVector<Member<AudioBuffer>> input_buffers_;
  PersistentHeapVector<Member<AudioBuffer>> output_buffers_;

  size_t buffer_size_;
  unsigned buffer_read_write_index_;

  unsigned number_of_input_channels_;
  unsigned number_of_output_channels_;

  scoped_refptr<AudioBus> internal_input_bus_;
  // Synchronize process() with fireProcessEvent().
  mutable Mutex process_event_lock_;

  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;

  FRIEND_TEST_ALL_PREFIXES(ScriptProcessorNodeTest, BufferLifetime);
};

class ScriptProcessorNode final
    : public AudioNode,
      public ActiveScriptWrappable<ScriptProcessorNode> {
  DEFINE_WRAPPERTYPEINFO();
  USING_GARBAGE_COLLECTED_MIXIN(ScriptProcessorNode);

 public:
  // bufferSize must be one of the following values: 256, 512, 1024, 2048,
  // 4096, 8192, 16384.
  // This value controls how frequently the onaudioprocess event handler is
  // called and how many sample-frames need to be processed each call.
  // Lower numbers for bufferSize will result in a lower (better)
  // latency. Higher numbers will be necessary to avoid audio breakup and
  // glitches.
  // The value chosen must carefully balance between latency and audio quality.
  static ScriptProcessorNode* Create(BaseAudioContext&, ExceptionState&);
  static ScriptProcessorNode* Create(BaseAudioContext&,
                                     size_t buffer_size,
                                     ExceptionState&);
  static ScriptProcessorNode* Create(BaseAudioContext&,
                                     size_t buffer_size,
                                     unsigned number_of_input_channels,
                                     ExceptionState&);
  static ScriptProcessorNode* Create(BaseAudioContext&,
                                     size_t buffer_size,
                                     unsigned number_of_input_channels,
                                     unsigned number_of_output_channels,
                                     ExceptionState&);

  DEFINE_ATTRIBUTE_EVENT_LISTENER(audioprocess);
  size_t bufferSize() const;

  // ScriptWrappable
  bool HasPendingActivity() const final;

  void Trace(blink::Visitor* visitor) override { AudioNode::Trace(visitor); }

 private:
  ScriptProcessorNode(BaseAudioContext&,
                      float sample_rate,
                      size_t buffer_size,
                      unsigned number_of_input_channels,
                      unsigned number_of_output_channels);
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_WEBAUDIO_SCRIPT_PROCESSOR_NODE_H_
