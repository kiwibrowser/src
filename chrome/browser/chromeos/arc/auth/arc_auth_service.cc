// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/arc/auth/arc_auth_service.h"

#include <utility>

#include "base/command_line.h"
#include "base/memory/singleton.h"
#include "base/time/time.h"
#include "chrome/browser/chromeos/arc/arc_optin_uma.h"
#include "chrome/browser/chromeos/arc/arc_session_manager.h"
#include "chrome/browser/chromeos/arc/arc_util.h"
#include "chrome/browser/chromeos/arc/auth/arc_background_auth_code_fetcher.h"
#include "chrome/browser/chromeos/arc/auth/arc_robot_auth_code_fetcher.h"
#include "chrome/browser/chromeos/arc/policy/arc_policy_util.h"
#include "chrome/browser/lifetime/application_lifetime.h"
#include "chrome/browser/profiles/profile.h"
#include "chromeos/chromeos_switches.h"
#include "components/arc/arc_bridge_service.h"
#include "components/arc/arc_browser_context_keyed_service_factory_base.h"
#include "components/arc/arc_features.h"
#include "components/arc/arc_prefs.h"
#include "components/arc/arc_service_manager.h"
#include "components/arc/arc_supervision_transition.h"
#include "components/arc/arc_util.h"
#include "components/prefs/pref_service.h"
#include "components/user_manager/user_manager.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/storage_partition.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace arc {

namespace {

// Singleton factory for ArcAuthService.
class ArcAuthServiceFactory
    : public internal::ArcBrowserContextKeyedServiceFactoryBase<
          ArcAuthService,
          ArcAuthServiceFactory> {
 public:
  // Factory name used by ArcBrowserContextKeyedServiceFactoryBase.
  static constexpr const char* kName = "ArcAuthServiceFactory";

  static ArcAuthServiceFactory* GetInstance() {
    return base::Singleton<ArcAuthServiceFactory>::get();
  }

 private:
  friend struct base::DefaultSingletonTraits<ArcAuthServiceFactory>;

  ArcAuthServiceFactory() = default;
  ~ArcAuthServiceFactory() override = default;
};

// Convers mojom::ArcSignInStatus into ProvisiningResult.
ProvisioningResult ConvertArcSignInStatusToProvisioningResult(
    mojom::ArcSignInStatus reason) {
  using ArcSignInStatus = mojom::ArcSignInStatus;

#define MAP_PROVISIONING_RESULT(name) \
  case ArcSignInStatus::name:         \
    return ProvisioningResult::name

  switch (reason) {
    MAP_PROVISIONING_RESULT(UNKNOWN_ERROR);
    MAP_PROVISIONING_RESULT(MOJO_VERSION_MISMATCH);
    MAP_PROVISIONING_RESULT(MOJO_CALL_TIMEOUT);
    MAP_PROVISIONING_RESULT(DEVICE_CHECK_IN_FAILED);
    MAP_PROVISIONING_RESULT(DEVICE_CHECK_IN_TIMEOUT);
    MAP_PROVISIONING_RESULT(DEVICE_CHECK_IN_INTERNAL_ERROR);
    MAP_PROVISIONING_RESULT(GMS_NETWORK_ERROR);
    MAP_PROVISIONING_RESULT(GMS_SERVICE_UNAVAILABLE);
    MAP_PROVISIONING_RESULT(GMS_BAD_AUTHENTICATION);
    MAP_PROVISIONING_RESULT(GMS_SIGN_IN_FAILED);
    MAP_PROVISIONING_RESULT(GMS_SIGN_IN_TIMEOUT);
    MAP_PROVISIONING_RESULT(GMS_SIGN_IN_INTERNAL_ERROR);
    MAP_PROVISIONING_RESULT(CLOUD_PROVISION_FLOW_FAILED);
    MAP_PROVISIONING_RESULT(CLOUD_PROVISION_FLOW_TIMEOUT);
    MAP_PROVISIONING_RESULT(CLOUD_PROVISION_FLOW_INTERNAL_ERROR);
    MAP_PROVISIONING_RESULT(NO_NETWORK_CONNECTION);
    MAP_PROVISIONING_RESULT(CHROME_SERVER_COMMUNICATION_ERROR);
    MAP_PROVISIONING_RESULT(ARC_DISABLED);
    MAP_PROVISIONING_RESULT(SUCCESS);
    MAP_PROVISIONING_RESULT(SUCCESS_ALREADY_PROVISIONED);
  }
#undef MAP_PROVISIONING_RESULT

  NOTREACHED() << "unknown reason: " << static_cast<int>(reason);
  return ProvisioningResult::UNKNOWN_ERROR;
}

mojom::ChromeAccountType GetAccountType(const Profile* profile) {
  if (profile->IsChild())
    return mojom::ChromeAccountType::CHILD_ACCOUNT;

  return IsRobotAccountMode() ? mojom::ChromeAccountType::ROBOT_ACCOUNT
                              : mojom::ChromeAccountType::USER_ACCOUNT;
}

mojom::AccountInfoPtr CreateAccountInfo(bool is_enforced,
                                        const std::string& auth_info,
                                        const std::string& account_name,
                                        mojom::ChromeAccountType account_type,
                                        bool is_managed) {
  mojom::AccountInfoPtr account_info = mojom::AccountInfo::New();
  account_info->account_name = account_name;
  if (account_type == mojom::ChromeAccountType::ACTIVE_DIRECTORY_ACCOUNT) {
    account_info->enrollment_token = auth_info;
  } else {
    if (!is_enforced)
      account_info->auth_code = base::nullopt;
    else
      account_info->auth_code = auth_info;
  }
  account_info->account_type = account_type;
  account_info->is_managed = is_managed;
  return account_info;
}

}  // namespace

