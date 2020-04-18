// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/web/navigation/serializable_user_data_manager_impl.h"

#import "ios/web/public/test/fakes/test_web_state.h"
#import "testing/gtest_mac.h"
#include "testing/platform_test.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
// User Data and Key to use for tests.
NSString* const kTestUserData = @"TestUserData";
NSString* const kTestUserDataKey = @"TestUserDataKey";
}  // namespace

class SerializableUserDataManagerImplTest : public PlatformTest {
 protected:
  // Convenience getter for the user data manager.
  web::SerializableUserDataManager* manager() {
    return web::SerializableUserDataManager::FromWebState(&web_state_);
  }

  web::TestWebState web_state_;
};

// Test
TEST_F(SerializableUserDataManagerImplTest, TestLegacyKeyConversion) {
  NSDictionary<NSString*, NSString*>* legacy_key_conversion =
      web::SerializableUserDataImpl::GetLegacyKeyConversion();

  // Create data mapping legacy key to itself.
  NSMutableData* data = [[NSMutableData alloc] init];
  NSKeyedArchiver* archiver =
      [[NSKeyedArchiver alloc] initForWritingWithMutableData:data];
  for (NSString* key : [legacy_key_conversion allKeys]) {
    [archiver encodeObject:key forKey:key];
  }
  [archiver finishEncoding];

  // Decode data and check that legacy key have been converted.
  std::unique_ptr<web::SerializableUserData> user_data =
      web::SerializableUserData::Create();
  NSKeyedUnarchiver* unarchiver =
      [[NSKeyedUnarchiver alloc] initForReadingWithData:data];
  user_data->Decode(unarchiver);

  web::TestWebState web_state;
  web::SerializableUserDataManager* user_data_manager =
      web::SerializableUserDataManager::FromWebState(&web_state);
  user_data_manager->AddSerializableUserData(user_data.get());

  // Check that all key have been converted.
  for (NSString* key : [legacy_key_conversion allKeys]) {
    id value = user_data_manager->GetValueForSerializationKey(key);
    EXPECT_NSEQ(nil, value);
    value = user_data_manager->GetValueForSerializationKey(
        [legacy_key_conversion objectForKey:key]);
    EXPECT_NSEQ(key, value);
  }
}
