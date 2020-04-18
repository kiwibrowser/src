// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_WEBAUDIO_AUDIO_CONTEXT_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_WEBAUDIO_AUDIO_CONTEXT_H_

#include "third_party/blink/renderer/bindings/core/v8/script_promise.h"
#include "third_party/blink/renderer/bindings/core/v8/script_promise_resolver.h"
#include "third_party/blink/renderer/modules/webaudio/audio_context_options.h"
#include "third_party/blink/renderer/modules/webaudio/base_audio_context.h"
#include "third_party/blink/renderer/platform/heap/handle.h"

namespace blink {

class AudioContextOptions;
class AudioTimestamp;
class Document;
class ExceptionState;
class ScriptState;
class WebAudioLatencyHint;

// This is an BaseAudioContext which actually plays sound, unlike an
// OfflineAudioContext which renders sound into a buffer.
class MODULES_EXPORT AudioContext : public BaseAudioContext {
  DEFINE_WRAPPERTYPEINFO();

 public:
  static AudioContext* Create(Document&,
                              const AudioContextOptions&,
                              ExceptionState&);

  ~AudioContext() override;
  void Trace(blink::Visitor*) override;

  ScriptPromise closeContext(ScriptState*);
  bool IsContextClosed() const final;

  ScriptPromise suspendContext(ScriptState*) final;
  ScriptPromise resumeContext(ScriptState*) final;

  bool HasRealtimeConstraint() final { return true; }

  void getOutputTimestamp(ScriptState*, AudioTimestamp&);
  double baseLatency() const;

 protected:
  AudioContext(Document&, const WebAudioLatencyHint&);

  void DidClose() final;

 private:
  void StopRendering();

  unsigned context_id_;
  Member<ScriptPromiseResolver> close_resolver_;

};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_WEBAUDIO_AUDIO_CONTEXT_H_
