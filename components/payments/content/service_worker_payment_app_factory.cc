// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/payments/content/service_worker_payment_app_factory.h"

#include <algorithm>
#include <vector>

#include "base/callback.h"
#include "base/logging.h"
#include "base/memory/ref_counted.h"
#include "base/memory/singleton.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/payments/content/installable_payment_app_crawler.h"
#include "components/payments/content/manifest_verifier.h"
#include "components/payments/content/payment_manifest_web_data_service.h"
#include "components/payments/content/utility/payment_manifest_parser.h"
#include "components/payments/core/features.h"
#include "components/payments/core/payment_manifest_downloader.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/stored_payment_app.h"
#include "content/public/browser/web_contents.h"
#include "net/url_request/url_request_context_getter.h"
#include "url/url_canon.h"

namespace payments {
namespace {

// Returns true if |requested| is empty or contains at least one of the items in
// |capabilities|.
template <typename T>
bool CapabilityMatches(const std::vector<T>& requested,
                       const std::vector<int32_t>& capabilities) {
  if (requested.empty())
    return true;
  for (const auto& request : requested) {
    if (base::ContainsValue(capabilities, static_cast<int32_t>(request)))
      return true;
  }
  return false;
}

// Returns true if the basic-card |capabilities| of the payment app match the
// |request|.
bool BasicCardCapabilitiesMatch(
    const std::vector<content::StoredCapabilities>& capabilities,
    const mojom::PaymentMethodDataPtr& request) {
  for (const auto& capability : capabilities) {
    if (CapabilityMatches(request->supported_networks,
                          capability.supported_card_networks) &&
        CapabilityMatches(request->supported_types,
                          capability.supported_card_types)) {
      return true;
    }
  }
  return capabilities.empty() && request->supported_networks.empty() &&
         request->supported_types.empty();
}

// Returns true if |app| supports at least one of the |requests|.
bool AppSupportsAtLeastOneRequestedMethodData(
    const content::StoredPaymentApp& app,
    const std::vector<mojom::PaymentMethodDataPtr>& requests) {
  for (const auto& enabled_method : app.enabled_methods) {
    for (const auto& request : requests) {
      auto it = std::find(request->supported_methods.begin(),
                          request->supported_methods.end(), enabled_method);
      if (it != request->supported_methods.end()) {
        if (enabled_method != "basic-card" ||
            BasicCardCapabilitiesMatch(app.capabilities, request)) {
          return true;
        }
      }
    }
  }
  return false;
}

void RemovePortNumbersFromScopesForTest(
    content::PaymentAppProvider::PaymentApps* apps) {
  url::Replacements<char> replacements;
  replacements.ClearPort();
  for (auto& app : *apps) {
    app.second->scope = app.second->scope.ReplaceComponents(replacements);
  }
}

class SelfDeletingServiceWorkerPaymentAppFactory {
 public:
  SelfDeletingServiceWorkerPaymentAppFactory() {}
  ~SelfDeletingServiceWorkerPaymentAppFactory() {}

