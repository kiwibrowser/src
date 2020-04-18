// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/mediarecorder/media_recorder.h"

#include <algorithm>
#include <limits>
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/public/platform/task_type.h"
#include "third_party/blink/public/platform/web_media_stream.h"
#include "third_party/blink/renderer/bindings/core/v8/dictionary.h"
#include "third_party/blink/renderer/core/dom/events/event.h"
#include "third_party/blink/renderer/core/fileapi/blob.h"
#include "third_party/blink/renderer/core/inspector/console_message.h"
#include "third_party/blink/renderer/modules/event_target_modules.h"
#include "third_party/blink/renderer/modules/mediarecorder/blob_event.h"
#include "third_party/blink/renderer/platform/blob/blob_data.h"
#include "third_party/blink/renderer/platform/network/mime/content_type.h"
#include "third_party/blink/renderer/platform/wtf/time.h"

namespace blink {

namespace {

const char kDefaultMimeType[] = "video/webm";

// Boundaries of Opus bitrate from https://www.opus-codec.org/.
const int kSmallestPossibleOpusBitRate = 6000;
const int kLargestAutoAllocatedOpusBitRate = 128000;

// Smallest Vpx bitrate that can be requested.
const int kSmallestPossibleVpxBitRate = 100000;

String StateToString(MediaRecorder::State state) {
  switch (state) {
    case MediaRecorder::State::kInactive:
      return "inactive";
    case MediaRecorder::State::kRecording:
      return "recording";
    case MediaRecorder::State::kPaused:
      return "paused";
  }

  NOTREACHED();
  return String();
}

// Allocates the requested bit rates from |bitrateOptions| into the respective
// |{audio,video}BitsPerSecond| (where a value of zero indicates Platform to use
// whatever it sees fit). If |options.bitsPerSecond()| is specified, it
// overrides any specific bitrate, and the UA is free to allocate as desired:
// here a 90%/10% video/audio is used. In all cases where a value is explicited
// or calculated, values are clamped in sane ranges.
// This method throws NotSupportedError.
void AllocateVideoAndAudioBitrates(ExceptionState& exception_state,
                                   ExecutionContext* context,
                                   const MediaRecorderOptions& options,
                                   MediaStream* stream,
                                   int* audio_bits_per_second,
                                   int* video_bits_per_second) {
  const bool use_video = !stream->getVideoTracks().IsEmpty();
  const bool use_audio = !stream->getAudioTracks().IsEmpty();

  // Clamp incoming values into a signed integer's range.
  // TODO(mcasas): This section would no be needed if the bit rates are signed
  // or double, see https://github.com/w3c/mediacapture-record/issues/48.
  const unsigned kMaxIntAsUnsigned = std::numeric_limits<int>::max();

  int overall_bps = 0;
  if (options.hasBitsPerSecond())
    overall_bps = std::min(options.bitsPerSecond(), kMaxIntAsUnsigned);
  int video_bps = 0;
  if (options.hasVideoBitsPerSecond() && use_video)
    video_bps = std::min(options.videoBitsPerSecond(), kMaxIntAsUnsigned);
  int audio_bps = 0;
  if (options.hasAudioBitsPerSecond() && use_audio)
    audio_bps = std::min(options.audioBitsPerSecond(), kMaxIntAsUnsigned);

  if (use_audio) {
    // |overallBps| overrides the specific audio and video bit rates.
    if (options.hasBitsPerSecond()) {
      if (use_video)
        audio_bps = overall_bps / 10;
      else
        audio_bps = overall_bps;
    }
    // Limit audio bitrate values if set explicitly or calculated.
    if (options.hasAudioBitsPerSecond() || options.hasBitsPerSecond()) {
      if (audio_bps > kLargestAutoAllocatedOpusBitRate) {
        context->AddConsoleMessage(ConsoleMessage::Create(
            kJSMessageSource, kWarningMessageLevel,
            "Clamping calculated audio bitrate (" + String::Number(audio_bps) +
                "bps) to the maximum (" +
                String::Number(kLargestAutoAllocatedOpusBitRate) + "bps)"));
        audio_bps = kLargestAutoAllocatedOpusBitRate;
      }

      if (audio_bps < kSmallestPossibleOpusBitRate) {
        context->AddConsoleMessage(ConsoleMessage::Create(
            kJSMessageSource, kWarningMessageLevel,
            "Clamping calculated audio bitrate (" + String::Number(audio_bps) +
                "bps) to the minimum (" +
                String::Number(kSmallestPossibleOpusBitRate) + "bps)"));
        audio_bps = kSmallestPossibleOpusBitRate;
      }
    } else {
      DCHECK(!audio_bps);
    }
  }

  if (use_video) {
    // Allocate the remaining |overallBps|, if any, to video.
    if (options.hasBitsPerSecond())
      video_bps = overall_bps - audio_bps;
    // Clamp the video bit rate. Avoid clamping if the user has not set it
    // explicitly.
    if (options.hasVideoBitsPerSecond() || options.hasBitsPerSecond()) {
      if (video_bps < kSmallestPossibleVpxBitRate) {
        context->AddConsoleMessage(ConsoleMessage::Create(
            kJSMessageSource, kWarningMessageLevel,
            "Clamping calculated video bitrate (" + String::Number(video_bps) +
                "bps) to the minimum (" +
                String::Number(kSmallestPossibleVpxBitRate) + "bps)"));
        video_bps = kSmallestPossibleVpxBitRate;
      }
    } else {
      DCHECK(!video_bps);
    }
  }

  *video_bits_per_second = video_bps;
  *audio_bits_per_second = audio_bps;
  return;
}

}  // namespace

MediaRecorder* MediaRecorder::Create(ExecutionContext* context,
                                     MediaStream* stream,
                                     ExceptionState& exception_state) {
  MediaRecorder* recorder = new MediaRecorder(
      context, stream, MediaRecorderOptions(), exception_state);
  recorder->PauseIfNeeded();

  return recorder;
}

MediaRecorder* MediaRecorder::Create(ExecutionContext* context,
                                     MediaStream* stream,
                                     const MediaRecorderOptions& options,
                                     ExceptionState& exception_state) {
  MediaRecorder* recorder =
      new MediaRecorder(context, stream, options, exception_state);
  recorder->PauseIfNeeded();

  return recorder;
}

MediaRecorder::MediaRecorder(ExecutionContext* context,
                             MediaStream* stream,
                             const MediaRecorderOptions& options,
                             ExceptionState& exception_state)
    : PausableObject(context),
      stream_(stream),
      mime_type_(options.hasMimeType() ? options.mimeType() : kDefaultMimeType),
      stopped_(true),
      audio_bits_per_second_(0),
      video_bits_per_second_(0),
      state_(State::kInactive),
      // MediaStream recording should use DOM manipulation task source.
      // https://www.w3.org/TR/mediastream-recording/
      dispatch_scheduled_event_runner_(AsyncMethodRunner<MediaRecorder>::Create(
          this,
          &MediaRecorder::DispatchScheduledEvent,
          context->GetTaskRunner(TaskType::kDOMManipulation))) {
  DCHECK(stream_->getTracks().size());

  recorder_handler_ = Platform::Current()->CreateMediaRecorderHandler(
      context->GetTaskRunner(TaskType::kInternalMediaRealTime));
  DCHECK(recorder_handler_);

  if (!recorder_handler_) {
    exception_state.ThrowDOMException(
        kNotSupportedError, "No MediaRecorder handler can be created.");
    return;
  }

  AllocateVideoAndAudioBitrates(exception_state, context, options, stream,
                                &audio_bits_per_second_,
                                &video_bits_per_second_);

  const ContentType content_type(mime_type_);
  if (!recorder_handler_->Initialize(
          this, stream->Descriptor(), content_type.GetType(),
          content_type.Parameter("codecs"), audio_bits_per_second_,
          video_bits_per_second_)) {
    exception_state.ThrowDOMException(
        kNotSupportedError,
        "Failed to initialize native MediaRecorder the type provided (" +
            mime_type_ + ") is not supported.");
    return;
  }
  stopped_ = false;
}

String MediaRecorder::state() const {
  return StateToString(state_);
}

void MediaRecorder::start(ExceptionState& exception_state) {
  start(std::numeric_limits<int>::max() /* timeSlice */, exception_state);
}

void MediaRecorder::start(int time_slice, ExceptionState& exception_state) {
  if (state_ != State::kInactive) {
    exception_state.ThrowDOMException(
        kInvalidStateError,
        "The MediaRecorder's state is '" + StateToString(state_) + "'.");
    return;
  }
  state_ = State::kRecording;

  if (!recorder_handler_->Start(time_slice)) {
    exception_state.ThrowDOMException(kUnknownError,
                                      "The MediaRecorder failed to start "
                                      "because there are no audio or video "
                                      "tracks available.");
    return;
  }
  ScheduleDispatchEvent(Event::Create(EventTypeNames::start));
}

void MediaRecorder::stop(ExceptionState& exception_state) {
  if (state_ == State::kInactive) {
    exception_state.ThrowDOMException(
        kInvalidStateError,
        "The MediaRecorder's state is '" + StateToString(state_) + "'.");
    return;
  }

  StopRecording();
}

void MediaRecorder::pause(ExceptionState& exception_state) {
  if (state_ == State::kInactive) {
    exception_state.ThrowDOMException(
        kInvalidStateError,
        "The MediaRecorder's state is '" + StateToString(state_) + "'.");
    return;
  }
  if (state_ == State::kPaused)
    return;

  state_ = State::kPaused;

  recorder_handler_->Pause();

  ScheduleDispatchEvent(Event::Create(EventTypeNames::pause));
}

void MediaRecorder::resume(ExceptionState& exception_state) {
  if (state_ == State::kInactive) {
    exception_state.ThrowDOMException(
        kInvalidStateError,
        "The MediaRecorder's state is '" + StateToString(state_) + "'.");
    return;
  }
  if (state_ == State::kRecording)
    return;

  state_ = State::kRecording;

  recorder_handler_->Resume();
  ScheduleDispatchEvent(Event::Create(EventTypeNames::resume));
}

void MediaRecorder::requestData(ExceptionState& exception_state) {
  if (state_ == State::kInactive) {
    exception_state.ThrowDOMException(
        kInvalidStateError,
        "The MediaRecorder's state is '" + StateToString(state_) + "'.");
    return;
  }
  WriteData(nullptr /* data */, 0 /* length */, true /* lastInSlice */,
            WTF::CurrentTimeMS());
}

bool MediaRecorder::isTypeSupported(ExecutionContext* context,
                                    const String& type) {
  std::unique_ptr<WebMediaRecorderHandler> handler =
      Platform::Current()->CreateMediaRecorderHandler(
          context->GetTaskRunner(TaskType::kInternalMediaRealTime));
  if (!handler)
    return false;

  // If true is returned from this method, it only indicates that the
  // MediaRecorder implementation is capable of recording Blob objects for the
  // specified MIME type. Recording may still fail if sufficient resources are
  // not available to support the concrete media encoding.
  // https://w3c.github.io/mediacapture-record/#dom-mediarecorder-istypesupported
  ContentType content_type(type);
  return handler->CanSupportMimeType(content_type.GetType(),
                                     content_type.Parameter("codecs"));
}

const AtomicString& MediaRecorder::InterfaceName() const {
  return EventTargetNames::MediaRecorder;
}

ExecutionContext* MediaRecorder::GetExecutionContext() const {
  return PausableObject::GetExecutionContext();
}

void MediaRecorder::Pause() {
  dispatch_scheduled_event_runner_->Pause();
}

void MediaRecorder::Unpause() {
  dispatch_scheduled_event_runner_->Unpause();
}

void MediaRecorder::ContextDestroyed(ExecutionContext*) {
  if (stopped_)
    return;

  stopped_ = true;
  stream_.Clear();
  recorder_handler_.reset();
}

void MediaRecorder::WriteData(const char* data,
                              size_t length,
                              bool last_in_slice,
                              double timecode) {
  if (stopped_ && !last_in_slice) {
    stopped_ = false;
    ScheduleDispatchEvent(Event::Create(EventTypeNames::start));
  }

  if (!blob_data_) {
    blob_data_ = BlobData::Create();
    blob_data_->SetContentType(mime_type_);
  }
  if (data)
    blob_data_->AppendBytes(data, length);

  if (!last_in_slice)
    return;

  // Cache |m_blobData->length()| before release()ng it.
  const long long blob_data_length = blob_data_->length();
  CreateBlobEvent(Blob::Create(BlobDataHandle::Create(std::move(blob_data_),
                                                      blob_data_length)),
                  timecode);
}

void MediaRecorder::OnError(const WebString& message) {
  DLOG(ERROR) << message.Ascii();
  StopRecording();
  ScheduleDispatchEvent(Event::Create(EventTypeNames::error));
}

void MediaRecorder::CreateBlobEvent(Blob* blob, double timecode) {
  ScheduleDispatchEvent(
      BlobEvent::Create(EventTypeNames::dataavailable, blob, timecode));
}

void MediaRecorder::StopRecording() {
  DCHECK(state_ != State::kInactive);
  state_ = State::kInactive;

  recorder_handler_->Stop();

  WriteData(nullptr /* data */, 0 /* length */, true /* lastInSlice */,
            WTF::CurrentTimeMS());
  ScheduleDispatchEvent(Event::Create(EventTypeNames::stop));
}

void MediaRecorder::ScheduleDispatchEvent(Event* event) {
  scheduled_events_.push_back(event);

  dispatch_scheduled_event_runner_->RunAsync();
}

void MediaRecorder::DispatchScheduledEvent() {
  HeapVector<Member<Event>> events;
  events.swap(scheduled_events_);

  for (const auto& event : events)
    DispatchEvent(event);
}

void MediaRecorder::Trace(blink::Visitor* visitor) {
  visitor->Trace(stream_);
  visitor->Trace(dispatch_scheduled_event_runner_);
  visitor->Trace(scheduled_events_);
  EventTargetWithInlineData::Trace(visitor);
  PausableObject::Trace(visitor);
}

}  // namespace blink
