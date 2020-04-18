// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_BOOKMARKS_BOOKMARK_MODEL_FACTORY_H_
#define CHROME_BROWSER_BOOKMARKS_BOOKMARK_MODEL_FACTORY_H_

#include "base/macros.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

namespace base {
template <typename T> struct DefaultSingletonTraits;
}

namespace bookmarks {
class BookmarkModel;
}

// Singleton that owns all BookmarkModels and associates them with
// BrowserContexts.
class BookmarkModelFactory : public BrowserContextKeyedServiceFactory {
 public:
  static bookmarks::BookmarkModel* GetForBrowserContext(
      content::BrowserContext* browser_context);

  static bookmarks::BookmarkModel* GetForBrowserContextIfExists(
      content::BrowserContext* browser_context);

  static BookmarkModelFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<BookmarkModelFactory>;

  BookmarkModelFactory();
  ~BookmarkModelFactory() override;

  // BrowserContextKeyedServiceFactory:
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;
  void RegisterProfilePrefs(
      user_prefs::PrefRegistrySyncable* registry) override;
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;
  bool ServiceIsNULLWhileTesting() const override;

  DISALLOW_COPY_AND_ASSIGN(BookmarkModelFactory);
};

#endif  // CHROME_BROWSER_BOOKMARKS_BOOKMARK_MODEL_FACTORY_H_
