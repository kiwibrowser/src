// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromecast/browser/tts/tts_controller.h"
#include "chromecast/browser/tts/tts_platform.h"

namespace chromecast {

// An implementaton of TtsPlaform for Cast that merely logs events.
class TtsPlatformImplStub : public TtsPlatformImpl {
 public:
  bool PlatformImplAvailable() override { return true; }

  bool Speak(int utterance_id,
             const std::string& utterance,
             const std::string& lang,
             const VoiceData& voice,
             const UtteranceContinuousParameters& params) override;

  bool StopSpeaking() override;

  void Pause() override;

  void Resume() override;

  bool IsSpeaking() override;

  void GetVoices(std::vector<VoiceData>* out_voices) override;

  // Get the single instance of this class.
  static TtsPlatformImplStub* GetInstance();

 private:
  TtsPlatformImplStub() = default;
  ~TtsPlatformImplStub() override = default;

  friend struct base::DefaultSingletonTraits<TtsPlatformImplStub>;

  DISALLOW_COPY_AND_ASSIGN(TtsPlatformImplStub);
};

bool TtsPlatformImplStub::Speak(int utterance_id,
                                const std::string& utterance,
                                const std::string& lang,
                                const VoiceData& voice,
                                const UtteranceContinuousParameters& params) {
  LOG(INFO) << "Speak: " << utterance;
  return true;
}

bool TtsPlatformImplStub::StopSpeaking() {
  LOG(INFO) << "StopSpeaking";
  return true;
}

void TtsPlatformImplStub::Pause() {
  LOG(INFO) << "Pause";
}

void TtsPlatformImplStub::Resume() {
  LOG(INFO) << "Resume";
}

bool TtsPlatformImplStub::IsSpeaking() {
  LOG(INFO) << "IsSpeaking";
  return false;
}

void TtsPlatformImplStub::GetVoices(std::vector<VoiceData>* out_voices) {
  LOG(INFO) << "GetVoices";
}

// static
TtsPlatformImplStub* TtsPlatformImplStub::GetInstance() {
  return base::Singleton<TtsPlatformImplStub, base::LeakySingletonTraits<
                                                  TtsPlatformImplStub>>::get();
}

}  // namespace chromecast

// TODO(rdaum): This is temporarily disabled to avoid link time duplicate
// symbol failures when the internal platform version is added. This code
// will be restored (behind a flag) after the internal CL which adds a
// GetInstance method symbol failures when the internal platform version is
// added.
// static
// TtsPlatformImpl* TtsPlatformImpl::GetInstance() {
//   return chromecast::TtsPlatformImplStub::GetInstance();
// }
