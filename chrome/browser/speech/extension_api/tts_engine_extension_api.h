// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SPEECH_EXTENSION_API_TTS_ENGINE_EXTENSION_API_H_
#define CHROME_BROWSER_SPEECH_EXTENSION_API_TTS_ENGINE_EXTENSION_API_H_

#include <vector>

#include "base/memory/singleton.h"
#include "chrome/browser/speech/tts_controller.h"
#include "extensions/browser/extension_function.h"

class Utterance;

namespace content {
class BrowserContext;
}

namespace tts_engine_events {
extern const char kOnSpeak[];
extern const char kOnStop[];
extern const char kOnPause[];
extern const char kOnResume[];
}

// TtsEngineDelegate implementation used by TtsController.
class TtsExtensionEngine : public TtsEngineDelegate {
 public:
  static TtsExtensionEngine* GetInstance();

  // Overridden from TtsEngineDelegate:
  void GetVoices(content::BrowserContext* browser_context,
                 std::vector<VoiceData>* out_voices) override;
  void Speak(Utterance* utterance, const VoiceData& voice) override;
  void Stop(Utterance* utterance) override;
  void Pause(Utterance* utterance) override;
  void Resume(Utterance* utterance) override;
  bool LoadBuiltInTtsExtension(
      content::BrowserContext* browser_context) override;
};

// Function that allows tts engines to update its list of supported voices at
// runtime.
class ExtensionTtsEngineUpdateVoicesFunction
    : public UIThreadExtensionFunction {
 private:
  ~ExtensionTtsEngineUpdateVoicesFunction() override {}
  ResponseAction Run() override;
  DECLARE_EXTENSION_FUNCTION("ttsEngine.updateVoices", TTSENGINE_UPDATEVOICES)
};

// Hidden/internal extension function used to allow TTS engine extensions
// to send events back to the client that's calling tts.speak().
class ExtensionTtsEngineSendTtsEventFunction
    : public UIThreadExtensionFunction {
 private:
  ~ExtensionTtsEngineSendTtsEventFunction() override {}
  ResponseAction Run() override;
  DECLARE_EXTENSION_FUNCTION("ttsEngine.sendTtsEvent", TTSENGINE_SENDTTSEVENT)
};

#endif  // CHROME_BROWSER_SPEECH_EXTENSION_API_TTS_ENGINE_EXTENSION_API_H_
