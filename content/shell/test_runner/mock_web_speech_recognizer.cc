// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/shell/test_runner/mock_web_speech_recognizer.h"

#include <stddef.h>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/logging.h"
#include "base/macros.h"
#include "content/shell/test_runner/web_test_delegate.h"
#include "third_party/blink/public/web/web_speech_recognition_result.h"
#include "third_party/blink/public/web/web_speech_recognizer_client.h"

namespace test_runner {

namespace {

// Task class for calling a client function that does not take any parameters.
typedef void (blink::WebSpeechRecognizerClient::*ClientFunctionPointer)(
    const blink::WebSpeechRecognitionHandle&);
class ClientCallTask : public MockWebSpeechRecognizer::Task {
 public:
  ClientCallTask(MockWebSpeechRecognizer* mock, ClientFunctionPointer function)
      : MockWebSpeechRecognizer::Task(mock), function_(function) {}

  ~ClientCallTask() override {}

  void run() override {
    (recognizer_->Client().*function_)(recognizer_->Handle());
  }

 private:
  ClientFunctionPointer function_;

  DISALLOW_COPY_AND_ASSIGN(ClientCallTask);
};

// Task for delivering a result event.
class ResultTask : public MockWebSpeechRecognizer::Task {
 public:
  ResultTask(MockWebSpeechRecognizer* mock,
             const blink::WebString transcript,
             float confidence)
      : MockWebSpeechRecognizer::Task(mock),
        transcript_(transcript),
        confidence_(confidence) {}

  ~ResultTask() override {}

  void run() override {
    blink::WebVector<blink::WebString> transcripts(static_cast<size_t>(1));
    blink::WebVector<float> confidences(static_cast<size_t>(1));
    transcripts[0] = transcript_;
    confidences[0] = confidence_;
    blink::WebVector<blink::WebSpeechRecognitionResult> final_results(
        static_cast<size_t>(1));
    blink::WebVector<blink::WebSpeechRecognitionResult> interim_results;
    final_results[0].Assign(transcripts, confidences, true);

    recognizer_->Client().DidReceiveResults(recognizer_->Handle(),
                                            final_results, interim_results);
  }

 private:
  blink::WebString transcript_;
  float confidence_;

  DISALLOW_COPY_AND_ASSIGN(ResultTask);
};

// Task for delivering a nomatch event.
class NoMatchTask : public MockWebSpeechRecognizer::Task {
 public:
  explicit NoMatchTask(MockWebSpeechRecognizer* mock)
      : MockWebSpeechRecognizer::Task(mock) {}

  ~NoMatchTask() override {}

  void run() override {
    recognizer_->Client().DidReceiveNoMatch(
        recognizer_->Handle(), blink::WebSpeechRecognitionResult());
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(NoMatchTask);
};

// Task for delivering an error event.
class ErrorTask : public MockWebSpeechRecognizer::Task {
 public:
  ErrorTask(MockWebSpeechRecognizer* mock,
            blink::WebSpeechRecognizerClient::ErrorCode code,
            const blink::WebString& message)
      : MockWebSpeechRecognizer::Task(mock), code_(code), message_(message) {}

  ~ErrorTask() override {}

  void run() override {
    recognizer_->Client().DidReceiveError(recognizer_->Handle(), message_,
                                          code_);
  }

 private:
  blink::WebSpeechRecognizerClient::ErrorCode code_;
  blink::WebString message_;

  DISALLOW_COPY_AND_ASSIGN(ErrorTask);
};

// Task for tidying up after recognition task has ended.
class EndedTask : public MockWebSpeechRecognizer::Task {
 public:
  explicit EndedTask(MockWebSpeechRecognizer* mock)
      : MockWebSpeechRecognizer::Task(mock) {}

  ~EndedTask() override {}

  void run() override {
    blink::WebSpeechRecognitionHandle handle = recognizer_->Handle();
    blink::WebSpeechRecognizerClient client = recognizer_->Client();
    recognizer_->SetClientContext(blink::WebSpeechRecognitionHandle(),
                                  blink::WebSpeechRecognizerClient());
    client.DidEnd(handle);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(EndedTask);
};

// Task for switching processing to the next (handle, client) pairing.
class SwitchClientHandleTask : public MockWebSpeechRecognizer::Task {
 public:
  SwitchClientHandleTask(MockWebSpeechRecognizer* mock,
                         const blink::WebSpeechRecognitionHandle& handle,
                         const blink::WebSpeechRecognizerClient& client)
      : MockWebSpeechRecognizer::Task(mock), handle_(handle), client_(client) {}

  ~SwitchClientHandleTask() override {}

  bool isNewContextTask() const override { return true; }

  void run() override { recognizer_->SetClientContext(handle_, client_); }

 private:
  const blink::WebSpeechRecognitionHandle handle_;
  blink::WebSpeechRecognizerClient client_;

