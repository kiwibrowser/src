// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_SPEECH_RECOGNITION_DISPATCHER_H_
#define CONTENT_RENDERER_SPEECH_RECOGNITION_DISPATCHER_H_

#include <map>
#include <memory>

#include "base/macros.h"
#include "content/common/speech_recognizer.mojom.h"
#include "content/public/common/speech_recognition_result.h"
#include "content/public/renderer/render_frame_observer.h"
#include "mojo/public/cpp/bindings/strong_binding_set.h"
#include "third_party/blink/public/web/web_speech_recognition_handle.h"
#include "third_party/blink/public/web/web_speech_recognizer.h"
#include "third_party/blink/public/web/web_speech_recognizer_client.h"

namespace content {
struct SpeechRecognitionError;

// SpeechRecognitionDispatcher is a delegate for methods used by WebKit for
// scripted JS speech APIs. It's the complement of
// SpeechRecognitionDispatcherHost (owned by RenderFrameHost), and communicates
// with it using two mojo interfaces (SpeechRecognizer and
// SpeechRecognitionSession).
class SpeechRecognitionDispatcher : public RenderFrameObserver,
                                    public blink::WebSpeechRecognizer {
 public:
  explicit SpeechRecognitionDispatcher(RenderFrame* render_frame);
  ~SpeechRecognitionDispatcher() override;

 private:
  // RenderFrameObserver implementation.
  void OnDestruct() override;
  void WasHidden() override;

  // blink::WebSpeechRecognizer implementation.
  void Start(const blink::WebSpeechRecognitionHandle&,
             const blink::WebSpeechRecognitionParams&,
             const blink::WebSpeechRecognizerClient&) override;
  void Stop(const blink::WebSpeechRecognitionHandle&,
            const blink::WebSpeechRecognizerClient&) override;
  void Abort(const blink::WebSpeechRecognitionHandle&,
             const blink::WebSpeechRecognizerClient&) override;

  // Methods to interact with |session_map_|
  void AddHandle(const blink::WebSpeechRecognitionHandle& handle,
                 mojom::SpeechRecognitionSessionPtr session);
  bool HandleExists(const blink::WebSpeechRecognitionHandle& handle);
  void RemoveHandle(const blink::WebSpeechRecognitionHandle& handle);
  mojom::SpeechRecognitionSession* GetSession(
      const blink::WebSpeechRecognitionHandle& handle);

  mojom::SpeechRecognizer& GetSpeechRecognitionHost();

  // InterfacePtr used to communicate with SpeechRecognitionDispatcherHost to
  // start a session for each WebSpeechRecognitionHandle.
  mojom::SpeechRecognizerPtr speech_recognition_host_;

  // The Blink client class that we use to send events back to the JS world.
  // This is passed to each SpeechRecognitionSessionClientImpl instance.
  blink::WebSpeechRecognizerClient recognizer_client_;

  // Owns a SpeechRecognitionSessionClientImpl for each session created. The
  // bindings are automatically cleaned up when the connection is closed by the
  // browser process. We use mojo::StrongBindingSet instead of using
  // mojo::MakeStrongBinding for each individual binding as
  // SpeechRecognitionSessionClientImpl holds a pointer to its parent
  // SpeechRecognitionDispatcher, and we need to clean up the
  // SpeechRecognitionSessionClientImpl objects when |this| is deleted.
  mojo::StrongBindingSet<mojom::SpeechRecognitionSessionClient> bindings_;

  // Owns a SpeechRecognitionSessionPtr per session created. Each
  // WebSpeechRecognitionHandle has its own speech recognition session, and is
  // used as a key for the map.
  using SessionMap = std::map<blink::WebSpeechRecognitionHandle,
                              mojom::SpeechRecognitionSessionPtr>;
  SessionMap session_map_;

  DISALLOW_COPY_AND_ASSIGN(SpeechRecognitionDispatcher);
  friend class SpeechRecognitionSessionClientImpl;
};

// SpeechRecognitionSessionClientImpl is owned by SpeechRecognitionDispatcher
// and is cleaned up either when the end point is closed by the browser process,
// or if SpeechRecognitionDispatcher is deleted.
class SpeechRecognitionSessionClientImpl
    : public mojom::SpeechRecognitionSessionClient {
 public:
  SpeechRecognitionSessionClientImpl(
      SpeechRecognitionDispatcher* dispatcher,
      const blink::WebSpeechRecognitionHandle& handle,
      const blink::WebSpeechRecognizerClient& client);
  ~SpeechRecognitionSessionClientImpl() override = default;

  // mojom::SpeechRecognitionSessionClient implementation
  void Started() override;
  void AudioStarted() override;
  void SoundStarted() override;
  void SoundEnded() override;
  void AudioEnded() override;
  void ErrorOccurred(const content::SpeechRecognitionError& error) override;
  void Ended() override;
  void ResultRetrieved(
      const std::vector<content::SpeechRecognitionResult>& results) override;

 private:
  // Not owned, |parent_dispatcher_| owns |this|.
  SpeechRecognitionDispatcher* parent_dispatcher_;
  // WebSpeechRecognitionHandle that this instance is associated with.
  blink::WebSpeechRecognitionHandle handle_;
  // The Blink client class that we use to send events back to the JS world.
  blink::WebSpeechRecognizerClient web_client_;
};

}  // namespace content

#endif  // CONTENT_RENDERER_SPEECH_RECOGNITION_DISPATCHER_H_
