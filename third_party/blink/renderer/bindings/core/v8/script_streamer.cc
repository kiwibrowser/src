// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/bindings/core/v8/script_streamer.h"

#include <memory>

#include "base/memory/ptr_util.h"
#include "third_party/blink/renderer/bindings/core/v8/script_streamer_thread.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_script_runner.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/element.h"
#include "third_party/blink/renderer/core/frame/settings.h"
#include "third_party/blink/renderer/core/html/parser/text_resource_decoder.h"
#include "third_party/blink/renderer/core/script/classic_pending_script.h"
#include "third_party/blink/renderer/platform/cross_thread_functional.h"
#include "third_party/blink/renderer/platform/histogram.h"
#include "third_party/blink/renderer/platform/instrumentation/tracing/trace_event.h"
#include "third_party/blink/renderer/platform/loader/fetch/cached_metadata.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource.h"
#include "third_party/blink/renderer/platform/scheduler/public/thread_scheduler.h"
#include "third_party/blink/renderer/platform/shared_buffer.h"
#include "third_party/blink/renderer/platform/wtf/deque.h"
#include "third_party/blink/renderer/platform/wtf/text/text_encoding_registry.h"

namespace blink {

namespace {

void RecordStartedStreamingHistogram(ScriptStreamer::Type script_type,
                                     int reason) {
  switch (script_type) {
    case ScriptStreamer::kParsingBlocking: {
      DEFINE_STATIC_LOCAL(
          EnumerationHistogram, parse_blocking_histogram,
          ("WebCore.Scripts.ParsingBlocking.StartedStreaming", 2));
      parse_blocking_histogram.Count(reason);
      break;
    }
    case ScriptStreamer::kDeferred: {
      DEFINE_STATIC_LOCAL(EnumerationHistogram, deferred_histogram,
                          ("WebCore.Scripts.Deferred.StartedStreaming", 2));
      deferred_histogram.Count(reason);
      break;
    }
    case ScriptStreamer::kAsync: {
      DEFINE_STATIC_LOCAL(EnumerationHistogram, async_histogram,
                          ("WebCore.Scripts.Async.StartedStreaming", 2));
      async_histogram.Count(reason);
      break;
    }
    default:
      NOTREACHED();
      break;
  }
}

// For tracking why some scripts are not streamed. Not streaming is part of
// normal operation (e.g., script already loaded, script too small) and doesn't
// necessarily indicate a failure.
enum NotStreamingReason {
  kAlreadyLoaded,  // DEPRECATED
  kNotHTTP,
  kReload,
  kContextNotValid,
  kEncodingNotSupported,
  kThreadBusy,
  kV8CannotStream,
  kScriptTooSmall,
  kNoResourceBuffer,
  kNotStreamingReasonEnd
};

void RecordNotStreamingReasonHistogram(ScriptStreamer::Type script_type,
                                       NotStreamingReason reason) {
  switch (script_type) {
    case ScriptStreamer::kParsingBlocking: {
      DEFINE_STATIC_LOCAL(EnumerationHistogram, parse_blocking_histogram,
                          ("WebCore.Scripts.ParsingBlocking.NotStreamingReason",
                           kNotStreamingReasonEnd));
      parse_blocking_histogram.Count(reason);
      break;
    }
    case ScriptStreamer::kDeferred: {
      DEFINE_STATIC_LOCAL(EnumerationHistogram, deferred_histogram,
                          ("WebCore.Scripts.Deferred.NotStreamingReason",
                           kNotStreamingReasonEnd));
      deferred_histogram.Count(reason);
      break;
    }
    case ScriptStreamer::kAsync: {
      DEFINE_STATIC_LOCAL(
          EnumerationHistogram, async_histogram,
          ("WebCore.Scripts.Async.NotStreamingReason", kNotStreamingReasonEnd));
      async_histogram.Count(reason);
      break;
    }
    default:
      NOTREACHED();
      break;
  }
}

}  // namespace

// For passing data between the main thread (producer) and the streamer thread
// (consumer). The main thread prepares the data (copies it from Resource) and
// the streamer thread feeds it to V8.
class SourceStreamDataQueue {
  WTF_MAKE_NONCOPYABLE(SourceStreamDataQueue);

 public:
  SourceStreamDataQueue() : finished_(false) {}

  ~SourceStreamDataQueue() { DiscardQueuedData(); }

  void Clear() {
    MutexLocker locker(mutex_);
    finished_ = false;
    DiscardQueuedData();
  }

