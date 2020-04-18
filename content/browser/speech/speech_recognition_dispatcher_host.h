// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_SPEECH_SPEECH_RECOGNITION_DISPATCHER_HOST_H_
#define CONTENT_BROWSER_SPEECH_SPEECH_RECOGNITION_DISPATCHER_HOST_H_

#include <memory>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "content/common/content_export.h"
#include "content/common/speech_recognizer.mojom.h"
#include "content/public/browser/browser_message_filter.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/speech_recognition_event_listener.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "net/url_request/url_request_context_getter.h"

namespace content {

class SpeechRecognitionSession;
class SpeechRecognitionManager;

// SpeechRecognitionDispatcherHost is an implementation of the SpeechRecognizer
// interface that allows a RenderFrame to start a speech recognition session
// in the browser process, by communicating with SpeechRecognitionManager.
class CONTENT_EXPORT SpeechRecognitionDispatcherHost
    : public mojom::SpeechRecognizer {
 public:
  SpeechRecognitionDispatcherHost(
      int render_process_id,
      int render_frame_id,
      scoped_refptr<net::URLRequestContextGetter> context_getter);
  ~SpeechRecognitionDispatcherHost() override;
  static void Create(int render_process_id,
                     int render_frame_id,
                     scoped_refptr<net::URLRequestContextGetter> context_getter,
                     mojom::SpeechRecognizerRequest request);
  base::WeakPtr<SpeechRecognitionDispatcherHost> AsWeakPtr();

  // mojom::SpeechRecognizer implementation
  void Start(mojom::StartSpeechRecognitionRequestParamsPtr params) override;

 private:
  static void StartRequestOnUI(
      base::WeakPtr<SpeechRecognitionDispatcherHost>
          speech_recognition_dispatcher_host,
      int render_process_id,
      int render_frame_id,
      mojom::StartSpeechRecognitionRequestParamsPtr params);
  void StartSessionOnIO(mojom::StartSpeechRecognitionRequestParamsPtr params,
                        int embedder_render_process_id,
                        int embedder_render_frame_id,
                        bool filter_profanities);

  const int render_process_id_;
  const int render_frame_id_;
  scoped_refptr<net::URLRequestContextGetter> context_getter_;

  // Used for posting asynchronous tasks (on the IO thread) without worrying
  // about this class being destroyed in the meanwhile (due to browser shutdown)
  // since tasks pending on a destroyed WeakPtr are automatically discarded.
  base::WeakPtrFactory<SpeechRecognitionDispatcherHost> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(SpeechRecognitionDispatcherHost);
};

// SpeechRecognitionSession implements the mojom::SpeechRecognitionSession
// interface for a particular session. It also acts as a proxy for events sent
// from SpeechRecognitionManager, and forwards the events to the renderer using
// a SpeechRecognitionSessionClientPtr (that is passed from the render process).
class SpeechRecognitionSession : public mojom::SpeechRecognitionSession,
                                 public SpeechRecognitionEventListener {
 public:
  explicit SpeechRecognitionSession(
      mojom::SpeechRecognitionSessionClientPtrInfo client_ptr_info);
  ~SpeechRecognitionSession() override;
  base::WeakPtr<SpeechRecognitionSession> AsWeakPtr();

  void SetSessionId(int session_id) { session_id_ = session_id; }

  // mojom::SpeechRecognitionSession implementation.
  void Abort() override;
  void StopCapture() override;

  // SpeechRecognitionEventListener methods.
  void OnRecognitionStart(int session_id) override;
  void OnAudioStart(int session_id) override;
  void OnEnvironmentEstimationComplete(int session_id) override;
  void OnSoundStart(int session_id) override;
  void OnSoundEnd(int session_id) override;
  void OnAudioEnd(int session_id) override;
  void OnRecognitionEnd(int session_id) override;
  void OnRecognitionResults(int session_id,
                            const SpeechRecognitionResults& results) override;
  void OnRecognitionError(int session_id,
                          const SpeechRecognitionError& error) override;
  void OnAudioLevelsChange(int session_id,
                           float volume,
                           float noise_volume) override;

 private:
  int session_id_;
  mojom::SpeechRecognitionSessionClientPtr client_;

  base::WeakPtrFactory<SpeechRecognitionSession> weak_factory_;
};

}  // namespace content

#endif  // CONTENT_BROWSER_SPEECH_SPEECH_RECOGNITION_DISPATCHER_HOST_H_