// static
const char ArcAuthService::kArcServiceName[] = "arc::ArcAuthService";

// static
ArcAuthService* ArcAuthService::GetForBrowserContext(
    content::BrowserContext* context) {
  return ArcAuthServiceFactory::GetForBrowserContext(context);
}

ArcAuthService::ArcAuthService(content::BrowserContext* browser_context,
                               ArcBridgeService* arc_bridge_service)
    : profile_(Profile::FromBrowserContext(browser_context)),
      arc_bridge_service_(arc_bridge_service),
      url_loader_factory_(
          content::BrowserContext::GetDefaultStoragePartition(profile_)
              ->GetURLLoaderFactoryForBrowserProcess()),
      weak_ptr_factory_(this) {
  arc_bridge_service_->auth()->SetHost(this);
  arc_bridge_service_->auth()->AddObserver(this);
}

ArcAuthService::~ArcAuthService() {
  arc_bridge_service_->auth()->RemoveObserver(this);
  arc_bridge_service_->auth()->SetHost(nullptr);
}

void ArcAuthService::OnConnectionClosed() {
  fetcher_.reset();
}

void ArcAuthService::OnAuthorizationComplete(mojom::ArcSignInStatus status,
                                             bool initial_signin) {
  if (!initial_signin) {
    // Note, UMA for initial signin is updated from ArcSessionManager.
    DCHECK_NE(mojom::ArcSignInStatus::SUCCESS_ALREADY_PROVISIONED, status);
    UpdateReauthorizationResultUMA(
        ConvertArcSignInStatusToProvisioningResult(status), profile_);
    return;
  }

  ArcSessionManager::Get()->OnProvisioningFinished(
      ConvertArcSignInStatusToProvisioningResult(status));
}

void ArcAuthService::OnSignInCompleteDeprecated() {
  OnAuthorizationComplete(mojom::ArcSignInStatus::SUCCESS,
                          true /* initial_signin */);
}

void ArcAuthService::OnSignInFailedDeprecated(mojom::ArcSignInStatus reason) {
  DCHECK_NE(mojom::ArcSignInStatus::SUCCESS, reason);
  OnAuthorizationComplete(reason, true /* initial_signin */);
}

void ArcAuthService::ReportMetrics(mojom::MetricsType metrics_type,
                                   int32_t value) {
  switch (metrics_type) {
    case mojom::MetricsType::NETWORK_WAITING_TIME_MILLISECONDS:
      UpdateAuthTiming("ArcAuth.NetworkWaitTime",
                       base::TimeDelta::FromMilliseconds(value));
      break;
    case mojom::MetricsType::CHECKIN_ATTEMPTS:
      UpdateAuthCheckinAttempts(value);
      break;
    case mojom::MetricsType::CHECKIN_TIME_MILLISECONDS:
      UpdateAuthTiming("ArcAuth.CheckinTime",
                       base::TimeDelta::FromMilliseconds(value));
      break;
    case mojom::MetricsType::SIGNIN_TIME_MILLISECONDS:
      UpdateAuthTiming("ArcAuth.SignInTime",
                       base::TimeDelta::FromMilliseconds(value));
      break;
    case mojom::MetricsType::ACCOUNT_CHECK_MILLISECONDS:
      UpdateAuthTiming("ArcAuth.AccountCheckTime",
                       base::TimeDelta::FromMilliseconds(value));
      break;
  }
}

void ArcAuthService::ReportAccountCheckStatus(
    mojom::AccountCheckStatus status) {
  UpdateAuthAccountCheckStatus(status);
}

void ArcAuthService::ReportSupervisionChangeStatus(
    mojom::SupervisionChangeStatus status) {
  switch (status) {
    case mojom::SupervisionChangeStatus::CLOUD_DPC_DISABLED:
    case mojom::SupervisionChangeStatus::CLOUD_DPC_ALREADY_DISABLED:
    case mojom::SupervisionChangeStatus::CLOUD_DPC_ENABLED:
    case mojom::SupervisionChangeStatus::CLOUD_DPC_ALREADY_ENABLED:
      profile_->GetPrefs()->SetInteger(
          prefs::kArcSupervisionTransition,
          static_cast<int>(ArcSupervisionTransition::NO_TRANSITION));
      // TODO(brunokim): notify potential observers.
      break;
    case mojom::SupervisionChangeStatus::INVALID_SUPERVISION_STATE:
    case mojom::SupervisionChangeStatus::CLOUD_DPC_DISABLING_FAILED:
    case mojom::SupervisionChangeStatus::CLOUD_DPC_ENABLING_FAILED:
    default:
      LOG(WARNING) << "Failed to changed supervision: " << status;
      // TODO(crbug/841939): Block ARC++ in case of Unicorn graduation failure.
  }
}

