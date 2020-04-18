// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_SHELL_TEST_RUNNER_MOCK_WEB_SPEECH_RECOGNIZER_H_
#define CONTENT_SHELL_TEST_RUNNER_MOCK_WEB_SPEECH_RECOGNIZER_H_

#include <vector>

#include "base/containers/circular_deque.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "third_party/blink/public/web/web_speech_recognizer.h"
#include "third_party/blink/public/web/web_speech_recognizer_client.h"

namespace blink {
class WebSpeechRecognitionHandle;
class WebSpeechRecognitionParams;
class WebSpeechRecognizerClient;
class WebString;
}  // namespace blink

namespace test_runner {

class WebTestDelegate;

class MockWebSpeechRecognizer : public blink::WebSpeechRecognizer {
 public:
  MockWebSpeechRecognizer();
  ~MockWebSpeechRecognizer() override;

  void SetDelegate(WebTestDelegate* delegate);

  // WebSpeechRecognizer implementation:
  void Start(const blink::WebSpeechRecognitionHandle& handle,
             const blink::WebSpeechRecognitionParams& params,
             const blink::WebSpeechRecognizerClient& client) override;
  void Stop(const blink::WebSpeechRecognitionHandle& handle,
            const blink::WebSpeechRecognizerClient& client) override;
  void Abort(const blink::WebSpeechRecognitionHandle& handle,
             const blink::WebSpeechRecognizerClient& client) override;

  // Methods accessed by layout tests:
  void AddMockResult(const blink::WebString& transcript, float confidence);
  void SetError(const blink::WebString& error, const blink::WebString& message);
  bool WasAborted() const { return was_aborted_; }

  // Methods accessed from Task objects:
  blink::WebSpeechRecognizerClient& Client() { return client_; }
  blink::WebSpeechRecognitionHandle& Handle() { return handle_; }

  void SetClientContext(const blink::WebSpeechRecognitionHandle&,
                        const blink::WebSpeechRecognizerClient&);

  class Task {
   public:
    explicit Task(MockWebSpeechRecognizer* recognizer)
        : recognizer_(recognizer) {}
    virtual ~Task() {}
    virtual void run() = 0;
    virtual bool isNewContextTask() const;

   protected:
    MockWebSpeechRecognizer* recognizer_;

   private:
    DISALLOW_COPY_AND_ASSIGN(Task);
  };

 private:
  void StartTaskQueue();
  void ClearTaskQueue();
  void PostRunTaskFromQueue();
  void RunTaskFromQueue();

  bool HasPendingNewContextTasks() const;

  blink::WebSpeechRecognitionHandle handle_;
  blink::WebSpeechRecognizerClient client_;
  std::vector<blink::WebString> mock_transcripts_;
  std::vector<float> mock_confidences_;
  bool was_aborted_;

  // Queue of tasks to be run.
  base::circular_deque<Task*> task_queue_;
  bool task_queue_running_;

  WebTestDelegate* delegate_;

  base::WeakPtrFactory<MockWebSpeechRecognizer> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(MockWebSpeechRecognizer);
};

}  // namespace test_runner

#endif  // CONTENT_SHELL_TEST_RUNNER_MOCK_WEB_SPEECH_RECOGNIZER_H_
