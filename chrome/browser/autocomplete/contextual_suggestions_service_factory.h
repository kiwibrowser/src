// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_AUTOCOMPLETE_CONTEXTUAL_SUGGESTIONS_SERVICE_FACTORY_H_
#define CHROME_BROWSER_AUTOCOMPLETE_CONTEXTUAL_SUGGESTIONS_SERVICE_FACTORY_H_

#include "base/macros.h"
#include "base/memory/singleton.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

class ContextualSuggestionsService;
class Profile;

class ContextualSuggestionsServiceFactory
    : public BrowserContextKeyedServiceFactory {
 public:
  static ContextualSuggestionsService* GetForProfile(Profile* profile,
                                                     bool create_if_necessary);
  static ContextualSuggestionsServiceFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<
      ContextualSuggestionsServiceFactory>;

  ContextualSuggestionsServiceFactory();
  ~ContextualSuggestionsServiceFactory() override;

  // Overrides from BrowserContextKeyedServiceFactory:
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;

  DISALLOW_COPY_AND_ASSIGN(ContextualSuggestionsServiceFactory);
};

#endif  // CHROME_BROWSER_AUTOCOMPLETE_CONTEXTUAL_SUGGESTIONS_SERVICE_FACTORY_H_
