/*
 * Copyright (C) 2013 Apple Computer, Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/modules/speech/testing/platform_speech_synthesizer_mock.h"

#include "third_party/blink/public/platform/task_type.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/platform/speech/platform_speech_synthesis_utterance.h"

namespace blink {

PlatformSpeechSynthesizerMock* PlatformSpeechSynthesizerMock::Create(
    PlatformSpeechSynthesizerClient* client,
    ExecutionContext* context) {
  PlatformSpeechSynthesizerMock* synthesizer =
      new PlatformSpeechSynthesizerMock(client, context);
  synthesizer->InitializeVoiceList();
  client->VoicesDidChange();
  return synthesizer;
}

PlatformSpeechSynthesizerMock::PlatformSpeechSynthesizerMock(
    PlatformSpeechSynthesizerClient* client,
    ExecutionContext* context)
    : PlatformSpeechSynthesizer(client),
      speaking_error_occurred_timer_(
          context->GetTaskRunner(TaskType::kInternalTest),
          this,
          &PlatformSpeechSynthesizerMock::SpeakingErrorOccurred),
      speaking_finished_timer_(
          context->GetTaskRunner(TaskType::kInternalTest),
          this,
          &PlatformSpeechSynthesizerMock::SpeakingFinished) {}

PlatformSpeechSynthesizerMock::~PlatformSpeechSynthesizerMock() = default;

void PlatformSpeechSynthesizerMock::SpeakingErrorOccurred(TimerBase*) {
  DCHECK(current_utterance_);

  Client()->SpeakingErrorOccurred(current_utterance_);
  SpeakNext();
}

void PlatformSpeechSynthesizerMock::SpeakingFinished(TimerBase*) {
  DCHECK(current_utterance_);
  Client()->DidFinishSpeaking(current_utterance_);
  SpeakNext();
}

void PlatformSpeechSynthesizerMock::SpeakNext() {
  if (speaking_error_occurred_timer_.IsActive())
    return;

  if (queued_utterances_.IsEmpty()) {
    current_utterance_ = nullptr;
    return;
  }
  current_utterance_ = queued_utterances_.TakeFirst();
  SpeakNow();
}

void PlatformSpeechSynthesizerMock::InitializeVoiceList() {
  voice_list_.clear();
  voice_list_.push_back(PlatformSpeechSynthesisVoice::Create(
      String("mock.voice.bruce"), String("bruce"), String("en-US"), true,
      true));
  voice_list_.push_back(PlatformSpeechSynthesisVoice::Create(
      String("mock.voice.clark"), String("clark"), String("en-US"), true,
      false));
  voice_list_.push_back(PlatformSpeechSynthesisVoice::Create(
      String("mock.voice.logan"), String("logan"), String("fr-CA"), true,
      true));
  voices_initialized_ = true;
}

void PlatformSpeechSynthesizerMock::Speak(
    PlatformSpeechSynthesisUtterance* utterance) {
  if (!current_utterance_) {
    current_utterance_ = utterance;
    SpeakNow();
    return;
  }
  queued_utterances_.push_back(utterance);
}

void PlatformSpeechSynthesizerMock::SpeakNow() {
  DCHECK(current_utterance_);
  Client()->DidStartSpeaking(current_utterance_);

  // Fire a fake word and then sentence boundary event.
  Client()->BoundaryEventOccurred(current_utterance_, kSpeechWordBoundary, 0);
  Client()->BoundaryEventOccurred(current_utterance_, kSpeechSentenceBoundary,
                                  current_utterance_->GetText().length());

  // Give the fake speech job some time so that pause and other functions have
  // time to be called.
  speaking_finished_timer_.StartOneShot(TimeDelta::FromMilliseconds(100),
                                        FROM_HERE);
}

void PlatformSpeechSynthesizerMock::Cancel() {
  if (!current_utterance_)
    return;

  // Per spec, removes all queued utterances.
  queued_utterances_.clear();

  speaking_finished_timer_.Stop();
  speaking_error_occurred_timer_.StartOneShot(TimeDelta::FromMilliseconds(100),
                                              FROM_HERE);
}

void PlatformSpeechSynthesizerMock::Pause() {
  if (!current_utterance_)
    return;

  Client()->DidPauseSpeaking(current_utterance_);
}

void PlatformSpeechSynthesizerMock::Resume() {
  if (!current_utterance_)
    return;

  Client()->DidResumeSpeaking(current_utterance_);
}

void PlatformSpeechSynthesizerMock::Trace(blink::Visitor* visitor) {
  visitor->Trace(current_utterance_);
  visitor->Trace(queued_utterances_);
  PlatformSpeechSynthesizer::Trace(visitor);
}

}  // namespace blink
