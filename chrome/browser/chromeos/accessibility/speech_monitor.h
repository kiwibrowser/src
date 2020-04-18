// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_ACCESSIBILITY_SPEECH_MONITOR_H_
#define CHROME_BROWSER_CHROMEOS_ACCESSIBILITY_SPEECH_MONITOR_H_

#include "base/containers/circular_deque.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "chrome/browser/speech/tts_platform.h"
#include "content/public/test/test_utils.h"

namespace chromeos {

// For testing purpose installs itself as the platform speech synthesis engine,
// allowing it to intercept all speech calls, and then provides a method to
// block until the next utterance is spoken.
class SpeechMonitor : public TtsPlatformImpl {
 public:
  SpeechMonitor();
  ~SpeechMonitor() override;

  // Blocks until the next utterance is spoken, and returns its text.
  std::string GetNextUtterance();

  // Wait for next utterance and return true if next utterance is ChromeVox
  // enabled message.
  bool SkipChromeVoxEnabledMessage();
  bool SkipChromeVoxMessage(const std::string& message);

  // Returns true if StopSpeaking() was called on TtsController.
  bool DidStop();

  // Blocks until StopSpeaking() is called on TtsController.
  void BlockUntilStop();

 private:
  // TtsPlatformImpl implementation.
  bool PlatformImplAvailable() override;
  bool Speak(int utterance_id,
             const std::string& utterance,
             const std::string& lang,
             const VoiceData& voice,
             const UtteranceContinuousParameters& params) override;
  bool StopSpeaking() override;
  bool IsSpeaking() override;
  void GetVoices(std::vector<VoiceData>* out_voices) override;
  void Pause() override {}
  void Resume() override {}
  std::string error() override;
  void clear_error() override {}
  void set_error(const std::string& error) override {}
  void WillSpeakUtteranceWithVoice(const Utterance* utterance,
                                   const VoiceData& voice_data) override;

  scoped_refptr<content::MessageLoopRunner> loop_runner_;
  base::circular_deque<std::string> utterance_queue_;
  bool did_stop_ = false;

  DISALLOW_COPY_AND_ASSIGN(SpeechMonitor);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_ACCESSIBILITY_SPEECH_MONITOR_H_
