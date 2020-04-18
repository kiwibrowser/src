// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_NTP_SNIPPETS_CONTEXTUAL_CONTENT_SUGGESTIONS_SERVICE_FACTORY_H_
#define CHROME_BROWSER_NTP_SNIPPETS_CONTEXTUAL_CONTENT_SUGGESTIONS_SERVICE_FACTORY_H_

#include <memory>

#include "base/macros.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

class Profile;

namespace base {
template <typename T>
struct DefaultSingletonTraits;
}  // namespace base

namespace contextual_suggestions {
class ContextualContentSuggestionsService;
}  // namespace contextual_suggestions

class ContextualContentSuggestionsServiceFactory
    : public BrowserContextKeyedServiceFactory {
 public:
  static ContextualContentSuggestionsServiceFactory* GetInstance();
  static contextual_suggestions::ContextualContentSuggestionsService*
  GetForProfile(Profile* profile);
  static contextual_suggestions::ContextualContentSuggestionsService*
  GetForProfileIfExists(Profile* profile);

 private:
  friend struct base::DefaultSingletonTraits<
      ContextualContentSuggestionsServiceFactory>;

  ContextualContentSuggestionsServiceFactory();
  ~ContextualContentSuggestionsServiceFactory() override;

  // BrowserContextKeyedServiceFactory implementation.
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;

  DISALLOW_COPY_AND_ASSIGN(ContextualContentSuggestionsServiceFactory);
};

#endif  // CHROME_BROWSER_NTP_SNIPPETS_CONTEXTUAL_CONTENT_SUGGESTIONS_SERVICE_FACTORY_H_
