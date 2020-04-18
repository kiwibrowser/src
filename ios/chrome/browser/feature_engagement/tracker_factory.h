// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_FEATURE_ENGAGEMENT_TRACKER_FACTORY_H_
#define IOS_CHROME_BROWSER_FEATURE_ENGAGEMENT_TRACKER_FACTORY_H_

#include "base/macros.h"
#include "components/keyed_service/ios/browser_state_keyed_service_factory.h"

namespace base {
template <typename T>
struct DefaultSingletonTraits;
}  // namespace base

namespace ios {
class ChromeBrowserState;
}  // namespace ios

namespace feature_engagement {
class Tracker;

// TrackerFactory is the main class for interacting with the
// feature_engagement component. It uses the KeyedService API to
// expose functions to associate and retrieve a feature_engagement::Tracker
// object with a given ios::ChromeBrowserState object.
class TrackerFactory : public BrowserStateKeyedServiceFactory {
 public:
  // Returns the TrackerFactory singleton object.
  static TrackerFactory* GetInstance();

  // Retrieves the Tracker object associated with a given
  // browser state. It is created if it does not already exist.
  static Tracker* GetForBrowserState(ios::ChromeBrowserState* browser_state);

 protected:
  // BrowserStateKeyedServiceFactory implementation.
  std::unique_ptr<KeyedService> BuildServiceInstanceFor(
      web::BrowserState* context) const override;
  web::BrowserState* GetBrowserStateToUse(
      web::BrowserState* context) const override;

 private:
  friend struct base::DefaultSingletonTraits<TrackerFactory>;

  TrackerFactory();
  ~TrackerFactory() override;

  DISALLOW_COPY_AND_ASSIGN(TrackerFactory);
};

}  // namespace feature_engagement

#endif  // IOS_CHROME_BROWSER_FEATURE_ENGAGEMENT_TRACKER_FACTORY_H_
