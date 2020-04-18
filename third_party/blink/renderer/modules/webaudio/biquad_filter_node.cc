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

#include "third_party/blink/renderer/modules/webaudio/biquad_filter_node.h"

#include <memory>

#include "third_party/blink/renderer/bindings/core/v8/exception_messages.h"
#include "third_party/blink/renderer/bindings/core/v8/exception_state.h"
#include "third_party/blink/renderer/core/dom/exception_code.h"
#include "third_party/blink/renderer/modules/webaudio/audio_basic_processor_handler.h"
#include "third_party/blink/renderer/modules/webaudio/biquad_filter_options.h"
#include "third_party/blink/renderer/platform/histogram.h"

namespace blink {

BiquadFilterHandler::BiquadFilterHandler(AudioNode& node,
                                         float sample_rate,
                                         AudioParamHandler& frequency,
                                         AudioParamHandler& q,
                                         AudioParamHandler& gain,
                                         AudioParamHandler& detune)
    : AudioBasicProcessorHandler(kNodeTypeBiquadFilter,
                                 node,
                                 sample_rate,
                                 std::make_unique<BiquadProcessor>(sample_rate,
                                                                   1,
                                                                   frequency,
                                                                   q,
                                                                   gain,
                                                                   detune)) {
  // Initialize the handler so that AudioParams can be processed.
  Initialize();
}

scoped_refptr<BiquadFilterHandler> BiquadFilterHandler::Create(
    AudioNode& node,
    float sample_rate,
    AudioParamHandler& frequency,
    AudioParamHandler& q,
    AudioParamHandler& gain,
    AudioParamHandler& detune) {
  return base::AdoptRef(
      new BiquadFilterHandler(node, sample_rate, frequency, q, gain, detune));
}

BiquadFilterNode::BiquadFilterNode(BaseAudioContext& context)
    : AudioNode(context),
      frequency_(
          AudioParam::Create(context,
                             kParamTypeBiquadFilterFrequency,
                             350.0,
                             AudioParamHandler::AutomationRate::kAudio,
                             AudioParamHandler::AutomationRateMode::kVariable,
                             0,
                             context.sampleRate() / 2)),
      q_(AudioParam::Create(context,
                            kParamTypeBiquadFilterQ,
                            1.0,
                            AudioParamHandler::AutomationRate::kAudio,
                            AudioParamHandler::AutomationRateMode::kVariable)),
      gain_(
          AudioParam::Create(context,
                             kParamTypeBiquadFilterGain,
                             0.0,
                             AudioParamHandler::AutomationRate::kAudio,
                             AudioParamHandler::AutomationRateMode::kVariable)),
      detune_(AudioParam::Create(
          context,
          kParamTypeBiquadFilterDetune,
          0.0,
          AudioParamHandler::AutomationRate::kAudio,
          AudioParamHandler::AutomationRateMode::kVariable)) {
  SetHandler(BiquadFilterHandler::Create(*this, context.sampleRate(),
                                         frequency_->Handler(), q_->Handler(),
                                         gain_->Handler(), detune_->Handler()));

  setType("lowpass");
}

BiquadFilterNode* BiquadFilterNode::Create(BaseAudioContext& context,
                                           ExceptionState& exception_state) {
  DCHECK(IsMainThread());

  if (context.IsContextClosed()) {
    context.ThrowExceptionForClosedState(exception_state);
    return nullptr;
  }

  return new BiquadFilterNode(context);
}

BiquadFilterNode* BiquadFilterNode::Create(BaseAudioContext* context,
                                           const BiquadFilterOptions& options,
                                           ExceptionState& exception_state) {
  BiquadFilterNode* node = Create(*context, exception_state);

  if (!node)
    return nullptr;

  node->HandleChannelOptions(options, exception_state);

  node->setType(options.type());
  node->q()->setValue(options.Q());
  node->detune()->setValue(options.detune());
  node->frequency()->setValue(options.frequency());
  node->gain()->setValue(options.gain());

  return node;
}

void BiquadFilterNode::Trace(blink::Visitor* visitor) {
  visitor->Trace(frequency_);
  visitor->Trace(q_);
  visitor->Trace(gain_);
  visitor->Trace(detune_);
  AudioNode::Trace(visitor);
}

BiquadProcessor* BiquadFilterNode::GetBiquadProcessor() const {
  return static_cast<BiquadProcessor*>(
      static_cast<BiquadFilterHandler&>(Handler()).Processor());
}

String BiquadFilterNode::type() const {
  switch (
      const_cast<BiquadFilterNode*>(this)->GetBiquadProcessor()->GetType()) {
    case BiquadProcessor::kLowPass:
      return "lowpass";
    case BiquadProcessor::kHighPass:
      return "highpass";
    case BiquadProcessor::kBandPass:
      return "bandpass";
    case BiquadProcessor::kLowShelf:
      return "lowshelf";
    case BiquadProcessor::kHighShelf:
      return "highshelf";
    case BiquadProcessor::kPeaking:
      return "peaking";
    case BiquadProcessor::kNotch:
      return "notch";
    case BiquadProcessor::kAllpass:
      return "allpass";
    default:
      NOTREACHED();
      return "lowpass";
  }
}

void BiquadFilterNode::setType(const String& type) {
  // For the Q histogram, we need to change the name of the AudioParam for the
  // lowpass and highpass filters so we know to count the Q value when it is
  // set. And explicitly set the value to itself so the histograms know the
  // initial value.

  if (type == "lowpass") {
    setType(BiquadProcessor::kLowPass);
  } else if (type == "highpass") {
    setType(BiquadProcessor::kHighPass);
  } else if (type == "bandpass") {
    setType(BiquadProcessor::kBandPass);
  } else if (type == "lowshelf") {
    setType(BiquadProcessor::kLowShelf);
  } else if (type == "highshelf") {
    setType(BiquadProcessor::kHighShelf);
  } else if (type == "peaking") {
    setType(BiquadProcessor::kPeaking);
  } else if (type == "notch") {
    setType(BiquadProcessor::kNotch);
  } else if (type == "allpass") {
    setType(BiquadProcessor::kAllpass);
  }
}

bool BiquadFilterNode::setType(unsigned type) {
  if (type > BiquadProcessor::kAllpass)
    return false;

  DEFINE_STATIC_LOCAL(
      EnumerationHistogram, filter_type_histogram,
      ("WebAudio.BiquadFilter.Type", BiquadProcessor::kAllpass + 1));
  filter_type_histogram.Count(type);

  GetBiquadProcessor()->SetType(static_cast<BiquadProcessor::FilterType>(type));
  return true;
}

void BiquadFilterNode::getFrequencyResponse(
    NotShared<const DOMFloat32Array> frequency_hz,
    NotShared<DOMFloat32Array> mag_response,
    NotShared<DOMFloat32Array> phase_response,
    ExceptionState& exception_state) {
  unsigned frequency_hz_length = frequency_hz.View()->length();

  if (mag_response.View()->length() != frequency_hz_length) {
    exception_state.ThrowDOMException(
        kInvalidAccessError,
        ExceptionMessages::IndexOutsideRange(
            "magResponse length", mag_response.View()->length(),
            frequency_hz_length, ExceptionMessages::kInclusiveBound,
            frequency_hz_length, ExceptionMessages::kInclusiveBound));
    return;
  }

  if (phase_response.View()->length() != frequency_hz_length) {
    exception_state.ThrowDOMException(
        kInvalidAccessError,
        ExceptionMessages::IndexOutsideRange(
            "phaseResponse length", phase_response.View()->length(),
            frequency_hz_length, ExceptionMessages::kInclusiveBound,
            frequency_hz_length, ExceptionMessages::kInclusiveBound));
    return;
  }

  GetBiquadProcessor()->GetFrequencyResponse(
      frequency_hz_length, frequency_hz.View()->Data(),
      mag_response.View()->Data(), phase_response.View()->Data());
}

}  // namespace blink
