// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_SETTINGS_TTS_HANDLER_H_
#define CHROME_BROWSER_UI_WEBUI_SETTINGS_TTS_HANDLER_H_

#include "chrome/browser/speech/tts_controller.h"
#include "chrome/browser/ui/webui/settings/settings_page_ui_handler.h"

namespace settings {

// Chrome "/manageAccessibility/tts/*" settings page UI handler.
class TtsHandler : public SettingsPageUIHandler, public VoicesChangedDelegate {
 public:
  TtsHandler() = default;
  ~TtsHandler() override = default;

  void HandleGetAllTtsVoiceData(const base::ListValue* args);
  void HandleGetTtsExtensions(const base::ListValue* args);
  void HandlePreviewTtsVoice(const base::ListValue* args);

  // SettingsPageUIHandler implementation.
  void RegisterMessages() override;
  void OnJavascriptAllowed() override;
  void OnJavascriptDisallowed() override;

  // VoicesChangedDelegate implementation.
  void OnVoicesChanged() override;

 private:
  int GetVoiceLangMatchScore(const VoiceData* voice,
                             const std::string& app_locale);

  DISALLOW_COPY_AND_ASSIGN(TtsHandler);
};

}  // namespace settings

#endif  // CHROME_BROWSER_UI_WEBUI_SETTINGS_TTS_HANDLER_H_
