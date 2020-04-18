// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/dom_distiller/dom_distiller_service_factory.h"

#include <utility>

#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/memory/singleton.h"
#include "components/dom_distiller/core/article_entry.h"
#include "components/dom_distiller/core/distiller.h"
#include "components/dom_distiller/core/dom_distiller_service.h"
#include "components/dom_distiller/core/dom_distiller_store.h"
#include "components/dom_distiller/ios/distiller_page_factory_ios.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/keyed_service/ios/browser_state_dependency_manager.h"
#include "components/leveldb_proto/proto_database.h"
#include "components/leveldb_proto/proto_database_impl.h"
#include "ios/chrome/browser/browser_state/browser_state_otr_helper.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/web/public/browser_state.h"
#include "ios/web/public/web_thread.h"

namespace {
// A simple wrapper for DomDistillerService to expose it as a
// KeyedService.
class DomDistillerKeyedService : public KeyedService,
                                 public dom_distiller::DomDistillerService {
 public:
  DomDistillerKeyedService(
      std::unique_ptr<dom_distiller::DomDistillerStoreInterface> store,
      std::unique_ptr<dom_distiller::DistillerFactory> distiller_factory,
      std::unique_ptr<dom_distiller::DistillerPageFactory>
          distiller_page_factory,
      std::unique_ptr<dom_distiller::DistilledPagePrefs> distilled_page_prefs)
      : DomDistillerService(std::move(store),
                            std::move(distiller_factory),
                            std::move(distiller_page_factory),
                            std::move(distilled_page_prefs)) {}

  ~DomDistillerKeyedService() override {}

 private:
  DISALLOW_COPY_AND_ASSIGN(DomDistillerKeyedService);
};
}  // namespace

namespace dom_distiller {

// static
DomDistillerServiceFactory* DomDistillerServiceFactory::GetInstance() {
  return base::Singleton<DomDistillerServiceFactory>::get();
}

// static
DomDistillerService* DomDistillerServiceFactory::GetForBrowserState(
    ios::ChromeBrowserState* browser_state) {
  return static_cast<DomDistillerKeyedService*>(
      GetInstance()->GetServiceForBrowserState(browser_state, true));
}

DomDistillerServiceFactory::DomDistillerServiceFactory()
    : BrowserStateKeyedServiceFactory(
          "DomDistillerService",
          BrowserStateDependencyManager::GetInstance()) {
}

DomDistillerServiceFactory::~DomDistillerServiceFactory() {}

std::unique_ptr<KeyedService>
DomDistillerServiceFactory::BuildServiceInstanceFor(
    web::BrowserState* context) const {
  std::unique_ptr<DistillerPageFactory> distiller_page_factory =
      std::make_unique<DistillerPageFactoryIOS>(context);

  std::unique_ptr<DistillerURLFetcherFactory> distiller_url_fetcher_factory =
      std::make_unique<DistillerURLFetcherFactory>(
          context->GetRequestContext());

  dom_distiller::proto::DomDistillerOptions options;
  std::unique_ptr<DistillerFactory> distiller_factory =
      std::make_unique<DistillerFactoryImpl>(
          std::move(distiller_url_fetcher_factory), options);
  std::unique_ptr<DistilledPagePrefs> distilled_page_prefs =
      std::make_unique<DistilledPagePrefs>(
          ios::ChromeBrowserState::FromBrowserState(context)->GetPrefs());

  return std::make_unique<DomDistillerKeyedService>(
      nullptr, std::move(distiller_factory), std::move(distiller_page_factory),
      std::move(distilled_page_prefs));
}

web::BrowserState* DomDistillerServiceFactory::GetBrowserStateToUse(
    web::BrowserState* context) const {
  // Makes normal profile and off-the-record profile use same service instance.
  return GetBrowserStateRedirectedInIncognito(context);
}

}  // namespace dom_distiller