  // After |callback| has fired, the factory refreshes its own cache in the
  // background. Once the cache has been refreshed, the factory invokes the
  // |finished_using_resources_callback|. At this point, it's safe to delete
  // this factory. Don't destroy the factory and don't call this method again
  // until |finished_using_resources_callback| has run.
  void GetAllPaymentApps(
      content::WebContents* web_contents,
      std::unique_ptr<PaymentManifestDownloader> downloader,
      scoped_refptr<PaymentManifestWebDataService> cache,
      const std::vector<mojom::PaymentMethodDataPtr>& requested_method_data,
      bool may_crawl_for_installable_payment_apps,
      ServiceWorkerPaymentAppFactory::GetAllPaymentAppsCallback callback,
      base::OnceClosure finished_using_resources_callback) {
    DCHECK(!verifier_);

    downloader_ = std::move(downloader);
    parser_ = std::make_unique<PaymentManifestParser>();
    cache_ = cache;
    verifier_ = std::make_unique<ManifestVerifier>(
        web_contents, downloader_.get(), parser_.get(), cache_.get());
    if (may_crawl_for_installable_payment_apps &&
        base::FeatureList::IsEnabled(
            features::kWebPaymentsJustInTimePaymentApp)) {
      // Construct crawler in constructor to allow it observe the web_contents.
      crawler_ = std::make_unique<InstallablePaymentAppCrawler>(
          web_contents, downloader_.get(), parser_.get(), cache_.get());
    }

    // Method data cannot be copied and is passed in as a const-ref, which
    // cannot be moved, so make a manual copy for using below.
    for (const auto& method_data : requested_method_data) {
      requested_method_data_.emplace_back(method_data.Clone());
    }
    callback_ = std::move(callback);
    finished_using_resources_callback_ =
        std::move(finished_using_resources_callback);

    content::PaymentAppProvider::GetInstance()->GetAllPaymentApps(
        web_contents->GetBrowserContext(),
        base::BindOnce(
            &SelfDeletingServiceWorkerPaymentAppFactory::OnGotAllPaymentApps,
            base::Unretained(this)));
  }

  void IgnorePortInAppScopeForTesting() {
    ignore_port_in_app_scope_for_testing_ = true;
  }

 private:
  void OnGotAllPaymentApps(content::PaymentAppProvider::PaymentApps apps) {
    if (ignore_port_in_app_scope_for_testing_)
      RemovePortNumbersFromScopesForTest(&apps);

    ServiceWorkerPaymentAppFactory::RemoveAppsWithoutMatchingMethodData(
        requested_method_data_, &apps);
    if (apps.empty()) {
      OnPaymentAppsVerified(std::move(apps));
      OnPaymentAppsVerifierFinishedUsingResources();
      return;
    }

    // The |verifier_| will invoke |OnPaymentAppsVerified| with the list of all
    // valid payment apps. This list may be empty, if none of the apps were
    // found to be valid.
    is_payment_verifier_finished_using_resources_ = false;
    verifier_->Verify(
        std::move(apps),
        base::BindOnce(
            &SelfDeletingServiceWorkerPaymentAppFactory::OnPaymentAppsVerified,
            base::Unretained(this)),
        base::BindOnce(&SelfDeletingServiceWorkerPaymentAppFactory::
                           OnPaymentAppsVerifierFinishedUsingResources,
                       base::Unretained(this)));
  }

  void OnPaymentAppsVerified(content::PaymentAppProvider::PaymentApps apps) {
    if (apps.empty() && crawler_ != nullptr) {
      // Crawls installable web payment apps if no web payment apps have been
      // installed.
      is_payment_app_crawler_finished_using_resources_ = false;
      crawler_->Start(
          requested_method_data_,
          base::BindOnce(
              &SelfDeletingServiceWorkerPaymentAppFactory::OnPaymentAppsCrawled,
              base::Unretained(this)),
          base::BindOnce(&SelfDeletingServiceWorkerPaymentAppFactory::
                             OnPaymentAppsCrawlerFinishedUsingResources,
                         base::Unretained(this)));
      return;
    }

    // Release crawler_ since it will not be used from now on.
    crawler_.reset();

    std::move(callback_).Run(
        std::move(apps),
        ServiceWorkerPaymentAppFactory::InstallablePaymentApps());
  }

  void OnPaymentAppsCrawled(
      std::map<GURL, std::unique_ptr<WebAppInstallationInfo>> apps_info) {
    std::move(callback_).Run(content::PaymentAppProvider::PaymentApps(),
                             std::move(apps_info));
  }

  void OnPaymentAppsCrawlerFinishedUsingResources() {
    crawler_.reset();

    is_payment_app_crawler_finished_using_resources_ = true;
    FinishUsingResourcesIfReady();
  }

