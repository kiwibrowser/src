// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ntp_tiles/chrome_most_visited_sites_factory.h"

#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/callback.h"
#include "chrome/browser/favicon/favicon_service_factory.h"
#include "chrome/browser/favicon/large_icon_service_factory.h"
#include "chrome/browser/history/top_sites_factory.h"
#include "chrome/browser/ntp_tiles/chrome_popular_sites_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/search/suggestions/image_decoder_impl.h"
#include "chrome/browser/search/suggestions/suggestions_service_factory.h"
#include "chrome/browser/supervised_user/supervised_user_service.h"
#include "chrome/browser/supervised_user/supervised_user_service_factory.h"
#include "chrome/browser/supervised_user/supervised_user_service_observer.h"
#include "chrome/browser/supervised_user/supervised_user_url_filter.h"
#include "chrome/browser/thumbnails/thumbnail_list_source.h"
#include "components/history/core/browser/top_sites.h"
#include "components/image_fetcher/core/image_fetcher_impl.h"
#include "components/ntp_tiles/icon_cacher_impl.h"
#include "components/ntp_tiles/metrics.h"
#include "components/ntp_tiles/most_visited_sites.h"

using suggestions::SuggestionsServiceFactory;

namespace {

class SupervisorBridge : public ntp_tiles::MostVisitedSitesSupervisor,
                         public SupervisedUserServiceObserver {
 public:
  explicit SupervisorBridge(Profile* profile);
  ~SupervisorBridge() override;

  void SetObserver(Observer* observer) override;
  bool IsBlocked(const GURL& url) override;
  std::vector<MostVisitedSitesSupervisor::Whitelist> GetWhitelists() override;
  bool IsChildProfile() override;

  // SupervisedUserServiceObserver implementation.
  void OnURLFilterChanged() override;

 private:
  Profile* const profile_;
  Observer* supervisor_observer_;
  ScopedObserver<SupervisedUserService, SupervisedUserServiceObserver>
      register_observer_;
};

SupervisorBridge::SupervisorBridge(Profile* profile)
    : profile_(profile),
      supervisor_observer_(nullptr),
      register_observer_(this) {
  register_observer_.Add(SupervisedUserServiceFactory::GetForProfile(profile_));
}

SupervisorBridge::~SupervisorBridge() {}

void SupervisorBridge::SetObserver(Observer* new_observer) {
  if (new_observer) {
    DCHECK(!supervisor_observer_);
  } else {
    DCHECK(supervisor_observer_);
  }

  supervisor_observer_ = new_observer;
}

bool SupervisorBridge::IsBlocked(const GURL& url) {
  SupervisedUserService* supervised_user_service =
      SupervisedUserServiceFactory::GetForProfile(profile_);
  auto* url_filter = supervised_user_service->GetURLFilter();
  return url_filter->GetFilteringBehaviorForURL(url) ==
         SupervisedUserURLFilter::FilteringBehavior::BLOCK;
}

std::vector<ntp_tiles::MostVisitedSitesSupervisor::Whitelist>
SupervisorBridge::GetWhitelists() {
  std::vector<MostVisitedSitesSupervisor::Whitelist> results;
  SupervisedUserService* supervised_user_service =
      SupervisedUserServiceFactory::GetForProfile(profile_);
  for (const auto& whitelist : supervised_user_service->whitelists()) {
    results.emplace_back(Whitelist{
        whitelist->title(), whitelist->entry_point(),
        whitelist->large_icon_path(),
    });
  }
  return results;
}

bool SupervisorBridge::IsChildProfile() {
  return profile_->IsChild();
}

void SupervisorBridge::OnURLFilterChanged() {
  if (supervisor_observer_) {
    supervisor_observer_->OnBlockedSitesChanged();
  }
}

}  // namespace

// static
std::unique_ptr<ntp_tiles::MostVisitedSites>
ChromeMostVisitedSitesFactory::NewForProfile(Profile* profile) {
  // MostVisitedSites doesn't exist in incognito profiles.
  if (profile->IsOffTheRecord()) {
    return nullptr;
  }

  return std::make_unique<ntp_tiles::MostVisitedSites>(
      profile->GetPrefs(), TopSitesFactory::GetForProfile(profile),
      SuggestionsServiceFactory::GetForProfile(profile),
#if defined(OS_ANDROID)
      ChromePopularSitesFactory::NewForProfile(profile),
#else
      nullptr,
#endif
      std::make_unique<ntp_tiles::IconCacherImpl>(
          FaviconServiceFactory::GetForProfile(
              profile, ServiceAccessType::IMPLICIT_ACCESS),
          LargeIconServiceFactory::GetForBrowserContext(profile),
          std::make_unique<image_fetcher::ImageFetcherImpl>(
              std::make_unique<suggestions::ImageDecoderImpl>(),
              profile->GetRequestContext())),
      std::make_unique<SupervisorBridge>(profile));
}
