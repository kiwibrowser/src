// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_FEATURE_ENGAGEMENT_BOOKMARK_BOOKMARK_TRACKER_FACTORY_H_
#define CHROME_BROWSER_FEATURE_ENGAGEMENT_BOOKMARK_BOOKMARK_TRACKER_FACTORY_H_

#include "base/macros.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

class Profile;

namespace base {
template <typename T>
struct DefaultSingletonTraits;
}  // namespace base

namespace content {
class BrowserContext;
}  // namespace content

namespace feature_engagement {
class BookmarkTracker;

// BookmarkTrackerFactory is the main client class for interaction with the
// BookmarkTracker component.
class BookmarkTrackerFactory : public BrowserContextKeyedServiceFactory {
 public:
  // Returns singleton instance of BookmarkTrackerFactory.
  static BookmarkTrackerFactory* GetInstance();

  // Returns the FeatureEngagementTracker associated with the profile.
  BookmarkTracker* GetForProfile(Profile* profile);

 private:
  friend struct base::DefaultSingletonTraits<BookmarkTrackerFactory>;

  BookmarkTrackerFactory();
  ~BookmarkTrackerFactory() override;

  // BrowserContextKeyedServiceFactory overrides:
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;
  bool ServiceIsNULLWhileTesting() const override;

  DISALLOW_COPY_AND_ASSIGN(BookmarkTrackerFactory);
};

}  // namespace feature_engagement

#endif  // CHROME_BROWSER_FEATURE_ENGAGEMENT_BOOKMARK_BOOKMARK_TRACKER_FACTORY_H_