  void OnPaymentAppsVerifierFinishedUsingResources() {
    verifier_.reset();

    is_payment_verifier_finished_using_resources_ = true;
    FinishUsingResourcesIfReady();
  }

  void FinishUsingResourcesIfReady() {
    if (is_payment_verifier_finished_using_resources_ &&
        is_payment_app_crawler_finished_using_resources_ &&
        !finished_using_resources_callback_.is_null()) {
      downloader_.reset();
      parser_.reset();
      std::move(finished_using_resources_callback_).Run();

      base::ThreadTaskRunnerHandle::Get()->DeleteSoon(FROM_HERE, this);
    }
  }

  std::unique_ptr<PaymentManifestDownloader> downloader_;
  std::unique_ptr<PaymentManifestParser> parser_;
  scoped_refptr<PaymentManifestWebDataService> cache_;
  std::vector<mojom::PaymentMethodDataPtr> requested_method_data_;
  ServiceWorkerPaymentAppFactory::GetAllPaymentAppsCallback callback_;
  base::OnceClosure finished_using_resources_callback_;

  std::unique_ptr<ManifestVerifier> verifier_;
  bool is_payment_verifier_finished_using_resources_ = true;

  std::unique_ptr<InstallablePaymentAppCrawler> crawler_;
  bool is_payment_app_crawler_finished_using_resources_ = true;

  bool ignore_port_in_app_scope_for_testing_ = false;

  DISALLOW_COPY_AND_ASSIGN(SelfDeletingServiceWorkerPaymentAppFactory);
};

}  // namespace

// static
ServiceWorkerPaymentAppFactory* ServiceWorkerPaymentAppFactory::GetInstance() {
  return base::Singleton<ServiceWorkerPaymentAppFactory>::get();
}

void ServiceWorkerPaymentAppFactory::GetAllPaymentApps(
    content::WebContents* web_contents,
    scoped_refptr<PaymentManifestWebDataService> cache,
    const std::vector<mojom::PaymentMethodDataPtr>& requested_method_data,
    bool may_crawl_for_installable_payment_apps,
    GetAllPaymentAppsCallback callback,
    base::OnceClosure finished_writing_cache_callback_for_testing) {
  SelfDeletingServiceWorkerPaymentAppFactory* self_delete_factory =
      new SelfDeletingServiceWorkerPaymentAppFactory();
  if (test_downloader_ != nullptr)
    self_delete_factory->IgnorePortInAppScopeForTesting();

  self_delete_factory->GetAllPaymentApps(
      web_contents,
      test_downloader_ == nullptr
          ? std::make_unique<payments::PaymentManifestDownloader>(
                content::BrowserContext::GetDefaultStoragePartition(
                    web_contents->GetBrowserContext())
                    ->GetURLRequestContext())
          : std::move(test_downloader_),
      cache, requested_method_data, may_crawl_for_installable_payment_apps,
      std::move(callback),
      std::move(finished_writing_cache_callback_for_testing));
}

ServiceWorkerPaymentAppFactory::ServiceWorkerPaymentAppFactory()
    : test_downloader_(nullptr) {}

ServiceWorkerPaymentAppFactory::~ServiceWorkerPaymentAppFactory() {}

// static
void ServiceWorkerPaymentAppFactory::RemoveAppsWithoutMatchingMethodData(
    const std::vector<mojom::PaymentMethodDataPtr>& requested_method_data,
    content::PaymentAppProvider::PaymentApps* apps) {
  for (auto it = apps->begin(); it != apps->end();) {
    if (AppSupportsAtLeastOneRequestedMethodData(*it->second,
                                                 requested_method_data)) {
      ++it;
    } else {
      it = apps->erase(it);
    }
  }
}

void ServiceWorkerPaymentAppFactory::
    SetDownloaderAndIgnorePortInAppScopeForTesting(
        std::unique_ptr<PaymentManifestDownloader> downloader) {
  test_downloader_ = std::move(downloader);
}

}  // namespace payments
