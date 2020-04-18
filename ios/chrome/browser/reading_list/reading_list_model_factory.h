// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_READING_LIST_READING_LIST_MODEL_FACTORY_H_
#define IOS_CHROME_BROWSER_READING_LIST_READING_LIST_MODEL_FACTORY_H_

#include <memory>

#include "components/keyed_service/ios/browser_state_keyed_service_factory.h"

namespace base {
template <typename T>
struct DefaultSingletonTraits;
}  // namespace base

class ReadingListModel;

namespace ios {
class ChromeBrowserState;
}

// Singleton that creates the ReadingListModel and associates that service with
// ios::ChromeBrowserState.
class ReadingListModelFactory : public BrowserStateKeyedServiceFactory {
 public:
  static ReadingListModel* GetForBrowserState(
      ios::ChromeBrowserState* browser_state);
  static ReadingListModel* GetForBrowserStateIfExists(
      ios::ChromeBrowserState* browser_state);
  static ReadingListModelFactory* GetInstance();
  void RegisterBrowserStatePrefs(
      user_prefs::PrefRegistrySyncable* registry) override;

 private:
  friend struct base::DefaultSingletonTraits<ReadingListModelFactory>;

  ReadingListModelFactory();
  ~ReadingListModelFactory() override;

  // BrowserStateKeyedServiceFactory implementation.
  std::unique_ptr<KeyedService> BuildServiceInstanceFor(
      web::BrowserState* context) const override;
  web::BrowserState* GetBrowserStateToUse(
      web::BrowserState* context) const override;

  DISALLOW_COPY_AND_ASSIGN(ReadingListModelFactory);
};

#endif  // IOS_CHROME_BROWSER_READING_LIST_READING_LIST_MODEL_FACTORY_H_
