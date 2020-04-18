// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_BINDINGS_CORE_V8_SCRIPT_STREAMER_H_
#define THIRD_PARTY_BLINK_RENDERER_BINDINGS_CORE_V8_SCRIPT_STREAMER_H_

#include <memory>

#include "base/single_thread_task_runner.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/wtf/noncopyable.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"
#include "v8/include/v8.h"

namespace blink {

class ClassicPendingScript;
class ScriptResource;
class ScriptState;
class Settings;
class SourceStream;

// ScriptStreamer streams incomplete script data to V8 so that it can be parsed
// while it's loaded. ClassicPendingScript holds a reference to ScriptStreamer.
// At the moment, ScriptStreamer is only used for parser blocking scripts; this
// means that the Document stays stable and no other scripts are executing
// while we're streaming. It is possible, though, that Document and the
// ClassicPendingScript are destroyed while the streaming is in progress, and
// ScriptStreamer handles it gracefully.
class CORE_EXPORT ScriptStreamer final
    : public GarbageCollectedFinalized<ScriptStreamer> {
  WTF_MAKE_NONCOPYABLE(ScriptStreamer);

 public:
  enum Type { kParsingBlocking, kDeferred, kAsync };

  ~ScriptStreamer();
  void Trace(blink::Visitor*);

  // Launches a task (on a background thread) which will stream the given
  // ClassicPendingScript into V8 as it loads.
  static void StartStreaming(ClassicPendingScript*,
                             Type,
                             Settings*,
                             ScriptState*,
                             scoped_refptr<base::SingleThreadTaskRunner>);

  // Returns false if we cannot stream the given encoding.
  static bool ConvertEncoding(const char* encoding_name,
                              v8::ScriptCompiler::StreamedSource::Encoding*);

  bool IsFinished() const;           // Has loading & streaming finished?
  bool IsStreamingFinished() const;  // Has streaming finished?

  v8::ScriptCompiler::StreamedSource* Source() { return source_.get(); }

  // Called when the script is not needed any more (e.g., loading was
  // cancelled). After calling cancel, ClassicPendingScript can drop its
  // reference to ScriptStreamer, and ScriptStreamer takes care of eventually
  // deleting itself (after the V8 side has finished too).
  void Cancel();

  // When the streaming is suppressed, the data is not given to V8, but
  // ScriptStreamer still watches the resource load and notifies the upper
  // layers when loading is finished. It is used in situations when we have
  // started streaming but then we detect we don't want to stream (e.g., when
  // we have the code cache for the script) and we still want to parse and
  // execute it when it has finished loading.
  void SuppressStreaming();
  bool StreamingSuppressed() const { return streaming_suppressed_; }

  // Called by ClassicPendingScript when data arrives from the network.
  void NotifyAppendData(ScriptResource*);
  void NotifyFinished();

  // Called by ScriptStreamingTask when it has streamed all data to V8 and V8
  // has processed it.
  void StreamingCompleteOnBackgroundThread();

  const String& ScriptURLString() const { return script_url_string_; }
  unsigned long ScriptResourceIdentifier() const {
    return script_resource_identifier_;
  }

  static void SetSmallScriptThresholdForTesting(size_t threshold) {
    small_script_threshold_ = threshold;
  }

 private:
  // Scripts whose first data chunk is smaller than this constant won't be
  // streamed. Non-const for testing.
  static size_t small_script_threshold_;

  ScriptStreamer(ClassicPendingScript*,
                 Type,
                 ScriptState*,
                 v8::ScriptCompiler::CompileOptions,
                 scoped_refptr<base::SingleThreadTaskRunner>);

  void StreamingComplete();
  void NotifyFinishedToClient();

  Member<ClassicPendingScript> pending_script_;
  // Whether ScriptStreamer is detached from the Resource. In those cases, the
  // script data is not needed any more, and the client won't get notified
  // when the loading and streaming are done.
  bool detached_;

  SourceStream* stream_;
  std::unique_ptr<v8::ScriptCompiler::StreamedSource> source_;
  bool loading_finished_;  // Whether loading from the network is done.
  bool parsing_finished_;  // Whether the V8 side processing is done.
  // Whether we have received enough data to start the streaming.
  bool have_enough_data_for_streaming_;

  // Whether the script source code should be retrieved from the Resource
  // instead of the ScriptStreamer.
  bool streaming_suppressed_;

  // What kind of cached data V8 produces during streaming.
  v8::ScriptCompiler::CompileOptions compile_options_;

  scoped_refptr<ScriptState> script_state_;

  // For recording metrics for different types of scripts separately.
  Type script_type_;

  // Keep the script URL string for event tracing.
  const String script_url_string_;

  // Keep the script resource dentifier for event tracing.
  const unsigned long script_resource_identifier_;

  // Encoding of the streamed script. Saved for sanity checking purposes.
  v8::ScriptCompiler::StreamedSource::Encoding encoding_;

  scoped_refptr<base::SingleThreadTaskRunner> loading_task_runner_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_BINDINGS_CORE_V8_SCRIPT_STREAMER_H_
