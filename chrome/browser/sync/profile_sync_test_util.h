// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SYNC_PROFILE_SYNC_TEST_UTIL_H_
#define CHROME_BROWSER_SYNC_PROFILE_SYNC_TEST_UTIL_H_

#include <memory>
#include <string>

#include "base/memory/ref_counted.h"
#include "base/run_loop.h"
#include "components/browser_sync/profile_sync_service_mock.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/notification_source.h"
#include "content/public/browser/notification_types.h"
#include "testing/gmock/include/gmock/gmock.h"

class KeyedService;
class Profile;
class TestingProfile;

namespace content {
class BrowserContext;
}

namespace syncer {
class SyncClient;
}

ACTION_P(Notify, type) {
  content::NotificationService::current()->Notify(
      type,
      content::NotificationService::AllSources(),
      content::NotificationService::NoDetails());
}

ACTION(QuitUIMessageLoop) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  base::RunLoop::QuitCurrentWhenIdleDeprecated();
}

// Helper methods for constructing ProfileSyncService mocks.
browser_sync::ProfileSyncService::InitParams
CreateProfileSyncServiceParamsForTest(Profile* profile);
browser_sync::ProfileSyncService::InitParams
CreateProfileSyncServiceParamsForTest(
    std::unique_ptr<syncer::SyncClient> sync_client,
    Profile* profile);

// A utility used by sync tests to create a TestingProfile with a Google
// Services username stored in a (Testing)PrefService.
std::unique_ptr<TestingProfile> MakeSignedInTestingProfile();

// Helper routine to be used in conjunction with
// BrowserContextKeyedServiceFactory::SetTestingFactory().
std::unique_ptr<KeyedService> BuildMockProfileSyncService(
    content::BrowserContext* context);

#endif  // CHROME_BROWSER_SYNC_PROFILE_SYNC_TEST_UTIL_H_