  DISALLOW_COPY_AND_ASSIGN(SwitchClientHandleTask);
};

}  // namespace

MockWebSpeechRecognizer::MockWebSpeechRecognizer()
    : was_aborted_(false),
      task_queue_running_(false),
      delegate_(nullptr),
      weak_factory_(this) {}

MockWebSpeechRecognizer::~MockWebSpeechRecognizer() {
  SetDelegate(nullptr);
}

bool MockWebSpeechRecognizer::Task::isNewContextTask() const {
  return false;
}

void MockWebSpeechRecognizer::SetDelegate(WebTestDelegate* delegate) {
  delegate_ = delegate;
  // No delegate to forward to, clear out pending tasks.
  if (!delegate_)
    ClearTaskQueue();
}

void MockWebSpeechRecognizer::SetClientContext(
    const blink::WebSpeechRecognitionHandle& handle,
    const blink::WebSpeechRecognizerClient& client) {
  handle_ = handle;
  client_ = client;
}

void MockWebSpeechRecognizer::Start(
    const blink::WebSpeechRecognitionHandle& handle,
    const blink::WebSpeechRecognitionParams& params,
    const blink::WebSpeechRecognizerClient& client) {
  was_aborted_ = false;
  if (client_.IsNull() && !HasPendingNewContextTasks()) {
    handle_ = handle;
    client_ = client;
  } else {
    task_queue_.push_back(new SwitchClientHandleTask(this, handle, client));
  }

  task_queue_.push_back(
      new ClientCallTask(this, &blink::WebSpeechRecognizerClient::DidStart));
  task_queue_.push_back(new ClientCallTask(
      this, &blink::WebSpeechRecognizerClient::DidStartAudio));
  task_queue_.push_back(new ClientCallTask(
      this, &blink::WebSpeechRecognizerClient::DidStartSound));

  if (!mock_transcripts_.empty()) {
    DCHECK_EQ(mock_transcripts_.size(), mock_confidences_.size());

    for (size_t i = 0; i < mock_transcripts_.size(); ++i)
      task_queue_.push_back(
          new ResultTask(this, mock_transcripts_[i], mock_confidences_[i]));

    mock_transcripts_.clear();
    mock_confidences_.clear();
  } else {
    task_queue_.push_back(new NoMatchTask(this));
  }

  task_queue_.push_back(
      new ClientCallTask(this, &blink::WebSpeechRecognizerClient::DidEndSound));
  task_queue_.push_back(
      new ClientCallTask(this, &blink::WebSpeechRecognizerClient::DidEndAudio));
  task_queue_.push_back(new EndedTask(this));

  StartTaskQueue();
}

void MockWebSpeechRecognizer::Stop(
    const blink::WebSpeechRecognitionHandle& handle,
    const blink::WebSpeechRecognizerClient& client) {
  SetClientContext(handle, client);

  // FIXME: Implement.
  NOTREACHED();
}

void MockWebSpeechRecognizer::Abort(
    const blink::WebSpeechRecognitionHandle& handle,
    const blink::WebSpeechRecognizerClient& client) {
  was_aborted_ = true;
  ClearTaskQueue();
  task_queue_.push_back(new SwitchClientHandleTask(this, handle, client));
  task_queue_.push_back(new EndedTask(this));

  StartTaskQueue();
}

void MockWebSpeechRecognizer::AddMockResult(const blink::WebString& transcript,
                                            float confidence) {
  mock_transcripts_.push_back(transcript);
  mock_confidences_.push_back(confidence);
}

void MockWebSpeechRecognizer::SetError(const blink::WebString& error,
                                       const blink::WebString& message) {
  blink::WebSpeechRecognizerClient::ErrorCode code;
  if (error == "OtherError")
    code = blink::WebSpeechRecognizerClient::kOtherError;
  else if (error == "NoSpeechError")
    code = blink::WebSpeechRecognizerClient::kNoSpeechError;
  else if (error == "AbortedError")
    code = blink::WebSpeechRecognizerClient::kAbortedError;
  else if (error == "AudioCaptureError")
    code = blink::WebSpeechRecognizerClient::kAudioCaptureError;
  else if (error == "NetworkError")
    code = blink::WebSpeechRecognizerClient::kNetworkError;
  else if (error == "NotAllowedError")
    code = blink::WebSpeechRecognizerClient::kNotAllowedError;
  else if (error == "ServiceNotAllowedError")
    code = blink::WebSpeechRecognizerClient::kServiceNotAllowedError;
  else if (error == "BadGrammarError")
    code = blink::WebSpeechRecognizerClient::kBadGrammarError;
  else if (error == "LanguageNotSupportedError")
    code = blink::WebSpeechRecognizerClient::kLanguageNotSupportedError;
  else
    return;

  ClearTaskQueue();
  task_queue_.push_back(new ErrorTask(this, code, message));
  task_queue_.push_back(new EndedTask(this));

  StartTaskQueue();
}

void MockWebSpeechRecognizer::StartTaskQueue() {
  if (task_queue_running_)
    return;
  PostRunTaskFromQueue();
}

void MockWebSpeechRecognizer::ClearTaskQueue() {
  while (!task_queue_.empty()) {
    Task* task = task_queue_.front();
    if (task->isNewContextTask())
      break;
    delete task_queue_.front();
    task_queue_.pop_front();
  }
  if (task_queue_.empty())
    task_queue_running_ = false;
}

void MockWebSpeechRecognizer::PostRunTaskFromQueue() {
  task_queue_running_ = true;
  delegate_->PostTask(base::BindOnce(&MockWebSpeechRecognizer::RunTaskFromQueue,
                                     weak_factory_.GetWeakPtr()));
}

void MockWebSpeechRecognizer::RunTaskFromQueue() {
  if (task_queue_.empty()) {
    task_queue_running_ = false;
    return;
  }

  MockWebSpeechRecognizer::Task* task = task_queue_.front();
  task_queue_.pop_front();
  task->run();
  delete task;

  if (task_queue_.empty()) {
    task_queue_running_ = false;
    return;
  }

  PostRunTaskFromQueue();
}

bool MockWebSpeechRecognizer::HasPendingNewContextTasks() const {
  for (auto* task : task_queue_) {
    if (task->isNewContextTask())
      return true;
  }
  return false;
}

}  // namespace test_runner
