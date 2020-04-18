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

#include "third_party/blink/renderer/modules/webaudio/audio_destination_node.h"
#include "third_party/blink/renderer/modules/webaudio/audio_node_input.h"
#include "third_party/blink/renderer/modules/webaudio/audio_node_output.h"
#include "third_party/blink/renderer/modules/webaudio/base_audio_context.h"
#include "third_party/blink/renderer/platform/audio/audio_utilities.h"
#include "third_party/blink/renderer/platform/audio/denormal_disabler.h"
#include "third_party/blink/renderer/platform/instrumentation/tracing/trace_event.h"
#include "third_party/blink/renderer/platform/wtf/atomics.h"

namespace blink {

AudioDestinationHandler::AudioDestinationHandler(AudioNode& node)
    : AudioHandler(kNodeTypeDestination, node, 0), current_sample_frame_(0) {
  AddInput();
}

AudioDestinationHandler::~AudioDestinationHandler() {
  DCHECK(!IsInitialized());
}

void AudioDestinationHandler::Render(AudioBus* source_bus,
                                     AudioBus* destination_bus,
                                     size_t number_of_frames,
                                     const AudioIOPosition& output_position) {
  TRACE_EVENT0("webaudio", "AudioDestinationHandler::Render");

  // We don't want denormals slowing down any of the audio processing
  // since they can very seriously hurt performance.  This will take care of all
  // AudioNodes because they all process within this scope.
  DenormalDisabler denormal_disabler;

  // Need to check if the context actually alive. Otherwise the subsequent
  // steps will fail. If the context is not alive somehow, return immediately
  // and do nothing.
  //
  // TODO(hongchan): because the context can go away while rendering, so this
  // check cannot guarantee the safe execution of the following steps.
  DCHECK(Context());
  if (!Context())
    return;

  Context()->GetDeferredTaskHandler().SetAudioThreadToCurrentThread();

  // If the destination node is not initialized, pass the silence to the final
  // audio destination (one step before the FIFO). This check is for the case
  // where the destination is in the middle of tearing down process.
  if (!IsInitialized()) {
    destination_bus->Zero();
    return;
  }

  // Let the context take care of any business at the start of each render
  // quantum.
  Context()->HandlePreRenderTasks(output_position);

  // Prepare the local audio input provider for this render quantum.
  if (source_bus)
    local_audio_input_provider_.Set(source_bus);

  DCHECK_GE(NumberOfInputs(), 1u);
  if (NumberOfInputs() < 1) {
    destination_bus->Zero();
    return;
  }
  // This will cause the node(s) connected to us to process, which in turn will
  // pull on their input(s), all the way backwards through the rendering graph.
  AudioBus* rendered_bus = Input(0).Pull(destination_bus, number_of_frames);

  if (!rendered_bus) {
    destination_bus->Zero();
  } else if (rendered_bus != destination_bus) {
    // in-place processing was not possible - so copy
    destination_bus->CopyFrom(*rendered_bus);
  }

  // Process nodes which need a little extra help because they are not connected
  // to anything, but still need to process.
  Context()->GetDeferredTaskHandler().ProcessAutomaticPullNodes(
      number_of_frames);

  // Let the context take care of any business at the end of each render
  // quantum.
  Context()->HandlePostRenderTasks();

  // Advance current sample-frame.
  size_t new_sample_frame = current_sample_frame_ + number_of_frames;
  ReleaseStore(&current_sample_frame_, new_sample_frame);

  Context()->UpdateWorkletGlobalScopeOnRenderingThread();
}

// ----------------------------------------------------------------

AudioDestinationNode::AudioDestinationNode(BaseAudioContext& context)
    : AudioNode(context) {}

AudioDestinationHandler& AudioDestinationNode::GetAudioDestinationHandler()
    const {
  return static_cast<AudioDestinationHandler&>(Handler());
}

unsigned long AudioDestinationNode::maxChannelCount() const {
  return GetAudioDestinationHandler().MaxChannelCount();
}

}  // namespace blink
