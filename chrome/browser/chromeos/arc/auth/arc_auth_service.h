// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_ARC_AUTH_ARC_AUTH_SERVICE_H_
#define CHROME_BROWSER_CHROMEOS_ARC_AUTH_ARC_AUTH_SERVICE_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "chrome/browser/chromeos/arc/auth/arc_active_directory_enrollment_token_fetcher.h"
#include "components/arc/common/auth.mojom.h"
#include "components/arc/connection_observer.h"
#include "components/keyed_service/core/keyed_service.h"

class Profile;

namespace content {
class BrowserContext;
}  // namespace content

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

namespace arc {

class ArcFetcherBase;
class ArcBridgeService;

// Implementation of ARC authorization.
class ArcAuthService : public KeyedService,
                       public mojom::AuthHost,
                       public ConnectionObserver<mojom::AuthInstance> {
 public:
  // Returns singleton instance for the given BrowserContext,
  // or nullptr if the browser |context| is not allowed to use ARC.
  static ArcAuthService* GetForBrowserContext(content::BrowserContext* context);

  ArcAuthService(content::BrowserContext* profile,
                 ArcBridgeService* bridge_service);
  ~ArcAuthService() override;

  // For supporting ArcServiceManager::GetService<T>().
  static const char kArcServiceName[];

  // ConnectionObserver<mojom::AuthInstance>:
  void OnConnectionClosed() override;

  // mojom::AuthHost:
  void OnAuthorizationComplete(mojom::ArcSignInStatus status,
                               bool initial_signin) override;
  void OnSignInCompleteDeprecated() override;
  void OnSignInFailedDeprecated(mojom::ArcSignInStatus reason) override;
  void RequestAccountInfo(bool initial_signin) override;
  void ReportMetrics(mojom::MetricsType metrics_type, int32_t value) override;
  void ReportAccountCheckStatus(mojom::AccountCheckStatus status) override;
  void ReportSupervisionChangeStatus(
      mojom::SupervisionChangeStatus status) override;

  void SetURLLoaderFactoryForTesting(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);

 private:
  // Callbacks when auth info is fetched.
  void OnEnrollmentTokenFetched(
      ArcActiveDirectoryEnrollmentTokenFetcher::Status status,
      const std::string& enrollment_token,
      const std::string& user_id);
  void OnAuthCodeFetched(bool success, const std::string& auth_code);

  // Called to let ARC container know the account info.
  void OnAccountInfoReady(mojom::AccountInfoPtr account_info,
                          mojom::ArcSignInStatus status);

  Profile* const profile_;
  ArcBridgeService* const arc_bridge_service_;
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;

  std::unique_ptr<ArcFetcherBase> fetcher_;

  base::WeakPtrFactory<ArcAuthService> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(ArcAuthService);
};

}  // namespace arc

#endif  // CHROME_BROWSER_CHROMEOS_ARC_AUTH_ARC_AUTH_SERVICE_H_
