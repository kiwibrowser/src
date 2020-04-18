// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/accessibility/speech_monitor.h"

namespace chromeos {

namespace {
const char kChromeVoxEnabledMessage[] = "chrome vox spoken feedback is ready";
const char kChromeVoxAlertMessage[] = "Alert";
const char kChromeVoxUpdate1[] = "chrome vox Updated Press chrome vox o,";
const char kChromeVoxUpdate2[] = "n to learn more about chrome vox Next.";
}  // namespace

SpeechMonitor::SpeechMonitor() {
  TtsController::GetInstance()->SetPlatformImpl(this);
}

SpeechMonitor::~SpeechMonitor() {
  TtsController::GetInstance()->SetPlatformImpl(TtsPlatformImpl::GetInstance());
}

std::string SpeechMonitor::GetNextUtterance() {
  if (utterance_queue_.empty()) {
    loop_runner_ = new content::MessageLoopRunner();
    loop_runner_->Run();
    loop_runner_ = NULL;
  }
  std::string result = utterance_queue_.front();
  utterance_queue_.pop_front();
  return result;
}

bool SpeechMonitor::SkipChromeVoxEnabledMessage() {
  return SkipChromeVoxMessage(kChromeVoxEnabledMessage);
}

bool SpeechMonitor::DidStop() {
  return did_stop_;
}

void SpeechMonitor::BlockUntilStop() {
  if (!did_stop_) {
    loop_runner_ = new content::MessageLoopRunner();
    loop_runner_->Run();
    loop_runner_ = NULL;
  }
}

bool SpeechMonitor::SkipChromeVoxMessage(const std::string& message) {
  while (true) {
    if (utterance_queue_.empty()) {
      loop_runner_ = new content::MessageLoopRunner();
      loop_runner_->Run();
      loop_runner_ = NULL;
    }
    std::string result = utterance_queue_.front();
    utterance_queue_.pop_front();
    if (result == message)
      return true;
  }
  return false;
}

bool SpeechMonitor::PlatformImplAvailable() {
  return true;
}

bool SpeechMonitor::Speak(int utterance_id,
                          const std::string& utterance,
                          const std::string& lang,
                          const VoiceData& voice,
                          const UtteranceContinuousParameters& params) {
  TtsController::GetInstance()->OnTtsEvent(utterance_id, TTS_EVENT_END,
                                           static_cast<int>(utterance.size()),
                                           std::string());
  return true;
}

bool SpeechMonitor::StopSpeaking() {
  did_stop_ = true;
  return true;
}

bool SpeechMonitor::IsSpeaking() {
  return false;
}

void SpeechMonitor::GetVoices(std::vector<VoiceData>* out_voices) {
  out_voices->push_back(VoiceData());
  VoiceData& voice = out_voices->back();
  voice.native = true;
  voice.name = "SpeechMonitor";
  voice.events.insert(TTS_EVENT_END);
}

std::string SpeechMonitor::error() {
  return "";
}

void SpeechMonitor::WillSpeakUtteranceWithVoice(const Utterance* utterance,
                                                const VoiceData& voice_data) {
  // Blacklist some phrases.
  // Filter out empty utterances which can be used to trigger a start event from
  // tts as an earcon sync.
  if (utterance->text() == "" || utterance->text() == kChromeVoxAlertMessage ||
      utterance->text() == kChromeVoxUpdate1 ||
      utterance->text() == kChromeVoxUpdate2)
    return;

  VLOG(0) << "Speaking " << utterance->text();
  utterance_queue_.push_back(utterance->text());
  if (loop_runner_.get())
    loop_runner_->Quit();
}

}  // namespace chromeos
