// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_NTP_SNIPPETS_CONTENT_SUGGESTIONS_NOTIFIER_SERVICE_FACTORY_H_
#define CHROME_BROWSER_NTP_SNIPPETS_CONTENT_SUGGESTIONS_NOTIFIER_SERVICE_FACTORY_H_

#include <memory>

#include "base/macros.h"
#include "base/memory/singleton.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

class Profile;
class ContentSuggestionsNotifierService;

class ContentSuggestionsNotifierServiceFactory
    : public BrowserContextKeyedServiceFactory {
 public:
  static ContentSuggestionsNotifierServiceFactory* GetInstance();
  static ContentSuggestionsNotifierService* GetForProfile(Profile* profile);
  static ContentSuggestionsNotifierService* GetForProfileIfExists(
      Profile* profile);

 private:
  friend struct base::DefaultSingletonTraits<
      ContentSuggestionsNotifierServiceFactory>;

  ContentSuggestionsNotifierServiceFactory();
  ~ContentSuggestionsNotifierServiceFactory() override;

  // BrowserContextKeyedServiceFactory implementation.
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;

  DISALLOW_COPY_AND_ASSIGN(ContentSuggestionsNotifierServiceFactory);
};

#endif  // CHROME_BROWSER_NTP_SNIPPETS_CONTENT_SUGGESTIONS_NOTIFIER_SERVICE_FACTORY_H_
