// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_SERVICES_ASSISTANT_SERVICE_H_
#define CHROMEOS_SERVICES_ASSISTANT_SERVICE_H_

#include <memory>
#include <string>

#include "ash/public/interfaces/session_controller.mojom.h"
#include "base/callback.h"
#include "base/memory/weak_ptr.h"
#include "base/optional.h"
#include "base/time/time.h"
#include "chromeos/services/assistant/public/mojom/assistant.mojom.h"
#include "chromeos/services/assistant/public/mojom/settings.mojom.h"
#include "components/account_id/account_id.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "mojo/public/cpp/bindings/interface_ptr_set.h"
#include "services/identity/public/mojom/identity_manager.mojom.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "services/service_manager/public/cpp/service.h"

class GoogleServiceAuthError;

namespace base {
class OneShotTimer;
}

namespace chromeos {
namespace assistant {

class AssistantManagerService;
class AssistantSettingsManager;

class Service : public service_manager::Service,
                public ash::mojom::SessionActivationObserver,
                public mojom::AssistantPlatform {
 public:
  Service();
  ~Service() override;

  void SetIdentityManagerForTesting(
      identity::mojom::IdentityManagerPtr identity_manager);

  void SetAssistantManagerForTesting(
      std::unique_ptr<AssistantManagerService> assistant_manager_service);

  void SetTimerForTesting(std::unique_ptr<base::OneShotTimer> timer);

 private:
  friend class ServiceTest;
  // service_manager::Service overrides
  void OnStart() override;
  void OnBindInterface(const service_manager::BindSourceInfo& source_info,
                       const std::string& interface_name,
                       mojo::ScopedMessagePipeHandle interface_pipe) override;
  void BindAssistantConnection(mojom::AssistantRequest request);
  void BindAssistantPlatformConnection(mojom::AssistantPlatformRequest request);

  // mojom::AssistantPlatform overrides:
  void Init(mojom::ClientPtr client,
            mojom::ContextPtr assistant_context,
            mojom::AudioInputPtr audio_input) override;

  // ash::mojom::SessionActivationObserver overrides:
  void OnSessionActivated(bool activated) override;
  void OnLockStateChanged(bool locked) override;

  void BindAssistantSettingsManager(
      mojom::AssistantSettingsManagerRequest request);

  void RequestAccessToken();

  identity::mojom::IdentityManager* GetIdentityManager();

  void GetPrimaryAccountInfoCallback(
      const base::Optional<AccountInfo>& account_info,
      const identity::AccountState& account_state);

  void GetAccessTokenCallback(const base::Optional<std::string>& token,
                              base::Time expiration_time,
                              const GoogleServiceAuthError& error);

  void AddAshSessionObserver();

  void UpdateListeningState();

  void FinalizeAssistantManagerService();

  service_manager::BinderRegistry registry_;

  mojo::BindingSet<mojom::Assistant> bindings_;
  mojo::Binding<mojom::AssistantPlatform> platform_binding_;
  mojo::Binding<ash::mojom::SessionActivationObserver>
      session_observer_binding_;
  mojom::ClientPtr client_;

  identity::mojom::IdentityManagerPtr identity_manager_;

  AccountId account_id_;
  std::unique_ptr<AssistantManagerService> assistant_manager_service_;
  AssistantSettingsManager* assistant_settings_manager_;
  std::unique_ptr<base::OneShotTimer> token_refresh_timer_;
  scoped_refptr<base::SingleThreadTaskRunner> main_thread_task_runner_;

  // Whether the current user session is active.
  bool session_active_ = false;
  // Whether the lock screen is on.
  bool locked_ = false;

  base::WeakPtrFactory<Service> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(Service);
};

}  // namespace assistant
}  // namespace chromeos

#endif  // CHROMEOS_SERVICES_ASSISTANT_SERVICE_H_
