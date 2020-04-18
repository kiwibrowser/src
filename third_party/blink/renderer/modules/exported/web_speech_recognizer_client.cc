// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/public/web/web_speech_recognizer_client.h"

#include "third_party/blink/public/web/web_speech_recognition_handle.h"
#include "third_party/blink/renderer/modules/speech/speech_recognition_client_proxy.h"

namespace blink {

WebSpeechRecognizerClient::WebSpeechRecognizerClient(
    SpeechRecognitionClientProxy* proxy)
    : private_(proxy) {}

WebSpeechRecognizerClient::~WebSpeechRecognizerClient() {
  Reset();
}

bool WebSpeechRecognizerClient::operator==(
    const WebSpeechRecognizerClient& other) const {
  return private_.Get() == other.private_.Get();
}

bool WebSpeechRecognizerClient::operator!=(
    const WebSpeechRecognizerClient& other) const {
  return !operator==(other);
}

void WebSpeechRecognizerClient::Reset() {
  private_.Reset();
}

void WebSpeechRecognizerClient::Assign(const WebSpeechRecognizerClient& other) {
  private_ = other.private_;
}

void WebSpeechRecognizerClient::DidStartAudio(
    const WebSpeechRecognitionHandle& handle) {
  DCHECK(!private_.IsNull());
  private_->DidStartAudio(handle);
}

void WebSpeechRecognizerClient::DidStartSound(
    const WebSpeechRecognitionHandle& handle) {
  DCHECK(!private_.IsNull());
  private_->DidStartSound(handle);
}

void WebSpeechRecognizerClient::DidEndSound(
    const WebSpeechRecognitionHandle& handle) {
  DCHECK(!private_.IsNull());
  private_->DidEndSound(handle);
}

void WebSpeechRecognizerClient::DidEndAudio(
    const WebSpeechRecognitionHandle& handle) {
  DCHECK(!private_.IsNull());
  private_->DidEndAudio(handle);
}

void WebSpeechRecognizerClient::DidReceiveResults(
    const WebSpeechRecognitionHandle& handle,
    const WebVector<WebSpeechRecognitionResult>& new_final_results,
    const WebVector<WebSpeechRecognitionResult>& current_interim_results) {
  DCHECK(!private_.IsNull());
  private_->DidReceiveResults(handle, new_final_results,
                              current_interim_results);
}

void WebSpeechRecognizerClient::DidReceiveNoMatch(
    const WebSpeechRecognitionHandle& handle,
    const WebSpeechRecognitionResult& result) {
  DCHECK(!private_.IsNull());
  private_->DidReceiveNoMatch(handle, result);
}

void WebSpeechRecognizerClient::DidReceiveError(
    const WebSpeechRecognitionHandle& handle,
    const WebString& message,
    ErrorCode error_code) {
  DCHECK(!private_.IsNull());
  private_->DidReceiveError(handle, message, error_code);
}

void WebSpeechRecognizerClient::DidStart(
    const WebSpeechRecognitionHandle& handle) {
  DCHECK(!private_.IsNull());
  private_->DidStart(handle);
}

void WebSpeechRecognizerClient::DidEnd(
    const WebSpeechRecognitionHandle& handle) {
  DCHECK(!private_.IsNull());
  private_->DidEnd(handle);
}

}  // namespace blink
