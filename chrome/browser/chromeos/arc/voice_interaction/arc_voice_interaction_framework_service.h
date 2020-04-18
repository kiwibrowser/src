// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_ARC_VOICE_INTERACTION_ARC_VOICE_INTERACTION_FRAMEWORK_SERVICE_H_
#define CHROME_BROWSER_CHROMEOS_ARC_VOICE_INTERACTION_ARC_VOICE_INTERACTION_FRAMEWORK_SERVICE_H_

#include <memory>

#include "ash/public/interfaces/voice_interaction_controller.mojom.h"
#include "base/callback_forward.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "chrome/browser/chromeos/arc/arc_session_manager.h"
#include "chromeos/audio/cras_audio_handler.h"
#include "components/arc/common/voice_interaction_framework.mojom.h"
#include "components/arc/connection_observer.h"
#include "components/keyed_service/core/keyed_service.h"

class KeyedServiceBaseFactory;

namespace content {
class BrowserContext;
}  // namespace content

namespace gfx {
class Rect;
}  // namespace gfx

namespace ui {
class LayerTreeOwner;
}  // namespace ui

namespace arc {

class ArcBridgeService;
class HighlighterControllerClient;

// This provides voice interaction context (currently screenshots)
// to ARC to be used by VoiceInteractionSession. This class lives on the UI
// thread.
class ArcVoiceInteractionFrameworkService
    : public chromeos::CrasAudioHandler::AudioObserver,
      public KeyedService,
      public mojom::VoiceInteractionFrameworkHost,
      public ConnectionObserver<mojom::VoiceInteractionFrameworkInstance>,
      public ArcSessionManager::Observer {
 public:
  // Returns singleton instance for the given BrowserContext,
  // or nullptr if the browser |context| is not allowed to use ARC.
  static ArcVoiceInteractionFrameworkService* GetForBrowserContext(
      content::BrowserContext* context);

  // Returns factory for ArcVoiceInteractionFrameworkService.
  static KeyedServiceBaseFactory* GetFactory();

  ArcVoiceInteractionFrameworkService(content::BrowserContext* context,
                                      ArcBridgeService* bridge_service);
  ~ArcVoiceInteractionFrameworkService() override;

  // ConnectionObserver<mojom::VoiceInteractionFrameworkInstance> overrides.
  void OnConnectionReady() override;
  void OnConnectionClosed() override;

  // mojom::VoiceInteractionFrameworkHost overrides.
  void CaptureFullscreen(CaptureFullscreenCallback callback) override;
  void SetVoiceInteractionState(
      arc::mojom::VoiceInteractionState state) override;

  void ShowMetalayer();
  void HideMetalayer();

  // ArcSessionManager::Observer overrides.
  void OnArcPlayStoreEnabledChanged(bool enabled) override;

  // CrasAudioHandler::AudioObserver overrides.
  void OnHotwordTriggered(uint64_t tv_sec, uint64_t tv_nsec) override;

  // Starts a voice interaction session after user-initiated interaction.
  // Records a timestamp and sets number of allowed requests to 2 since by
  // design, there will be one request for screenshot and the other for
  // voice interaction context.
  // |region| refers to the selected region on the screen to be passed to
  // VoiceInteractionFrameworkInstance::StartVoiceInteractionSessionForRegion().
  // If |region| is empty,
  // VoiceInteractionFrameworkInstance::StartVoiceInteraction() is called.
  void StartSessionFromUserInteraction(const gfx::Rect& region);

  // Similar to StartSessionFromUserInteraction but stops voice interaction
  // seesion if it is already running.
  void ToggleSessionFromUserInteraction();

  using VoiceInteractionSettingCompleteCallback =
      base::OnceCallback<void(bool)>;
  // Turn on / off voice interaction in ARC. |callback| will be called with
  // |true| if setting is applied to Android side.
  void SetVoiceInteractionEnabled(
      bool enable,
      VoiceInteractionSettingCompleteCallback callback);

  // Turn on / off voice interaction context (screenshot and structural data)
  // in ARC.
  void SetVoiceInteractionContextEnabled(bool enable);

  // Checks whether the caller is called within the time limit since last user
  // initiated interaction. Logs UMA metric when it's not.
  bool ValidateTimeSinceUserInteraction();

  // Start the voice interaction setup wizard in container.
  void StartVoiceInteractionSetupWizard();

  // Show the voice interaction settings in container.
  void ShowVoiceInteractionSettings();

  // Set voice interaction setup completed flag and notify the change.
  void SetVoiceInteractionSetupCompleted();

  // Starts voice interaction OOBE flow.
  void StartVoiceInteractionOobe();

  HighlighterControllerClient* GetHighlighterClientForTesting() const {
    return highlighter_client_.get();
  }

  ash::mojom::VoiceInteractionState GetStateForTesting() const {
    return state_;
  }

  std::unique_ptr<ui::LayerTreeOwner> CreateLayerTreeForSnapshotForTesting(
      aura::Window* root_window) const;

  // For supporting ArcServiceManager::GetService<T>().
  static const char kArcServiceName[];

 private:
  void NotifyMetalayerStatusChanged(bool visible);

  bool InitiateUserInteraction(bool is_toggle);

  void SetVoiceInteractionSetupCompletedInternal(bool completed);

  bool IsHomescreenActive();

  void StartVoiceInteractionSetupWizardActivity();

  content::BrowserContext* context_;
  ArcBridgeService* const arc_bridge_service_;  // Owned by ArcServiceManager

  // Whether there is a pending request to start/toggle voice interaction.
  bool is_request_pending_ = false;

  // Whether the pending request is toggle the voice interaction.
  bool is_pending_request_toggle_ = false;

  // Whether we should launch runtime setup flow for voice interaction.
  bool should_start_runtime_flow_ = false;

  // The current state voice interaction service is. There is usually a long
  // delay after boot before the service is ready. We wait for the container
  // to tell us if it is ready to quickly serve voice interaction requests.
  // We also give user proper feedback based on the state.
  ash::mojom::VoiceInteractionState state_ =
      ash::mojom::VoiceInteractionState::NOT_READY;

  // The time when a user initated an interaction.
  base::TimeTicks user_interaction_start_time_;

  // The number of allowed requests from container. Maximum is 2 (1 for
  // screenshot and 1 for view hierarchy). This amount decreases after each
  // context request or resets when allowed time frame is elapsed.  When this
  // quota is 0, but we still get requests from the container side, we assume
  // something malicious is going on.
  int32_t context_request_remaining_count_ = 0;

  std::unique_ptr<HighlighterControllerClient> highlighter_client_;

  base::WeakPtrFactory<ArcVoiceInteractionFrameworkService> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(ArcVoiceInteractionFrameworkService);
};

}  // namespace arc
#endif  // CHROME_BROWSER_CHROMEOS_ARC_VOICE_INTERACTION_ARC_VOICE_INTERACTION_FRAMEWORK_SERVICE_H_
