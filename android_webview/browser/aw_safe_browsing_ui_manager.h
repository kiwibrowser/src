// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// The Safe Browsing service is responsible for downloading anti-phishing and
// anti-malware tables and checking urls against them. This is android_webview
// specific ui_manager.

#ifndef ANDROID_WEBVIEW_BROWSER_AW_SAFE_BROWSING_UI_MANAGER_H_
#define ANDROID_WEBVIEW_BROWSER_AW_SAFE_BROWSING_UI_MANAGER_H_

#include "components/safe_browsing/base_ui_manager.h"
#include "content/public/browser/web_contents.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"

class PrefService;

namespace network {
class SharedURLLoaderFactory;
}

namespace safe_browsing {
class BasePingManager;
class SafeBrowsingNetworkContext;
class SafeBrowsingURLRequestContextGetter;
}  // namespace

namespace android_webview {
class AwURLRequestContextGetter;

class AwSafeBrowsingUIManager : public safe_browsing::BaseUIManager {
 public:
  class UIManagerClient {
   public:
    static UIManagerClient* FromWebContents(content::WebContents* web_contents);

    // Whether this web contents can show any sort of interstitial
    virtual bool CanShowInterstitial() = 0;

    // Returns the appropriate BaseBlockingPage::ErrorUiType
    virtual int GetErrorUiType() = 0;
  };

  // Construction needs to happen on the UI thread.
  AwSafeBrowsingUIManager(
      AwURLRequestContextGetter* browser_url_request_context_getter,
      PrefService* pref_service);

  // Gets the correct ErrorUiType for the web contents
  int GetErrorUiType(const UnsafeResource& resource) const;

  // BaseUIManager methods:
  void DisplayBlockingPage(const UnsafeResource& resource) override;

  // Called on the UI thread by the ThreatDetails with the serialized
  // protocol buffer, so the service can send it over.
  void SendSerializedThreatDetails(const std::string& serialized) override;

  void SetExtendedReportingAllowed(bool allowed);

  // Called on the IO thread to get a SharedURLLoaderFactory that can be used on
  // the IO thread.
  scoped_refptr<network::SharedURLLoaderFactory>
  GetURLLoaderFactoryOnIOThread();

 protected:
  ~AwSafeBrowsingUIManager() override;

  void ShowBlockingPageForResource(const UnsafeResource& resource) override;

 private:
  // Called on the UI thread to create a URLLoaderFactory interface ptr for
  // the IO thread.
  void CreateURLLoaderFactoryForIO(
      network::mojom::URLLoaderFactoryRequest request);

  // Provides phishing and malware statistics. Accessed on IO thread.
  std::unique_ptr<safe_browsing::BasePingManager> ping_manager_;

  // The SafeBrowsingURLRequestContextGetter used to access
  // |url_request_context_|. Accessed on UI thread.
  // This is only valid if the network service is disabled.
  scoped_refptr<safe_browsing::SafeBrowsingURLRequestContextGetter>
      url_request_context_getter_;

  // If the network service is disabled, this is a wrapper around
  // |url_request_context_getter_|. Otherwise it's what owns the
  // URLRequestContext inside the network service. This is used by
  // SimpleURLLoader for safe browsing requests.
  std::unique_ptr<safe_browsing::SafeBrowsingNetworkContext> network_context_;

  // A SharedURLLoaderFactory and its interfaceptr used on the IO thread.
  network::mojom::URLLoaderFactoryPtr url_loader_factory_on_io_;
  scoped_refptr<network::WeakWrapperSharedURLLoaderFactory>
      shared_url_loader_factory_on_io_;

  // non-owning
  PrefService* pref_service_;

  DISALLOW_COPY_AND_ASSIGN(AwSafeBrowsingUIManager);
};

}  // namespace android_webview

#endif  // ANDROID_WEBVIEW_BROWSER_AW_SAFE_BROWSING_UI_MANAGER_H_
