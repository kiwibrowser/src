// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_ARC_VOICE_INTERACTION_ARC_VOICE_INTERACTION_ARC_HOME_SERVICE_H_
#define CHROME_BROWSER_CHROMEOS_ARC_VOICE_INTERACTION_ARC_VOICE_INTERACTION_ARC_HOME_SERVICE_H_

#include <string>

#include "base/macros.h"
#include "base/timer/timer.h"
#include "chrome/browser/ui/app_list/arc/arc_app_list_prefs.h"
#include "components/arc/common/voice_interaction_arc_home.mojom.h"
#include "components/arc/connection_observer.h"
#include "components/keyed_service/core/keyed_service.h"
#include "ui/accessibility/ax_tree_update.h"

namespace content {
class BrowserContext;
}  // namespace content

namespace ui {
struct AssistantTree;
struct AssistantNode;
}  // namespace ui

namespace arc {

class ArcBridgeService;

// ArcVoiceInteractionArcHomeService provides view hierarchy to to ARC to be
// used by VoiceInteractionSession. This class lives on the UI thread.
class ArcVoiceInteractionArcHomeService
    : public KeyedService,
      public mojom::VoiceInteractionArcHomeHost,
      public ConnectionObserver<mojom::VoiceInteractionArcHomeInstance>,
      public ArcAppListPrefs::Observer,
      public ArcSessionManager::Observer {
 public:
  // Returns singleton instance for the given BrowserContext,
  // or nullptr if the browser |context| is not allowed to use ARC.
  static ArcVoiceInteractionArcHomeService* GetForBrowserContext(
      content::BrowserContext* context);

  static const char kAssistantPackageName[];

  ArcVoiceInteractionArcHomeService(content::BrowserContext* context,
                                    ArcBridgeService* bridge_service);
  ~ArcVoiceInteractionArcHomeService() override;

  // Notifies that assistant flow has been started and we have to lock PAI.
  void OnAssistantStarted();
  // Notifies that assistant flow expects Android assistant app started.
  void OnAssistantAppRequested();
  // Notifies that assistant flow has been canceled.
  void OnAssistantCanceled();

  // KeyedService overrides:
  void Shutdown() override;

  // ConnectionObserver<mojom::VoiceInteractionArcHomeInstance> overrides;
  void OnConnectionClosed() override;

  // Gets view hierarchy from current focused app and send it to ARC.
  void GetVoiceInteractionStructure(
      GetVoiceInteractionStructureCallback callback) override;
  void OnVoiceInteractionOobeSetupComplete() override;

  static mojom::VoiceInteractionStructurePtr
  CreateVoiceInteractionStructureForTesting(const ui::AssistantTree& tree,
                                            const ui::AssistantNode& node);

  void set_assistant_started_timeout_for_testing(
      const base::TimeDelta& timeout) {
    assistant_started_timeout_ = timeout;
  }

  void set_wizard_completed_timeout_for_testing(
      const base::TimeDelta& timeout) {
    wizard_completed_timeout_ = timeout;
  }

 private:
  // ArcAppListPrefs::Observer:
  void OnTaskCreated(int32_t task_id,
                     const std::string& package_name,
                     const std::string& activity,
                     const std::string& intent) override;
  void OnTaskDestroyed(int32_t task_id) override;

  // ArcSessionManager::Observer:
  void OnArcPlayStoreEnabledChanged(bool enabled) override;

  // Locks/Unlocks Play Auto Install.
  void LockPai();
  void UnlockPai();

  // Resets all optional timeouts and observers.
  void ResetTimeouts();
  // Callback to handle timeout of waiting assistant Android app to start.
  void OnAssistantStartTimeout();
  // Callback to handle timeout of waiting wizard to complete.
  void OnWizardCompleteTimeout();

  content::BrowserContext* const context_;
  ArcBridgeService* const arc_bridge_service_;  // Owned by ArcServiceManager.

  int32_t assistant_task_id_ = -1;
  // Waits until assistant is actually started.
  base::OneShotTimer assistant_started_timer_;
  base::TimeDelta assistant_started_timeout_;

  // Waits for wizard completed notification.
  base::OneShotTimer wizard_completed_timer_;
  base::TimeDelta wizard_completed_timeout_;

  // Whether there is a pending request to lock PAI before it's available.
  bool pending_pai_lock_ = false;

  DISALLOW_COPY_AND_ASSIGN(ArcVoiceInteractionArcHomeService);
};

}  // namespace arc
#endif  // CHROME_BROWSER_CHROMEOS_ARC_VOICE_INTERACTION_ARC_VOICE_INTERACTION_ARC_HOME_SERVICE_H_
