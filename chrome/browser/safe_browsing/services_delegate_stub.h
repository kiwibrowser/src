// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SAFE_BROWSING_SERVICES_DELEGATE_STUB_H_
#define CHROME_BROWSER_SAFE_BROWSING_SERVICES_DELEGATE_STUB_H_

#include "base/macros.h"
#include "chrome/browser/safe_browsing/services_delegate.h"

namespace safe_browsing {

// Dummy ServicesDelegate implementation. Create via ServicesDelegate::Create().
class ServicesDelegateStub : public ServicesDelegate {
 public:
  ServicesDelegateStub();
  ~ServicesDelegateStub() override;

 private:
  // ServicesDelegate:
  const scoped_refptr<SafeBrowsingDatabaseManager>& v4_local_database_manager()
      const override;
  void Initialize(bool v4_enabled = false) override;
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
  void CreatePasswordProtectionService(Profile* profile) override;
  void RemovePasswordProtectionService(Profile* profile) override;
  PasswordProtectionService* GetPasswordProtectionService(
      Profile* profile) const override;

  scoped_refptr<SafeBrowsingDatabaseManager> v4_local_database_manager_;

  DISALLOW_COPY_AND_ASSIGN(ServicesDelegateStub);
};

}  // namespace safe_browsing

#endif  // CHROME_BROWSER_SAFE_BROWSING_SERVICES_DELEGATE_STUB_H_
