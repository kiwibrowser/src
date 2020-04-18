// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_FEATURE_ENGAGEMENT_INCOGNITO_WINDOW_INCOGNITO_WINDOW_TRACKER_FACTORY_H_
#define CHROME_BROWSER_FEATURE_ENGAGEMENT_INCOGNITO_WINDOW_INCOGNITO_WINDOW_TRACKER_FACTORY_H_

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
class IncognitoWindowTracker;

// IncognitoWindowTrackerFactory is the main client class for interaction with
// the IncognitoWindowTracker component.
class IncognitoWindowTrackerFactory : public BrowserContextKeyedServiceFactory {
 public:
  // Returns singleton instance of IncognitoWindowTrackerFactory.
  static IncognitoWindowTrackerFactory* GetInstance();

  // Returns the FeatureEngagementTracker associated with the profile.
  IncognitoWindowTracker* GetForProfile(Profile* profile);

 private:
  friend struct base::DefaultSingletonTraits<IncognitoWindowTrackerFactory>;

  IncognitoWindowTrackerFactory();
  ~IncognitoWindowTrackerFactory() override;

  // BrowserContextKeyedServiceFactory overrides:
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;
  bool ServiceIsCreatedWithBrowserContext() const override;
  bool ServiceIsNULLWhileTesting() const override;

  DISALLOW_COPY_AND_ASSIGN(IncognitoWindowTrackerFactory);
};

}  // namespace feature_engagement

#endif  // CHROME_BROWSER_FEATURE_ENGAGEMENT_INCOGNITO_WINDOW_INCOGNITO_WINDOW_TRACKER_FACTORY_H_
