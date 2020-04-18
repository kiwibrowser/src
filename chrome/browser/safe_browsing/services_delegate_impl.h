// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SAFE_BROWSING_SERVICES_DELEGATE_IMPL_H_
#define CHROME_BROWSER_SAFE_BROWSING_SERVICES_DELEGATE_IMPL_H_

#include <memory>

#include "base/macros.h"
#include "chrome/browser/safe_browsing/chrome_password_protection_service.h"
#include "chrome/browser/safe_browsing/client_side_detection_service.h"
#include "chrome/browser/safe_browsing/download_protection/download_protection_service.h"
#include "chrome/browser/safe_browsing/incident_reporting/incident_reporting_service.h"
#include "chrome/browser/safe_browsing/incident_reporting/resource_request_detector.h"
#include "chrome/browser/safe_browsing/services_delegate.h"
#include "components/safe_browsing/password_protection/password_protection_service.h"

namespace safe_browsing {

class SafeBrowsingDatabaseManager;

// Actual ServicesDelegate implementation. Create via
// ServicesDelegate::Create().
class ServicesDelegateImpl : public ServicesDelegate {
 public:
  ServicesDelegateImpl(SafeBrowsingService* safe_browsing_service,
                       ServicesDelegate::ServicesCreator* services_creator);
  ~ServicesDelegateImpl() override;

 private:
  // ServicesDelegate:
  const scoped_refptr<SafeBrowsingDatabaseManager>& v4_local_database_manager()
      const override;
  void Initialize(bool v4_enabled) override;
  void InitializeCsdService(scoped_refptr<network::SharedURLLoaderFactory>
                                url_loader_factory) override;
  void ShutdownServices() override;
  void RefreshState(bool enable) override;
  void ProcessResourceRequest(const ResourceRequestInfo* request) override;
  std::unique_ptr<prefs::mojom::TrackedPreferenceValidationDelegate>
  CreatePreferenceValidationDelegate(Profile* profile) override;
  void RegisterDelayedAnalysisCallback(
      const DelayedAnalysisCallback& callback) override;
  void AddDownloadManager(content::DownloadManager* download_manager) override;
  ClientSideDetectionService* GetCsdService() override;
  DownloadProtectionService* GetDownloadService() override;

  void StartOnIOThread(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      const V4ProtocolConfig& v4_config) override;
  void StopOnIOThread(bool shutdown) override;

  // Reports the current extended reporting level. Note that this is an
  // estimation and may not always be correct. It is possible that the
  // estimation finds both Scout and legacy extended reporting to be enabled.
  // This can happen, for instance, if one profile has Scout enabled and another
  // has legacy extended reporting enabled. In such a case, this method reports
  // LEGACY as the current level.
  ExtendedReportingLevel GetEstimatedExtendedReportingLevel() const;

  DownloadProtectionService* CreateDownloadProtectionService();
  IncidentReportingService* CreateIncidentReportingService();
  ResourceRequestDetector* CreateResourceRequestDetector();

  void CreatePasswordProtectionService(Profile* profile) override;
  void RemovePasswordProtectionService(Profile* profile) override;
  PasswordProtectionService* GetPasswordProtectionService(
      Profile* profile) const override;

  std::unique_ptr<ClientSideDetectionService> csd_service_;
  std::unique_ptr<DownloadProtectionService> download_service_;
  std::unique_ptr<IncidentReportingService> incident_service_;
  std::unique_ptr<ResourceRequestDetector> resource_request_detector_;

  SafeBrowsingService* const safe_browsing_service_;
  ServicesDelegate::ServicesCreator* const services_creator_;

  // The Pver4 local database manager handles the database and download logic
  // Accessed on both UI and IO thread.
  scoped_refptr<SafeBrowsingDatabaseManager> v4_local_database_manager_;

  // Tracks existing Profiles, and their corresponding
  // ChromePasswordProtectionService instances.
  // Accessed on UI thread.
  std::map<Profile*, std::unique_ptr<ChromePasswordProtectionService>>
      password_protection_service_map_;

  DISALLOW_COPY_AND_ASSIGN(ServicesDelegateImpl);
};

}  // namespace safe_browsing

#endif  // CHROME_BROWSER_SAFE_BROWSING_SERVICES_DELEGATE_IMPL_H_
