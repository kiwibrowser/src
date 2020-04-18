// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/policy/core/common/cloud/resource_cache.h"

#include "base/bind.h"
#include "base/callback.h"
#include "base/files/scoped_temp_dir.h"
#include "base/test/test_simple_task_runner.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace policy {

namespace {

const char kKey1[] = "key 1";
const char kKey2[] = "key 2";
const char kKey3[] = "key 3";
const char kSubA[] = "a";
const char kSubB[] = "bb";
const char kSubC[] = "ccc";
const char kSubD[] = "dddd";
const char kSubE[] = "eeeee";

const char kData0[] = "{ \"key\": \"value\" }";
const char kData1[] = "{}";

bool Matches(const std::string& expected, const std::string& subkey) {
  return subkey == expected;
}

}  // namespace

TEST(ResourceCacheTest, StoreAndLoad) {
  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());
  ResourceCache cache(temp_dir.GetPath(),
                      base::MakeRefCounted<base::TestSimpleTaskRunner>());

  // No data initially.
  std::string data;
  EXPECT_FALSE(cache.Load(kKey1, kSubA, &data));

  // Store some data and load it.
  EXPECT_TRUE(cache.Store(kKey1, kSubA, kData0));
  EXPECT_TRUE(cache.Load(kKey1, kSubA, &data));
  EXPECT_EQ(kData0, data);

  // Store more data in another subkey.
  EXPECT_TRUE(cache.Store(kKey1, kSubB, kData1));

  // Write subkeys to two other keys.
  EXPECT_TRUE(cache.Store(kKey2, kSubA, kData0));
  EXPECT_TRUE(cache.Store(kKey2, kSubB, kData1));
  EXPECT_TRUE(cache.Store(kKey3, kSubA, kData0));
  EXPECT_TRUE(cache.Store(kKey3, kSubB, kData1));

  // Enumerate all the subkeys.
  std::map<std::string, std::string> contents;
  cache.LoadAllSubkeys(kKey1, &contents);
  EXPECT_EQ(2u, contents.size());
  EXPECT_EQ(kData0, contents[kSubA]);
  EXPECT_EQ(kData1, contents[kSubB]);

  // Store more subkeys.
  EXPECT_TRUE(cache.Store(kKey1, kSubC, kData1));
  EXPECT_TRUE(cache.Store(kKey1, kSubD, kData1));
  EXPECT_TRUE(cache.Store(kKey1, kSubE, kData1));

  // Now purge some of them.
  std::set<std::string> keep;
  keep.insert(kSubB);
  keep.insert(kSubD);
  cache.PurgeOtherSubkeys(kKey1, keep);

  // Enumerate all the remaining subkeys.
  cache.LoadAllSubkeys(kKey1, &contents);
  EXPECT_EQ(2u, contents.size());
  EXPECT_EQ(kData1, contents[kSubB]);
  EXPECT_EQ(kData1, contents[kSubD]);

  // Delete subkeys directly.
  cache.Delete(kKey1, kSubB);
  cache.Delete(kKey1, kSubD);
  cache.LoadAllSubkeys(kKey1, &contents);
  EXPECT_EQ(0u, contents.size());

  // The other two keys were not affected.
  cache.LoadAllSubkeys(kKey2, &contents);
  EXPECT_EQ(2u, contents.size());
  EXPECT_EQ(kData0, contents[kSubA]);
  EXPECT_EQ(kData1, contents[kSubB]);
  cache.LoadAllSubkeys(kKey3, &contents);
  EXPECT_EQ(2u, contents.size());
  EXPECT_EQ(kData0, contents[kSubA]);
  EXPECT_EQ(kData1, contents[kSubB]);

  // Now purge all keys except the third.
  keep.clear();
  keep.insert(kKey3);
  cache.PurgeOtherKeys(keep);

  // The first two keys are empty.
  cache.LoadAllSubkeys(kKey1, &contents);
  EXPECT_EQ(0u, contents.size());
  cache.LoadAllSubkeys(kKey1, &contents);
  EXPECT_EQ(0u, contents.size());

  // The third key is unaffected.
  cache.LoadAllSubkeys(kKey3, &contents);
  EXPECT_EQ(2u, contents.size());
  EXPECT_EQ(kData0, contents[kSubA]);
  EXPECT_EQ(kData1, contents[kSubB]);
}

TEST(ResourceCacheTest, FilterSubkeys) {
  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());
  ResourceCache cache(temp_dir.GetPath(),
                      base::MakeRefCounted<base::TestSimpleTaskRunner>());

  // Store some data.
  EXPECT_TRUE(cache.Store(kKey1, kSubA, kData0));
  EXPECT_TRUE(cache.Store(kKey1, kSubB, kData1));
  EXPECT_TRUE(cache.Store(kKey1, kSubC, kData0));
  EXPECT_TRUE(cache.Store(kKey2, kSubA, kData0));
  EXPECT_TRUE(cache.Store(kKey2, kSubB, kData1));
  EXPECT_TRUE(cache.Store(kKey3, kSubA, kData0));
  EXPECT_TRUE(cache.Store(kKey3, kSubB, kData1));

  // Check the contents of kKey1.
  std::map<std::string, std::string> contents;
  cache.LoadAllSubkeys(kKey1, &contents);
  EXPECT_EQ(3u, contents.size());
  EXPECT_EQ(kData0, contents[kSubA]);
  EXPECT_EQ(kData1, contents[kSubB]);
  EXPECT_EQ(kData0, contents[kSubC]);

  // Filter some subkeys.
  cache.FilterSubkeys(kKey1, base::Bind(&Matches, kSubA));

  // Check the contents of kKey1 again.
  cache.LoadAllSubkeys(kKey1, &contents);
  EXPECT_EQ(2u, contents.size());
  EXPECT_EQ(kData1, contents[kSubB]);
  EXPECT_EQ(kData0, contents[kSubC]);

  // Other keys weren't affected.
  cache.LoadAllSubkeys(kKey2, &contents);
  EXPECT_EQ(2u, contents.size());
  cache.LoadAllSubkeys(kKey3, &contents);
  EXPECT_EQ(2u, contents.size());
}

}  // namespace policy
