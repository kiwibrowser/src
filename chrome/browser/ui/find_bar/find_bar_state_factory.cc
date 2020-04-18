// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/find_bar/find_bar_state_factory.h"

#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/find_bar/find_bar_state.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"

// static
FindBarState* FindBarStateFactory::GetForProfile(Profile* profile) {
  return static_cast<FindBarState*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

// static
base::string16 FindBarStateFactory::GetLastPrepopulateText(Profile* p) {
  FindBarState* state = GetForProfile(p);
  base::string16 text = state->last_prepopulate_text();

  if (text.empty() && p->IsOffTheRecord()) {
    // Fall back to the original profile.
    state = GetForProfile(p->GetOriginalProfile());
    text = state->last_prepopulate_text();
  }

  return text;
}

// static
FindBarStateFactory* FindBarStateFactory::GetInstance() {
  return base::Singleton<FindBarStateFactory>::get();
}

FindBarStateFactory::FindBarStateFactory()
    : BrowserContextKeyedServiceFactory(
        "FindBarState",
        BrowserContextDependencyManager::GetInstance()) {
}

FindBarStateFactory::~FindBarStateFactory() {}

KeyedService* FindBarStateFactory::BuildServiceInstanceFor(
    content::BrowserContext* profile) const {
  return new FindBarState;
}

content::BrowserContext* FindBarStateFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return chrome::GetBrowserContextOwnInstanceInIncognito(context);
}
