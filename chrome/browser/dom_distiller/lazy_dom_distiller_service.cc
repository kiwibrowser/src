// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/dom_distiller/lazy_dom_distiller_service.h"

#include <utility>

#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/dom_distiller/dom_distiller_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "components/dom_distiller/core/distilled_page_prefs.h"
#include "components/dom_distiller/core/distiller_page.h"
#include "components/dom_distiller/core/dom_distiller_service.h"
#include "content/public/browser/notification_source.h"

namespace dom_distiller {

LazyDomDistillerService::LazyDomDistillerService(
    Profile* profile,
    const DomDistillerServiceFactory* service_factory)
    : profile_(profile), service_factory_(service_factory) {
  registrar_.Add(this,
                 chrome::NOTIFICATION_PROFILE_DESTROYED,
                 content::Source<Profile>(profile));
}

LazyDomDistillerService::~LazyDomDistillerService() {
}

// This will create an object and schedule work the first time it's called
// and just return an existing object after that.
DomDistillerServiceInterface* LazyDomDistillerService::instance() const {
  return service_factory_->GetForBrowserContext(profile_);
}

void LazyDomDistillerService::Observe(
    int type,
    const content::NotificationSource& source,
    const content::NotificationDetails& details) {
  DCHECK_EQ(chrome::NOTIFICATION_PROFILE_DESTROYED, type);
  DCHECK_EQ(profile_, content::Source<Profile>(source).ptr());
  delete this;
}

syncer::SyncableService* LazyDomDistillerService::GetSyncableService() const {
  return instance()->GetSyncableService();
}

bool LazyDomDistillerService::HasEntry(const std::string& entry_id) {
  return instance()->HasEntry(entry_id);
}

std::string LazyDomDistillerService::GetUrlForEntry(
    const std::string& entry_id) {
  return instance()->GetUrlForEntry(entry_id);
}

const std::string LazyDomDistillerService::AddToList(
    const GURL& url,
    std::unique_ptr<DistillerPage> distiller_page,
    const ArticleAvailableCallback& article_cb) {
  return instance()->AddToList(url, std::move(distiller_page), article_cb);
}

std::vector<ArticleEntry> LazyDomDistillerService::GetEntries() const {
  return instance()->GetEntries();
}

std::unique_ptr<ArticleEntry> LazyDomDistillerService::RemoveEntry(
    const std::string& entry_id) {
  return instance()->RemoveEntry(entry_id);
}

std::unique_ptr<ViewerHandle> LazyDomDistillerService::ViewEntry(
    ViewRequestDelegate* delegate,
    std::unique_ptr<DistillerPage> distiller_page,
    const std::string& entry_id) {
  return instance()->ViewEntry(delegate, std::move(distiller_page), entry_id);
}

std::unique_ptr<ViewerHandle> LazyDomDistillerService::ViewUrl(
    ViewRequestDelegate* delegate,
    std::unique_ptr<DistillerPage> distiller_page,
    const GURL& url) {
  return instance()->ViewUrl(delegate, std::move(distiller_page), url);
}

std::unique_ptr<DistillerPage>
LazyDomDistillerService::CreateDefaultDistillerPage(
    const gfx::Size& render_view_size) {
  return instance()->CreateDefaultDistillerPage(render_view_size);
}

std::unique_ptr<DistillerPage>
LazyDomDistillerService::CreateDefaultDistillerPageWithHandle(
    std::unique_ptr<SourcePageHandle> handle) {
  return instance()->CreateDefaultDistillerPageWithHandle(std::move(handle));
}

void LazyDomDistillerService::AddObserver(DomDistillerObserver* observer) {
  instance()->AddObserver(observer);
}

void LazyDomDistillerService::RemoveObserver(DomDistillerObserver* observer) {
  instance()->RemoveObserver(observer);
}

DistilledPagePrefs* LazyDomDistillerService::GetDistilledPagePrefs() {
  return instance()->GetDistilledPagePrefs();
}

}  // namespace dom_distiller
