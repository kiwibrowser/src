// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SPEECH_SPEECH_RECOGNIZER_H_
#define CHROME_BROWSER_SPEECH_SPEECH_RECOGNIZER_H_

#include <string>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"

namespace content {
struct SpeechRecognitionSessionPreamble;
}

namespace net {
class URLRequestContextGetter;
}

class SpeechRecognizerDelegate;

// SpeechRecognizer is a wrapper around the speech recognition engine that
// simplifies its use from the UI thread. This class handles all setup/shutdown,
// collection of results, error cases, and threading.
class SpeechRecognizer {
 public:
  SpeechRecognizer(const base::WeakPtr<SpeechRecognizerDelegate>& delegate,
                   net::URLRequestContextGetter* url_request_context_getter,
                   const std::string& locale);
  ~SpeechRecognizer();

  // Start/stop the speech recognizer. |preamble| contains the preamble audio to
  // log if auth parameters are available.
  // Must be called on the UI thread.
  void Start(
      const scoped_refptr<content::SpeechRecognitionSessionPreamble>& preamble);
  void Stop();

 private:
  class EventListener;

  base::WeakPtr<SpeechRecognizerDelegate> delegate_;
  scoped_refptr<EventListener> speech_event_listener_;

  DISALLOW_COPY_AND_ASSIGN(SpeechRecognizer);
};

#endif  // CHROME_BROWSER_SPEECH_SPEECH_RECOGNIZER_H_
