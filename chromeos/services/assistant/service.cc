// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/services/assistant/service.h"

#include <utility>

#include "ash/public/interfaces/assistant_controller.mojom.h"
#include "ash/public/interfaces/constants.mojom.h"
#include "base/bind.h"
#include "base/logging.h"
#include "base/single_thread_task_runner.h"
#include "base/timer/timer.h"
#include "build/buildflag.h"
#include "chromeos/assistant/buildflags.h"
#include "chromeos/services/assistant/assistant_manager_service.h"
#include "chromeos/services/assistant/assistant_settings_manager.h"
#include "google_apis/gaia/google_service_auth_error.h"
#include "google_apis/gaia/oauth2_token_service.h"
#include "services/identity/public/mojom/constants.mojom.h"
#include "services/service_manager/public/cpp/connector.h"
#include "services/service_manager/public/cpp/service_context.h"

#if BUILDFLAG(ENABLE_CROS_LIBASSISTANT)
#include "chromeos/assistant/internal/internal_constants.h"
#include "chromeos/services/assistant/assistant_manager_service_impl.h"
#include "chromeos/services/assistant/assistant_settings_manager_impl.h"
#include "services/device/public/mojom/battery_monitor.mojom.h"
#include "services/device/public/mojom/constants.mojom.h"
#else
#include "chromeos/services/assistant/fake_assistant_manager_service_impl.h"
#include "chromeos/services/assistant/fake_assistant_settings_manager_impl.h"
#endif

