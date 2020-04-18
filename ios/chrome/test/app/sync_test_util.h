// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_TEST_APP_SYNC_TEST_UTIL_H_
#define IOS_CHROME_TEST_APP_SYNC_TEST_UTIL_H_

#import <Foundation/Foundation.h>
#include <string>

#include "components/sync/base/model_type.h"
#include "url/gurl.h"

namespace chrome_test_util {

// Sets up a fake sync server to be used by the ProfileSyncService.
void SetUpFakeSyncServer();

// Tears down the fake sync server used by the ProfileSyncService and restores
// the real one.
void TearDownFakeSyncServer();

// Starts the sync server. The server should not be running when calling this.
void StartSync();

// Stops the sync server. The server should be running when calling this.
void StopSync();

// Triggers a sync cycle for a |type|.
void TriggerSyncCycle(syncer::ModelType type);

// Gets the number of entities of the given |type|.
int GetNumberOfSyncEntities(syncer::ModelType type);

// Verifies that |count| entities of the given |type| and |name| exist on the
// sync FakeServer. Folders are not included in this count.
BOOL VerifyNumberOfSyncEntitiesWithName(syncer::ModelType type,
                                        std::string name,
                                        size_t count,
                                        NSError** error);

// Injects a bookmark into the fake sync server with |url| and |title|.
void InjectBookmarkOnFakeSyncServer(std::string url, std::string title);

// Injects an autofill profile into the fake sync server with |guid| and
// |full_name|.
void InjectAutofillProfileOnFakeSyncServer(std::string guid,
                                           std::string full_name);

// Deletes an autofill profile from the fake sync server with |guid|, if it
// exists. If it doesn't exist, nothing is done.
void DeleteAutofillProfileOnFakeSyncServer(std::string guid);

// Clears the autofill profile for the given |guid|.
void ClearAutofillProfile(std::string guid);

// Clears fake sync server data.
void ClearSyncServerData();

// Returns true if the sync backend server is intialized.
bool IsSyncInitialized();

// Returns the current sync cache guid. The sync server must be running when
// calling this.
std::string GetSyncCacheGuid();

// Returns true if there is an autofilll profile with the corresponding |guid|
// and |full_name|.
bool IsAutofillProfilePresent(std::string guid, std::string full_name);

// Verifies the sessions hierarchy on the Sync FakeServer. |expected_urls| is
// the collection of URLs that are to be expected for a single window. On
// failure, returns NO and |error| is set and includes a message. See the
// SessionsHierarchy class for documentation regarding the verification.
BOOL VerifySessionsOnSyncServer(const std::multiset<std::string>& expected_urls,
                                NSError** error);

// Adds typed URL into HistoryService.
void AddTypedURLOnClient(const GURL& url);

// Injects typed URL to sync FakeServer.
void InjectTypedURLOnFakeSyncServer(const std::string& url);

// Returns YES if the provided |url| is present (or not) if |expected_present|
// is YES (or NO).
BOOL IsTypedUrlPresentOnClient(const GURL& url,
                               BOOL expect_present,
                               NSError** error);

// Deletes typed URL from HistoryService.
void DeleteTypedUrlFromClient(const GURL& url);

// Deletes typed URL on FakeServer by injecting a tombstone.
void DeleteTypedUrlFromFakeSyncServer(std::string url);

}  // namespace chrome_test_util

#endif  // IOS_CHROME_TEST_APP_SYNC_TEST_UTIL_H_