void ArcAuthService::OnAccountInfoReady(mojom::AccountInfoPtr account_info,
                                        mojom::ArcSignInStatus status) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  auto* instance = ARC_GET_INSTANCE_FOR_METHOD(arc_bridge_service_->auth(),
                                               OnAccountInfoReady);
  DCHECK(instance);
  instance->OnAccountInfoReady(std::move(account_info), status);
}

void ArcAuthService::RequestAccountInfo(bool initial_signin) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  // No other auth code-related operation may be in progress.
  DCHECK(!fetcher_);

  if (IsArcOptInVerificationDisabled()) {
    OnAccountInfoReady(
        CreateAccountInfo(
            false /* is_enforced */, std::string() /* auth_info */,
            std::string() /* auth_name */, GetAccountType(profile_),
            policy_util::IsAccountManaged(profile_)),
        mojom::ArcSignInStatus::SUCCESS);
    return;
  }

  if (IsActiveDirectoryUserForProfile(profile_)) {
    // For Active Directory enrolled devices, we get an enrollment token for a
    // managed Google Play account from DMServer.
    auto enrollment_token_fetcher =
        std::make_unique<ArcActiveDirectoryEnrollmentTokenFetcher>(
            ArcSessionManager::Get()->support_host());
    enrollment_token_fetcher->Fetch(
        base::BindOnce(&ArcAuthService::OnEnrollmentTokenFetched,
                       weak_ptr_factory_.GetWeakPtr()));
    fetcher_ = std::move(enrollment_token_fetcher);
    return;
  }
  // For non-AD enrolled devices an auth code is fetched.
  std::unique_ptr<ArcAuthCodeFetcher> auth_code_fetcher;
  if (IsRobotAccountMode()) {
    // In Kiosk and public session mode, use Robot auth code fetching.
    auth_code_fetcher = std::make_unique<ArcRobotAuthCodeFetcher>();
  } else {
    // Optionally retrieve auth code in silent mode.
    auth_code_fetcher = std::make_unique<ArcBackgroundAuthCodeFetcher>(
        url_loader_factory_, profile_, ArcSessionManager::Get()->auth_context(),
        initial_signin);
  }
  auth_code_fetcher->Fetch(base::Bind(&ArcAuthService::OnAuthCodeFetched,
                                      weak_ptr_factory_.GetWeakPtr()));
  fetcher_ = std::move(auth_code_fetcher);
}

void ArcAuthService::OnEnrollmentTokenFetched(
    ArcActiveDirectoryEnrollmentTokenFetcher::Status status,
    const std::string& enrollment_token,
    const std::string& user_id) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  fetcher_.reset();

  switch (status) {
    case ArcActiveDirectoryEnrollmentTokenFetcher::Status::SUCCESS: {
      // Save user_id to the user profile.
      profile_->GetPrefs()->SetString(prefs::kArcActiveDirectoryPlayUserId,
                                      user_id);

      // Send enrollment token to ARC.
      OnAccountInfoReady(
          CreateAccountInfo(true /* is_enforced */, enrollment_token,
                            std::string() /* account_name */,
                            mojom::ChromeAccountType::ACTIVE_DIRECTORY_ACCOUNT,
                            true),
          mojom::ArcSignInStatus::SUCCESS);
      break;
    }
    case ArcActiveDirectoryEnrollmentTokenFetcher::Status::FAILURE: {
      // Send error to ARC.
      OnAccountInfoReady(
          nullptr, mojom::ArcSignInStatus::CHROME_SERVER_COMMUNICATION_ERROR);
      break;
    }
    case ArcActiveDirectoryEnrollmentTokenFetcher::Status::ARC_DISABLED: {
      // Send error to ARC.
      OnAccountInfoReady(nullptr, mojom::ArcSignInStatus::ARC_DISABLED);
      break;
    }
  }
}

void ArcAuthService::OnAuthCodeFetched(bool success,
                                       const std::string& auth_code) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  fetcher_.reset();

  if (success) {
    OnAccountInfoReady(
        CreateAccountInfo(
            !IsArcOptInVerificationDisabled(), auth_code,
            ArcSessionManager::Get()->auth_context()->full_account_id(),
            GetAccountType(profile_), policy_util::IsAccountManaged(profile_)),
        mojom::ArcSignInStatus::SUCCESS);
  } else {
    // Send error to ARC.
    OnAccountInfoReady(
        nullptr, mojom::ArcSignInStatus::CHROME_SERVER_COMMUNICATION_ERROR);
  }
}

void ArcAuthService::SetURLLoaderFactoryForTesting(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory) {
  url_loader_factory_ = std::move(url_loader_factory);
}

}  // namespace arc