namespace chromeos {
namespace assistant {

namespace {

constexpr char kScopeAuthGcm[] = "https://www.googleapis.com/auth/gcm";
constexpr char kScopeAssistant[] =
    "https://www.googleapis.com/auth/assistant-sdk-prototype";

}  // namespace

Service::Service()
    : platform_binding_(this),
      session_observer_binding_(this),
      token_refresh_timer_(std::make_unique<base::OneShotTimer>()),
      main_thread_task_runner_(base::ThreadTaskRunnerHandle::Get()),
      weak_ptr_factory_(this) {
  registry_.AddInterface<mojom::AssistantPlatform>(base::BindRepeating(
      &Service::BindAssistantPlatformConnection, base::Unretained(this)));
}

Service::~Service() = default;

void Service::SetIdentityManagerForTesting(
    identity::mojom::IdentityManagerPtr identity_manager) {
  identity_manager_ = std::move(identity_manager);
}

void Service::SetAssistantManagerForTesting(
    std::unique_ptr<AssistantManagerService> assistant_manager_service) {
  assistant_manager_service_ = std::move(assistant_manager_service);
}

void Service::SetTimerForTesting(std::unique_ptr<base::OneShotTimer> timer) {
  token_refresh_timer_ = std::move(timer);
}

void Service::OnStart() {}

void Service::OnBindInterface(
    const service_manager::BindSourceInfo& source_info,
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle interface_pipe) {
  registry_.BindInterface(interface_name, std::move(interface_pipe));
}

void Service::BindAssistantConnection(mojom::AssistantRequest request) {
  // Assistant interface is supposed to be used when UI is actually in
  // use, which should be way later than assistant is created.
  DCHECK(assistant_manager_service_);
  DCHECK(assistant_manager_service_->GetState() ==
         AssistantManagerService::State::RUNNING);
  bindings_.AddBinding(assistant_manager_service_.get(), std::move(request));
}

void Service::BindAssistantPlatformConnection(
    mojom::AssistantPlatformRequest request) {
  platform_binding_.Bind(std::move(request));
}

void Service::OnSessionActivated(bool activated) {
  DCHECK(client_);
  session_active_ = activated;
  client_->OnAssistantStatusChanged(activated);
  UpdateListeningState();
}

void Service::OnLockStateChanged(bool locked) {
  locked_ = locked;
  UpdateListeningState();
}

void Service::BindAssistantSettingsManager(
    mojom::AssistantSettingsManagerRequest request) {
  DCHECK(assistant_settings_manager_);
  assistant_settings_manager_->BindRequest(std::move(request));
}

void Service::RequestAccessToken() {
  GetIdentityManager()->GetPrimaryAccountInfo(base::BindOnce(
      &Service::GetPrimaryAccountInfoCallback, base::Unretained(this)));
}

identity::mojom::IdentityManager* Service::GetIdentityManager() {
  if (!identity_manager_) {
    context()->connector()->BindInterface(
        identity::mojom::kServiceName, mojo::MakeRequest(&identity_manager_));
  }
  return identity_manager_.get();
}

void Service::Init(mojom::ClientPtr client,
                   mojom::ContextPtr assistant_context,
                   mojom::AudioInputPtr audio_input) {
  client_ = std::move(client);
#if BUILDFLAG(ENABLE_CROS_LIBASSISTANT)
  device::mojom::BatteryMonitorPtr battery_monitor;
  context()->connector()->BindInterface(device::mojom::kServiceName,
                                        mojo::MakeRequest(&battery_monitor));

  assistant_manager_service_ = std::make_unique<AssistantManagerServiceImpl>(
      std::move(audio_input), std::move(battery_monitor));
#else
  assistant_manager_service_ =
      std::make_unique<FakeAssistantManagerServiceImpl>();
#endif

  // This will eventually trigger the actual start of assistant services because
  // they all depend on it.
  RequestAccessToken();
}

void Service::GetPrimaryAccountInfoCallback(
    const base::Optional<AccountInfo>& account_info,
    const identity::AccountState& account_state) {
  if (!account_info.has_value() || !account_state.has_refresh_token ||
      account_info.value().gaia.empty()) {
    return;
  }
  account_id_ = AccountId::FromUserEmailGaiaId(account_info.value().email,
                                               account_info.value().gaia);
  OAuth2TokenService::ScopeSet scopes;
  scopes.insert(kScopeAssistant);
  scopes.insert(kScopeAuthGcm);
  identity_manager_->GetAccessToken(
      account_info.value().account_id, scopes, "cros_assistant",
      base::BindOnce(&Service::GetAccessTokenCallback, base::Unretained(this)));
}

void Service::GetAccessTokenCallback(const base::Optional<std::string>& token,
                                     base::Time expiration_time,
                                     const GoogleServiceAuthError& error) {
  if (!token.has_value()) {
    LOG(ERROR) << "Failed to retrieve token, error: " << error.ToString();
    return;
  }

  DCHECK(assistant_manager_service_);
  if (assistant_manager_service_->GetState() ==
      AssistantManagerService::State::STOPPED) {
    assistant_manager_service_->Start(
        token.value(),
        base::BindOnce(
            [](scoped_refptr<base::SingleThreadTaskRunner> task_runner,
               base::OnceCallback<void()> callback) {
              task_runner->PostTask(FROM_HERE, std::move(callback));
            },
            main_thread_task_runner_,
            base::BindOnce(&Service::FinalizeAssistantManagerService,
                           weak_ptr_factory_.GetWeakPtr())));
    DVLOG(1) << "Request Assistant start";
  } else {
    assistant_manager_service_->SetAccessToken(token.value());
  }

  token_refresh_timer_->Start(FROM_HERE, expiration_time - base::Time::Now(),
                              this, &Service::RequestAccessToken);
}

void Service::FinalizeAssistantManagerService() {
  DCHECK(assistant_manager_service_->GetState() ==
         AssistantManagerService::State::RUNNING);

  // Bind to Assistant controller in ash.
  ash::mojom::AssistantControllerPtr assistant_controller;
  context()->connector()->BindInterface(ash::mojom::kServiceName,
                                        &assistant_controller);
  mojom::AssistantPtr ptr;
  BindAssistantConnection(mojo::MakeRequest(&ptr));
  assistant_controller->SetAssistant(std::move(ptr));

  AddAshSessionObserver();
  registry_.AddInterface<mojom::Assistant>(base::BindRepeating(
      &Service::BindAssistantConnection, base::Unretained(this)));

  assistant_settings_manager_ =
      assistant_manager_service_.get()->GetAssistantSettingsManager();
  registry_.AddInterface<mojom::AssistantSettingsManager>(base::BindRepeating(
      &Service::BindAssistantSettingsManager, base::Unretained(this)));

  client_->OnAssistantStatusChanged(true);
  DVLOG(1) << "Assistant is running";
}

void Service::AddAshSessionObserver() {
  ash::mojom::SessionControllerPtr session_controller;
  context()->connector()->BindInterface(ash::mojom::kServiceName,
                                        &session_controller);
  ash::mojom::SessionActivationObserverPtr observer;
  session_observer_binding_.Bind(mojo::MakeRequest(&observer));
  session_controller->AddSessionActivationObserverForAccountId(
      account_id_, std::move(observer));
}

void Service::UpdateListeningState() {
  bool should_listen = !locked_ && session_active_;
  DVLOG(1) << "Update assistant listening state: " << should_listen;
  assistant_manager_service_->EnableListening(should_listen);
}

}  // namespace assistant
}  // namespace chromeos