  void Produce(const uint8_t* data, size_t length) {
    MutexLocker locker(mutex_);
    DCHECK(!finished_);
    data_.push_back(std::make_pair(data, length));
    have_data_.Signal();
  }

  void Finish() {
    MutexLocker locker(mutex_);
    finished_ = true;
    have_data_.Signal();
  }

  void Consume(const uint8_t** data, size_t* length) {
    MutexLocker locker(mutex_);
    while (!TryGetData(data, length))
      have_data_.Wait(mutex_);
  }

 private:
  bool TryGetData(const uint8_t** data, size_t* length) {
#if DCHECK_IS_ON()
    DCHECK(mutex_.Locked());
#endif
    if (!data_.IsEmpty()) {
      std::pair<const uint8_t*, size_t> next_data = data_.TakeFirst();
      *data = next_data.first;
      *length = next_data.second;
      return true;
    }
    if (finished_) {
      *length = 0;
      return true;
    }
    return false;
  }

  void DiscardQueuedData() {
    while (!data_.IsEmpty()) {
      std::pair<const uint8_t*, size_t> next_data = data_.TakeFirst();
      delete[] next_data.first;
    }
  }

  Deque<std::pair<const uint8_t*, size_t>> data_;
  bool finished_;
  Mutex mutex_;
  ThreadCondition have_data_;
};

// SourceStream implements the streaming interface towards V8. The main
// functionality is preparing the data to give to V8 on main thread, and
// actually giving the data (via GetMoreData which is called on a background
// thread).
class SourceStream : public v8::ScriptCompiler::ExternalSourceStream {
  WTF_MAKE_NONCOPYABLE(SourceStream);

 public:
  SourceStream()
      : v8::ScriptCompiler::ExternalSourceStream(),
        cancelled_(false),
        finished_(false),
        queue_lead_position_(0),
        queue_tail_position_(0) {}

  ~SourceStream() override = default;

  // Called by V8 on a background thread. Should block until we can return
  // some data.
  size_t GetMoreData(const uint8_t** src) override {
    DCHECK(!IsMainThread());
    {
      MutexLocker locker(mutex_);
      if (cancelled_)
        return 0;
    }
    size_t length = 0;
    // This will wait until there is data.
    data_queue_.Consume(src, &length);
    {
      MutexLocker locker(mutex_);
      if (cancelled_)
        return 0;
    }
    queue_lead_position_ += length;
    return length;
  }

  void DidFinishLoading() {
    DCHECK(IsMainThread());
    finished_ = true;
    data_queue_.Finish();
  }

  void DidReceiveData(ScriptResource* resource, ScriptStreamer* streamer) {
    DCHECK(IsMainThread());
    PrepareDataOnMainThread(resource, streamer);
  }

  void Cancel() {
    DCHECK(IsMainThread());
    // The script is no longer needed by the upper layers. Stop streaming
    // it. The next time GetMoreData is called (or woken up), it will return
    // 0, which will be interpreted as EOS by V8 and the parsing will
    // fail. ScriptStreamer::streamingComplete will be called, and at that
    // point we will release the references to SourceStream.
    {
      MutexLocker locker(mutex_);
      cancelled_ = true;
    }
    data_queue_.Finish();
  }

 private:
  void PrepareDataOnMainThread(ScriptResource* resource,
                               ScriptStreamer* streamer) {
    DCHECK(IsMainThread());

    if (cancelled_) {
      data_queue_.Finish();
      return;
    }

    // The Resource must still be alive; otherwise we should've cancelled
    // the streaming (if we have cancelled, the background thread is not
    // waiting).
    DCHECK(resource);

    if (!resource->GetResponse().CacheStorageCacheName().IsNull()) {
      streamer->SuppressStreaming();
      Cancel();
      return;
    }

    SingleCachedMetadataHandler* cache_handler = resource->CacheHandler();
    scoped_refptr<CachedMetadata> code_cache(
        cache_handler ? cache_handler->GetCachedMetadata(
                            V8ScriptRunner::TagForCodeCache(cache_handler))
                      : nullptr);
    if (code_cache.get()) {
      // The resource has a code cache, so it's unnecessary to stream and
      // parse the code. Cancel the streaming and resume the non-streaming
      // code path.
      streamer->SuppressStreaming();
      Cancel();
      return;
    }

    if (!resource_buffer_) {
      // We don't have a buffer yet. Try to get it from the resource.
      resource_buffer_ = resource->ResourceBuffer();
    }

    FetchDataFromResourceBuffer();
  }

