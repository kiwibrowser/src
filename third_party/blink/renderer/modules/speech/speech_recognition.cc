/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/modules/speech/speech_recognition.h"

#include "third_party/blink/renderer/bindings/core/v8/exception_state.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/exception_code.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/modules/speech/speech_recognition_controller.h"
#include "third_party/blink/renderer/modules/speech/speech_recognition_error.h"
#include "third_party/blink/renderer/modules/speech/speech_recognition_event.h"

namespace blink {

SpeechRecognition* SpeechRecognition::Create(ExecutionContext* context) {
  DCHECK(context);
  DCHECK(context->IsDocument());
  Document* document = ToDocument(context);
  DCHECK(document);
  return new SpeechRecognition(document->GetFrame(), context);
}

void SpeechRecognition::start(ExceptionState& exception_state) {
  if (!controller_)
    return;

  if (started_) {
    exception_state.ThrowDOMException(kInvalidStateError,
                                      "recognition has already started.");
    return;
  }

  final_results_.clear();
  controller_->Start(this, grammars_, lang_, continuous_, interim_results_,
                     max_alternatives_);
  started_ = true;
}

void SpeechRecognition::stopFunction() {
  if (!controller_)
    return;

  if (started_ && !stopping_) {
    stopping_ = true;
    controller_->Stop(this);
  }
}

void SpeechRecognition::abort() {
  if (!controller_)
    return;

  if (started_ && !stopping_) {
    stopping_ = true;
    controller_->Abort(this);
  }
}

void SpeechRecognition::DidStartAudio() {
  DispatchEvent(Event::Create(EventTypeNames::audiostart));
}

void SpeechRecognition::DidStartSound() {
  DispatchEvent(Event::Create(EventTypeNames::soundstart));
}

void SpeechRecognition::DidStartSpeech() {
  DispatchEvent(Event::Create(EventTypeNames::speechstart));
}

void SpeechRecognition::DidEndSpeech() {
  DispatchEvent(Event::Create(EventTypeNames::speechend));
}

void SpeechRecognition::DidEndSound() {
  DispatchEvent(Event::Create(EventTypeNames::soundend));
}

void SpeechRecognition::DidEndAudio() {
  DispatchEvent(Event::Create(EventTypeNames::audioend));
}

void SpeechRecognition::DidReceiveResults(
    const HeapVector<Member<SpeechRecognitionResult>>& new_final_results,
    const HeapVector<Member<SpeechRecognitionResult>>&
        current_interim_results) {
  size_t result_index = final_results_.size();

  for (size_t i = 0; i < new_final_results.size(); ++i)
    final_results_.push_back(new_final_results[i]);

  HeapVector<Member<SpeechRecognitionResult>> results = final_results_;
  for (size_t i = 0; i < current_interim_results.size(); ++i)
    results.push_back(current_interim_results[i]);

  DispatchEvent(SpeechRecognitionEvent::CreateResult(result_index, results));
}

void SpeechRecognition::DidReceiveNoMatch(SpeechRecognitionResult* result) {
  DispatchEvent(SpeechRecognitionEvent::CreateNoMatch(result));
}

void SpeechRecognition::DidReceiveError(SpeechRecognitionError* error) {
  DispatchEvent(error);
  started_ = false;
}

void SpeechRecognition::DidStart() {
  DispatchEvent(Event::Create(EventTypeNames::start));
}

void SpeechRecognition::DidEnd() {
  started_ = false;
  stopping_ = false;
  // If m_controller is null, this is being aborted from the ExecutionContext
  // being detached, so don't dispatch an event.
  if (controller_)
    DispatchEvent(Event::Create(EventTypeNames::end));
}

const AtomicString& SpeechRecognition::InterfaceName() const {
  return EventTargetNames::SpeechRecognition;
}

ExecutionContext* SpeechRecognition::GetExecutionContext() const {
  return ContextLifecycleObserver::GetExecutionContext();
}

void SpeechRecognition::ContextDestroyed(ExecutionContext*) {
  controller_ = nullptr;
  if (HasPendingActivity())
    abort();
}

bool SpeechRecognition::HasPendingActivity() const {
  return started_;
}

SpeechRecognition::SpeechRecognition(LocalFrame* frame,
                                     ExecutionContext* context)
    : ContextLifecycleObserver(context),
      grammars_(SpeechGrammarList::Create()),  // FIXME: The spec is not clear
                                               // on the default value for the
                                               // grammars attribute.
      continuous_(false),
      interim_results_(false),
      max_alternatives_(1),
      controller_(SpeechRecognitionController::From(frame)),
      started_(false),
      stopping_(false) {
  // FIXME: Need to hook up to get notified when the visibility changes.
}

SpeechRecognition::~SpeechRecognition() = default;

void SpeechRecognition::Trace(blink::Visitor* visitor) {
  visitor->Trace(grammars_);
  visitor->Trace(controller_);
  visitor->Trace(final_results_);
  EventTargetWithInlineData::Trace(visitor);
  ContextLifecycleObserver::Trace(visitor);
}

}  // namespace blink
