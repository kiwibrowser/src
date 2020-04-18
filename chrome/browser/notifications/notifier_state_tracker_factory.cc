// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/notifications/notifier_state_tracker_factory.h"

#include "chrome/browser/notifications/notifier_state_tracker.h"
#include "chrome/browser/permissions/permission_manager_factory.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"

// static
NotifierStateTracker*
NotifierStateTrackerFactory::GetForProfile(Profile* profile) {
  return static_cast<NotifierStateTracker*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

// static
NotifierStateTrackerFactory*
NotifierStateTrackerFactory::GetInstance() {
  return base::Singleton<NotifierStateTrackerFactory>::get();
}

NotifierStateTrackerFactory::NotifierStateTrackerFactory()
    : BrowserContextKeyedServiceFactory(
          "NotifierStateTracker",
          BrowserContextDependencyManager::GetInstance()) {
  DependsOn(PermissionManagerFactory::GetInstance());
}

NotifierStateTrackerFactory::~NotifierStateTrackerFactory() {}

KeyedService* NotifierStateTrackerFactory::BuildServiceInstanceFor(
    content::BrowserContext* profile) const {
  return new NotifierStateTracker(static_cast<Profile*>(profile));
}

content::BrowserContext*
NotifierStateTrackerFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return chrome::GetBrowserContextOwnInstanceInIncognito(context);
}
