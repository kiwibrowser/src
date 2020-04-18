// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/macros.h"
#include "chrome/browser/speech/tts_platform.h"
#include "components/arc/arc_bridge_service.h"
#include "components/arc/arc_service_manager.h"
#include "components/arc/common/tts.mojom.h"
#include "content/public/browser/browser_thread.h"

// This class includes extension-based tts through LoadBuiltInTtsExtension and
// native tts through ARC.
class TtsPlatformImplChromeOs : public TtsPlatformImpl {
 public:
  // TtsPlatformImpl overrides:
  bool PlatformImplAvailable() override {
    return arc::ArcServiceManager::Get() && arc::ArcServiceManager::Get()
                                                ->arc_bridge_service()
                                                ->tts()
                                                ->IsConnected();
  }

  bool LoadBuiltInTtsExtension(
      content::BrowserContext* browser_context) override {
    TtsEngineDelegate* tts_engine_delegate =
        TtsController::GetInstance()->GetTtsEngineDelegate();
    if (tts_engine_delegate)
      return tts_engine_delegate->LoadBuiltInTtsExtension(browser_context);
    return false;
  }

  bool Speak(int utterance_id,
             const std::string& utterance,
             const std::string& lang,
             const VoiceData& voice,
             const UtteranceContinuousParameters& params) override {
    DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
    auto* const arc_service_manager = arc::ArcServiceManager::Get();
    if (!arc_service_manager)
      return false;
    arc::mojom::TtsInstance* tts = ARC_GET_INSTANCE_FOR_METHOD(
        arc_service_manager->arc_bridge_service()->tts(), Speak);
    if (!tts)
      return false;

    arc::mojom::TtsUtterancePtr arc_utterance = arc::mojom::TtsUtterance::New();
    arc_utterance->utteranceId = utterance_id;
    arc_utterance->text = utterance;
    arc_utterance->rate = params.rate;
    arc_utterance->pitch = params.pitch;
    tts->Speak(std::move(arc_utterance));
    return true;
  }

  bool StopSpeaking() override {
    DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
    auto* const arc_service_manager = arc::ArcServiceManager::Get();
    if (!arc_service_manager)
      return false;
    arc::mojom::TtsInstance* tts = ARC_GET_INSTANCE_FOR_METHOD(
        arc_service_manager->arc_bridge_service()->tts(), Stop);
    if (!tts)
      return false;

    tts->Stop();
    return true;
  }

  void GetVoices(std::vector<VoiceData>* out_voices) override {
    out_voices->push_back(VoiceData());
    VoiceData& voice = out_voices->back();
    voice.native = true;
    voice.name = "Android";
    voice.events.insert(TTS_EVENT_START);
    voice.events.insert(TTS_EVENT_END);
  }

  // Unimplemented.
  void Pause() override {}
  void Resume() override {}
  bool IsSpeaking() override { return false; }

  // Get the single instance of this class.
  static TtsPlatformImplChromeOs* GetInstance();

 private:
  TtsPlatformImplChromeOs() {}
  ~TtsPlatformImplChromeOs() override {}

  friend struct base::DefaultSingletonTraits<TtsPlatformImplChromeOs>;

  DISALLOW_COPY_AND_ASSIGN(TtsPlatformImplChromeOs);
};

// static
TtsPlatformImpl* TtsPlatformImpl::GetInstance() {
  return TtsPlatformImplChromeOs::GetInstance();
}

// static
TtsPlatformImplChromeOs*
TtsPlatformImplChromeOs::GetInstance() {
  return base::Singleton<TtsPlatformImplChromeOs>::get();
}
