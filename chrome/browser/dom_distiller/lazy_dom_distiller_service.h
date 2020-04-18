// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_DOM_DISTILLER_LAZY_DOM_DISTILLER_SERVICE_H_
#define CHROME_BROWSER_DOM_DISTILLER_LAZY_DOM_DISTILLER_SERVICE_H_

#include <memory>

#include "base/macros.h"
#include "components/dom_distiller/core/dom_distiller_service.h"
#include "components/dom_distiller/core/task_tracker.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"

namespace content {
class NotificationSource;
class NotificationDetails;
}  // namespace content

class Profile;

namespace dom_distiller {

class DomDistillerServiceFactory;

// A class which helps with lazy instantiation of the DomDistillerService, using
// the BrowserContextKeyedServiceFactory for it. This class will delete itself
// when the profile is destroyed.
class LazyDomDistillerService : public DomDistillerServiceInterface,
                                public content::NotificationObserver {
 public:
  LazyDomDistillerService(Profile* profile,
                          const DomDistillerServiceFactory* service_factory);
  ~LazyDomDistillerService() override;

 public:
  // DomDistillerServiceInterface implementation:
  syncer::SyncableService* GetSyncableService() const override;
  const std::string AddToList(
      const GURL& url,
      std::unique_ptr<DistillerPage> distiller_page,
      const ArticleAvailableCallback& article_cb) override;
  bool HasEntry(const std::string& entry_id) override;
  std::string GetUrlForEntry(const std::string& entry_id) override;
  std::vector<ArticleEntry> GetEntries() const override;
  std::unique_ptr<ArticleEntry> RemoveEntry(
      const std::string& entry_id) override;
  std::unique_ptr<ViewerHandle> ViewEntry(
      ViewRequestDelegate* delegate,
      std::unique_ptr<DistillerPage> distiller_page,
      const std::string& entry_id) override;
  std::unique_ptr<ViewerHandle> ViewUrl(
      ViewRequestDelegate* delegate,
      std::unique_ptr<DistillerPage> distiller_page,
      const GURL& url) override;
  std::unique_ptr<DistillerPage> CreateDefaultDistillerPage(
      const gfx::Size& render_view_size) override;
  std::unique_ptr<DistillerPage> CreateDefaultDistillerPageWithHandle(
      std::unique_ptr<SourcePageHandle> handle) override;
  void AddObserver(DomDistillerObserver* observer) override;
  void RemoveObserver(DomDistillerObserver* observer) override;
  DistilledPagePrefs* GetDistilledPagePrefs() override;

 private:
  // Accessor method for the backing service instance.
  DomDistillerServiceInterface* instance() const;

  // content::NotificationObserver implementation:
  void Observe(int type,
               const content::NotificationSource& source,
               const content::NotificationDetails& details) override;

  // The Profile to use when retrieving the DomDistillerService and also the
  // profile to listen for destruction of.
  Profile* profile_;

  // A BrowserContextKeyedServiceFactory for the DomDistillerService.
  const DomDistillerServiceFactory* service_factory_;

  // Used to track when the profile is shut down.
  content::NotificationRegistrar registrar_;

  DISALLOW_COPY_AND_ASSIGN(LazyDomDistillerService);
};

}  // namespace dom_distiller

#endif  // CHROME_BROWSER_DOM_DISTILLER_LAZY_DOM_DISTILLER_SERVICE_H_