  void FetchDataFromResourceBuffer() {
    DCHECK(IsMainThread());
    MutexLocker locker(mutex_);

    DCHECK(!finished_);
    if (cancelled_) {
      data_queue_.Finish();
      return;
    }

    // Get as much data from the ResourceBuffer as we can.
    const char* data = nullptr;
    while (size_t length =
               resource_buffer_->GetSomeData(data, queue_tail_position_)) {
      // Copy the data chunks into a new buffer, since we're going to
      // give the data to a background thread.
      uint8_t* copied_data = new uint8_t[length];
      memcpy(copied_data, data, length);
      data_queue_.Produce(copied_data, length);

      queue_tail_position_ += length;
    }
  }

  // For coordinating between the main thread and background thread tasks.
  // Guards m_cancelled and m_queueTailPosition.
  Mutex mutex_;

  // The shared buffer containing the resource data + state variables.
  // Used by both threads, guarded by m_mutex.
  bool cancelled_;
  bool finished_;

  scoped_refptr<const SharedBuffer>
      resource_buffer_;  // Only used by the main thread.

  // The queue contains the data to be passed to the V8 thread.
  //   queueLeadPosition: data we have handed off to the V8 thread.
  //   queueTailPosition: end of data we have enqued in the queue.
  //   bookmarkPosition: position of the bookmark.
  SourceStreamDataQueue data_queue_;  // Thread safe.
  size_t queue_lead_position_;        // Only used by v8 thread.
  size_t queue_tail_position_;  // Used by both threads; guarded by m_mutex.
};

size_t ScriptStreamer::small_script_threshold_ = 30 * 1024;

bool ScriptStreamer::ConvertEncoding(
    const char* encoding_name,
    v8::ScriptCompiler::StreamedSource::Encoding* encoding) {
  // Here's a list of encodings we can use for streaming. These are
  // the canonical names.
  if (strcmp(encoding_name, "windows-1252") == 0 ||
      strcmp(encoding_name, "ISO-8859-1") == 0 ||
      strcmp(encoding_name, "US-ASCII") == 0) {
    *encoding = v8::ScriptCompiler::StreamedSource::ONE_BYTE;
    return true;
  }
  if (strcmp(encoding_name, "UTF-8") == 0) {
    *encoding = v8::ScriptCompiler::StreamedSource::UTF8;
    return true;
  }
  // We don't stream other encodings; especially we don't stream two
  // byte scripts to avoid the handling of endianness. Most scripts
  // are Latin1 or UTF-8 anyway, so this should be enough for most
  // real world purposes.
  return false;
}

bool ScriptStreamer::IsFinished() const {
  DCHECK(IsMainThread());
  return loading_finished_ && (parsing_finished_ || streaming_suppressed_);
}

bool ScriptStreamer::IsStreamingFinished() const {
  DCHECK(IsMainThread());
  return parsing_finished_ || streaming_suppressed_;
}

void ScriptStreamer::StreamingCompleteOnBackgroundThread() {
  DCHECK(!IsMainThread());

  // notifyFinished might already be called, or it might be called in the
  // future (if the parsing finishes earlier because of a parse error).
  PostCrossThreadTask(*loading_task_runner_, FROM_HERE,
                      CrossThreadBind(&ScriptStreamer::StreamingComplete,
                                      WrapCrossThreadPersistent(this)));

  // The task might delete ScriptStreamer, so it's not safe to do anything
  // after posting it. Note that there's no way to guarantee that this
  // function has returned before the task is ran - however, we should not
  // access the "this" object after posting the task.
}

void ScriptStreamer::Cancel() {
  DCHECK(IsMainThread());
  // The upper layer doesn't need the script any more, but streaming might
  // still be ongoing. Tell SourceStream to try to cancel it whenever it gets
  // the control the next time. It can also be that V8 has already completed
  // its operations and streamingComplete will be called soon.
  detached_ = true;
  if (stream_)
    stream_->Cancel();
}

void ScriptStreamer::SuppressStreaming() {
  DCHECK(IsMainThread());
  DCHECK(!loading_finished_);
  // It can be that the parsing task has already finished (e.g., if there was
  // a parse error).
  streaming_suppressed_ = true;
}

void ScriptStreamer::NotifyAppendData(ScriptResource* resource) {
  DCHECK(IsMainThread());
  if (streaming_suppressed_)
    return;
  if (!have_enough_data_for_streaming_) {
    // Even if the first data chunk is small, the script can still be big
    // enough - wait until the next data chunk comes before deciding whether
    // to start the streaming.
    DCHECK(resource->ResourceBuffer());
    if (resource->ResourceBuffer()->size() < small_script_threshold_)
      return;
    have_enough_data_for_streaming_ = true;

    {
      // Check for BOM (byte order marks), because that might change our
      // understanding of the data encoding.
      constexpr size_t kMaximumLengthOfBOM = 4;
      char maybe_bom[kMaximumLengthOfBOM] = {};
      if (!resource->ResourceBuffer()->GetBytes(maybe_bom,
                                                kMaximumLengthOfBOM)) {
        NOTREACHED();
        return;
      }

      std::unique_ptr<TextResourceDecoder> decoder(
          TextResourceDecoder::Create(TextResourceDecoderOptions(
              TextResourceDecoderOptions::kPlainTextContent,
              WTF::TextEncoding(resource->Encoding()))));
      decoder->CheckForBOM(maybe_bom, kMaximumLengthOfBOM);

      // The encoding may change when we see the BOM. Check for BOM now
      // and update the encoding from the decoder when necessary. Supress
      // streaming if the encoding is unsupported.
      //
      // Also note that have at least s_smallScriptThreshold worth of
      // data, which is more than enough for detecting a BOM.
      if (!ConvertEncoding(decoder->Encoding().GetName(), &encoding_)) {
        SuppressStreaming();
        RecordNotStreamingReasonHistogram(script_type_, kEncodingNotSupported);
        RecordStartedStreamingHistogram(script_type_, 0);
        return;
      }
    }

    if (ScriptStreamerThread::Shared()->IsRunningTask()) {
      // At the moment we only have one thread for running the tasks. A
      // new task shouldn't be queued before the running task completes,
      // because the running task can block and wait for data from the
      // network.
      SuppressStreaming();
      RecordNotStreamingReasonHistogram(script_type_, kThreadBusy);
      RecordStartedStreamingHistogram(script_type_, 0);
      return;
    }

    if (!script_state_->ContextIsValid()) {
      SuppressStreaming();
      RecordNotStreamingReasonHistogram(script_type_, kContextNotValid);
      RecordStartedStreamingHistogram(script_type_, 0);
      return;
    }

    DCHECK(!stream_);
    DCHECK(!source_);
    stream_ = new SourceStream;
    // m_source takes ownership of m_stream.
    source_ = std::make_unique<v8::ScriptCompiler::StreamedSource>(stream_,
                                                                   encoding_);

    ScriptState::Scope scope(script_state_.get());
    std::unique_ptr<v8::ScriptCompiler::ScriptStreamingTask>
        script_streaming_task(
            base::WrapUnique(v8::ScriptCompiler::StartStreamingScript(
                script_state_->GetIsolate(), source_.get(), compile_options_)));
    if (!script_streaming_task) {
      // V8 cannot stream the script.
      SuppressStreaming();
      stream_ = nullptr;
      source_.reset();
      RecordNotStreamingReasonHistogram(script_type_, kV8CannotStream);
      RecordStartedStreamingHistogram(script_type_, 0);
      return;
    }

    ScriptStreamerThread::Shared()->PostTask(
        CrossThreadBind(&ScriptStreamerThread::RunScriptStreamingTask,
                        WTF::Passed(std::move(script_streaming_task)),
                        WrapCrossThreadPersistent(this)));
    RecordStartedStreamingHistogram(script_type_, 1);
  }
  if (stream_)
    stream_->DidReceiveData(resource, this);
}

void ScriptStreamer::NotifyFinished() {
  DCHECK(IsMainThread());
  // A special case: empty and small scripts. We didn't receive enough data to
  // start the streaming before this notification. In that case, there won't
  // be a "parsing complete" notification either, and we should not wait for
  // it.
  if (!have_enough_data_for_streaming_) {
    RecordNotStreamingReasonHistogram(script_type_, kScriptTooSmall);
    RecordStartedStreamingHistogram(script_type_, 0);
    SuppressStreaming();
  }
  if (stream_)
    stream_->DidFinishLoading();
  loading_finished_ = true;

  NotifyFinishedToClient();
}

ScriptStreamer::ScriptStreamer(
    ClassicPendingScript* script,
    Type script_type,
    ScriptState* script_state,
    v8::ScriptCompiler::CompileOptions compile_options,
    scoped_refptr<base::SingleThreadTaskRunner> loading_task_runner)
    : pending_script_(script),
      detached_(false),
      stream_(nullptr),
      loading_finished_(false),
      parsing_finished_(false),
      have_enough_data_for_streaming_(false),
      streaming_suppressed_(false),
      compile_options_(compile_options),
      script_state_(script_state),
      script_type_(script_type),
      script_url_string_(script->GetResource()->Url().Copy().GetString()),
      script_resource_identifier_(script->GetResource()->Identifier()),
      // Unfortunately there's no dummy encoding value in the enum; let's use
      // one we don't stream.
      encoding_(v8::ScriptCompiler::StreamedSource::TWO_BYTE),
      loading_task_runner_(std::move(loading_task_runner)) {}

ScriptStreamer::~ScriptStreamer() = default;

void ScriptStreamer::Trace(blink::Visitor* visitor) {
  visitor->Trace(pending_script_);
}

void ScriptStreamer::StreamingComplete() {
  // The background task is completed; do the necessary ramp-down in the main
  // thread.
  DCHECK(IsMainThread());
  parsing_finished_ = true;

  // It's possible that the corresponding Resource was deleted before V8
  // finished streaming. In that case, the data or the notification is not
  // needed. In addition, if the streaming is suppressed, the non-streaming
  // code path will resume after the resource has loaded, before the
  // background task finishes.
  if (detached_ || streaming_suppressed_)
    return;

  // We have now streamed the whole script to V8 and it has parsed the
  // script. We're ready for the next step: compiling and executing the
  // script.
  NotifyFinishedToClient();
}

void ScriptStreamer::NotifyFinishedToClient() {
  DCHECK(IsMainThread());
  // Usually, the loading will be finished first, and V8 will still need some
  // time to catch up. But the other way is possible too: if V8 detects a
  // parse error, the V8 side can complete before loading has finished. Send
  // the notification after both loading and V8 side operations have
  // completed.
  if (!IsFinished())
    return;

  pending_script_->StreamingFinished();
}

void ScriptStreamer::StartStreaming(
    ClassicPendingScript* script,
    Type script_type,
    Settings* settings,
    ScriptState* script_state,
    scoped_refptr<base::SingleThreadTaskRunner> loading_task_runner) {
  DCHECK(IsMainThread());
  DCHECK(script_state->ContextIsValid());
  ScriptResource* resource = ToScriptResource(script->GetResource());
  if (!resource->Url().ProtocolIsInHTTPFamily()) {
    RecordNotStreamingReasonHistogram(script_type, kNotHTTP);
    RecordStartedStreamingHistogram(script_type, 0);
    return;
  }
  if (resource->IsCacheValidator()) {
    RecordNotStreamingReasonHistogram(script_type, kReload);
    RecordStartedStreamingHistogram(script_type, 0);
    // This happens e.g., during reloads. We're actually not going to load
    // the current Resource of the ClassicPendingScript but switch to another
    // Resource -> don't stream.
    return;
  }
  if (resource->IsLoaded() && !resource->ResourceBuffer()) {
    // This happens for already loaded resources, e.g. if resource
    // validation fails. In that case, the loading subsystem will discard
    // the resource buffer.
    RecordNotStreamingReasonHistogram(script_type, kNoResourceBuffer);
    RecordStartedStreamingHistogram(script_type, 0);
    return;
  }
  // We cannot filter out short scripts, even if we wait for the HTTP headers
  // to arrive: the Content-Length HTTP header is not sent for chunked
  // downloads.

  ScriptStreamer* streamer = new ScriptStreamer(
      script, script_type, script_state, v8::ScriptCompiler::kNoCompileOptions,
      std::move(loading_task_runner));

  // If this script was ready when streaming began, no callbacks will be
  // received to populate the data for the ScriptStreamer, so send them now.
  // Note that this script may be processing an asynchronous cache hit, in
  // which case ScriptResource::IsLoaded() will be true, but ready_state_ will
  // not be kReadyStreaming. In that case, ScriptStreamer can listen to the
  // async callbacks generated by the cache hit.
  if (script->IsReady()) {
    DCHECK(resource->IsLoaded());
    streamer->NotifyAppendData(resource);
    if (streamer->StreamingSuppressed())
      return;
  }

  // The Resource might go out of scope if the script is no longer needed.
  // This makes ClassicPendingScript notify the ScriptStreamer when it is
  // destroyed.
  script->SetStreamer(streamer);

  if (script->IsReady())
    streamer->NotifyFinished();
}

}  // namespace blink
