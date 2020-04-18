/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY GOOGLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL GOOGLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/modules/peerconnection/rtc_dtmf_sender.h"

#include <memory>

#include "third_party/blink/public/platform/task_type.h"
#include "third_party/blink/public/platform/web_media_stream_track.h"
#include "third_party/blink/public/platform/web_rtc_dtmf_sender_handler.h"
#include "third_party/blink/public/platform/web_rtc_peer_connection_handler.h"
#include "third_party/blink/renderer/bindings/core/v8/exception_messages.h"
#include "third_party/blink/renderer/bindings/core/v8/exception_state.h"
#include "third_party/blink/renderer/core/dom/exception_code.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/modules/mediastream/media_stream_track.h"
#include "third_party/blink/renderer/modules/peerconnection/rtc_dtmf_tone_change_event.h"

namespace blink {

static const int kMinToneDurationMs = 40;
static const int kDefaultToneDurationMs = 100;
static const int kMaxToneDurationMs = 6000;
// TODO(hta): Adjust kMinInterToneGapMs to 30 once WebRTC code has changed
// CL in progress: https://webrtc-review.googlesource.com/c/src/+/55260
static const int kMinInterToneGapMs = 50;
static const int kMaxInterToneGapMs = 6000;
static const int kDefaultInterToneGapMs = 70;

RTCDTMFSender* RTCDTMFSender::Create(
    ExecutionContext* context,
    std::unique_ptr<WebRTCDTMFSenderHandler> dtmf_sender_handler) {
  DCHECK(dtmf_sender_handler);
  return new RTCDTMFSender(context, std::move(dtmf_sender_handler));
}

RTCDTMFSender::RTCDTMFSender(ExecutionContext* context,
                             std::unique_ptr<WebRTCDTMFSenderHandler> handler)
    : ContextLifecycleObserver(context),
      track_(nullptr),
      duration_(kDefaultToneDurationMs),
      inter_tone_gap_(kDefaultInterToneGapMs),
      handler_(std::move(handler)),
      stopped_(false),
      scheduled_event_timer_(context->GetTaskRunner(TaskType::kNetworking),
                             this,
                             &RTCDTMFSender::ScheduledEventTimerFired) {
  handler_->SetClient(this);
}

RTCDTMFSender::~RTCDTMFSender() = default;

void RTCDTMFSender::Dispose() {
  // Promptly clears a raw reference from content/ to an on-heap object
  // so that content/ doesn't access it in a lazy sweeping phase.
  handler_->SetClient(nullptr);
  handler_.reset();
}

bool RTCDTMFSender::canInsertDTMF() const {
  return handler_->CanInsertDTMF();
}

MediaStreamTrack* RTCDTMFSender::track() const {
  return track_.Get();
}

void RTCDTMFSender::SetTrack(MediaStreamTrack* track) {
  track_ = track;
}

String RTCDTMFSender::toneBuffer() const {
  return handler_->CurrentToneBuffer();
}

void RTCDTMFSender::insertDTMF(const String& tones,
                               ExceptionState& exception_state) {
  insertDTMF(tones, kDefaultToneDurationMs, kDefaultInterToneGapMs,
             exception_state);
}

void RTCDTMFSender::insertDTMF(const String& tones,
                               int duration,
                               ExceptionState& exception_state) {
  insertDTMF(tones, duration, kDefaultInterToneGapMs, exception_state);
}

void RTCDTMFSender::insertDTMF(const String& tones,
                               int duration,
                               int inter_tone_gap,
                               ExceptionState& exception_state) {
  // https://w3c.github.io/webrtc-pc/#dom-rtcdtmfsender-insertdtmf
  // TODO(hta): Add check on transceiver's "stopped" and "currentDirection"
  // attributes
  if (!canInsertDTMF()) {
    exception_state.ThrowDOMException(kInvalidStateError,
                                      "The 'canInsertDTMF' attribute is false: "
                                      "this sender cannot send DTMF.");
    return;
  }
  // Spec: Throw on illegal characters
  if (strspn(tones.Ascii().data(), "0123456789abcdABCD#*,") != tones.length()) {
    exception_state.ThrowDOMException(
        kInvalidCharacterError,
        "Illegal characters in InsertDTMF tone argument");
    return;
  }

  // Spec: Clamp the duration to between 40 and 6000 ms
  duration = std::max(duration, kMinToneDurationMs);
  duration = std::min(duration, kMaxToneDurationMs);
  // Spec: Clamp the inter-tone gap to between 30 and 6000 ms
  inter_tone_gap = std::max(inter_tone_gap, kMinInterToneGapMs);
  inter_tone_gap = std::min(inter_tone_gap, kMaxInterToneGapMs);

  duration_ = duration;
  inter_tone_gap_ = inter_tone_gap;
  // Spec: a-d should be represented in the tone buffer as A-D
  if (!handler_->InsertDTMF(tones.UpperASCII(), duration_, inter_tone_gap_))
    exception_state.ThrowDOMException(
        kSyntaxError, "Could not send provided tones, '" + tones + "'.");
}

void RTCDTMFSender::DidPlayTone(const WebString& tone) {
  ScheduleDispatchEvent(RTCDTMFToneChangeEvent::Create(tone));
}

const AtomicString& RTCDTMFSender::InterfaceName() const {
  return EventTargetNames::RTCDTMFSender;
}

ExecutionContext* RTCDTMFSender::GetExecutionContext() const {
  return ContextLifecycleObserver::GetExecutionContext();
}

void RTCDTMFSender::ContextDestroyed(ExecutionContext*) {
  stopped_ = true;
  handler_->SetClient(nullptr);
}

void RTCDTMFSender::ScheduleDispatchEvent(Event* event) {
  scheduled_events_.push_back(event);

  if (!scheduled_event_timer_.IsActive())
    scheduled_event_timer_.StartOneShot(TimeDelta(), FROM_HERE);
}

void RTCDTMFSender::ScheduledEventTimerFired(TimerBase*) {
  if (stopped_)
    return;

  HeapVector<Member<Event>> events;
  events.swap(scheduled_events_);

  HeapVector<Member<Event>>::iterator it = events.begin();
  for (; it != events.end(); ++it)
    DispatchEvent((*it).Release());
}

void RTCDTMFSender::Trace(blink::Visitor* visitor) {
  visitor->Trace(track_);
  visitor->Trace(scheduled_events_);
  EventTargetWithInlineData::Trace(visitor);
  ContextLifecycleObserver::Trace(visitor);
}

}  // namespace blink
