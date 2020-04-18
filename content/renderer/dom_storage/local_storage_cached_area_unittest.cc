// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/dom_storage/local_storage_cached_area.h"

#include <stdint.h>

#include <list>

#include "base/bind.h"
#include "base/strings/utf_string_conversions.h"
#include "content/common/leveldb_wrapper.mojom.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "content/renderer/dom_storage/local_storage_cached_areas.h"
#include "content/renderer/dom_storage/mock_leveldb_wrapper.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/platform/scheduler/test/fake_renderer_scheduler.h"

namespace content {

class LocalStorageCachedAreaTest : public testing::Test {
 public:
  LocalStorageCachedAreaTest()
      : kOrigin(url::Origin::Create(GURL("http://dom_storage/"))),
        kKey(base::ASCIIToUTF16("key")),
        kValue(base::ASCIIToUTF16("value")),
        kPageUrl("http://dom_storage/page"),
        kStorageAreaId("7"),
        kSource(kPageUrl.spec() + "\n" + kStorageAreaId),
        renderer_scheduler_(new blink::scheduler::FakeRendererScheduler()),
        cached_areas_(&mock_leveldb_wrapper_, renderer_scheduler_.get()) {}

  const url::Origin kOrigin;
  const base::string16 kKey;
  const base::string16 kValue;
  const GURL kPageUrl;
  const std::string kStorageAreaId;
  const std::string kSource;

  bool IsCacheLoaded(LocalStorageCachedArea* cached_area) {
    return cached_area->map_.get();
  }

  bool IsIgnoringAllMutations(LocalStorageCachedArea* cached_area) {
    return cached_area->ignore_all_mutations_;
  }

  bool IsIgnoringKeyMutations(LocalStorageCachedArea* cached_area,
                              const base::string16& key) {
    return cached_area->ignore_key_mutations_.find(key) !=
           cached_area->ignore_key_mutations_.end();
  }

  void ResetAll(LocalStorageCachedArea* cached_area) {
    mock_leveldb_wrapper_.CompleteAllPendingCallbacks();
    mock_leveldb_wrapper_.ResetObservations();
    cached_area->Reset();
  }

  void ResetCacheOnly(LocalStorageCachedArea* cached_area) {
    cached_area->Reset();
  }

  static std::vector<uint8_t> String16ToUint8Vector(
      const base::string16& input) {
    return LocalStorageCachedArea::String16ToUint8Vector(
        input, LocalStorageCachedArea::FormatOption::kLocalStorageDetectFormat);
  }

  static base::string16 Uint8VectorToString16(
      const std::vector<uint8_t>& input) {
    return LocalStorageCachedArea::Uint8VectorToString16(
        input, LocalStorageCachedArea::FormatOption::kLocalStorageDetectFormat);
  }

 protected:
  TestBrowserThreadBundle test_browser_thread_bundle_;
  MockLevelDBWrapper mock_leveldb_wrapper_;
  std::unique_ptr<blink::scheduler::WebMainThreadScheduler> renderer_scheduler_;
  LocalStorageCachedAreas cached_areas_;
};

TEST_F(LocalStorageCachedAreaTest, Basics) {
  EXPECT_FALSE(mock_leveldb_wrapper_.HasBindings());
  scoped_refptr<LocalStorageCachedArea> cached_area =
      cached_areas_.GetCachedArea(kOrigin);
  EXPECT_EQ(kOrigin, cached_area->origin());
  EXPECT_TRUE(mock_leveldb_wrapper_.HasBindings());

  EXPECT_FALSE(IsCacheLoaded(cached_area.get()));

  const std::string kStorageAreaId = "123";
  EXPECT_EQ(0u, cached_area->GetLength());
  EXPECT_TRUE(cached_area->SetItem(kKey, kValue, kPageUrl, kStorageAreaId));
  EXPECT_EQ(1u, cached_area->GetLength());
  EXPECT_EQ(kKey, cached_area->GetKey(0).string());
  EXPECT_EQ(kValue, cached_area->GetItem(kKey).string());
  cached_area->RemoveItem(kKey, kPageUrl, kStorageAreaId);
  EXPECT_EQ(0u, cached_area->GetLength());
}

TEST_F(LocalStorageCachedAreaTest, Getters) {
  scoped_refptr<LocalStorageCachedArea> cached_area =
      cached_areas_.GetCachedArea(kOrigin);

  // GetLength, we expect to see one call to load in the db.
  EXPECT_FALSE(IsCacheLoaded(cached_area.get()));
  EXPECT_EQ(0u, cached_area->GetLength());
  EXPECT_TRUE(IsCacheLoaded(cached_area.get()));
  EXPECT_TRUE(mock_leveldb_wrapper_.observed_get_all());
  EXPECT_EQ(1u, mock_leveldb_wrapper_.pending_callbacks().size());
  EXPECT_TRUE(IsIgnoringAllMutations(cached_area.get()));
  mock_leveldb_wrapper_.CompleteAllPendingCallbacks();
  mock_leveldb_wrapper_.Flush();
  EXPECT_FALSE(IsIgnoringAllMutations(cached_area.get()));

  // GetKey, expect the one call to load.
  ResetAll(cached_area.get());
  EXPECT_FALSE(IsCacheLoaded(cached_area.get()));
  EXPECT_TRUE(cached_area->GetKey(2).is_null());
  EXPECT_TRUE(IsCacheLoaded(cached_area.get()));
  EXPECT_TRUE(mock_leveldb_wrapper_.observed_get_all());
  EXPECT_EQ(1u, mock_leveldb_wrapper_.pending_callbacks().size());

  // GetItem, ditto.
  ResetAll(cached_area.get());
  EXPECT_FALSE(IsCacheLoaded(cached_area.get()));
  EXPECT_TRUE(cached_area->GetItem(kKey).is_null());
  EXPECT_TRUE(IsCacheLoaded(cached_area.get()));
  EXPECT_TRUE(mock_leveldb_wrapper_.observed_get_all());
  EXPECT_EQ(1u, mock_leveldb_wrapper_.pending_callbacks().size());
}

TEST_F(LocalStorageCachedAreaTest, Setters) {
  scoped_refptr<LocalStorageCachedArea> cached_area =
      cached_areas_.GetCachedArea(kOrigin);

  // SetItem, we expect a call to load followed by a call to put in the db.
  EXPECT_FALSE(IsCacheLoaded(cached_area.get()));
  EXPECT_TRUE(cached_area->SetItem(kKey, kValue, kPageUrl, kStorageAreaId));
  mock_leveldb_wrapper_.Flush();
  EXPECT_TRUE(IsCacheLoaded(cached_area.get()));
  EXPECT_TRUE(mock_leveldb_wrapper_.observed_get_all());
  EXPECT_TRUE(mock_leveldb_wrapper_.observed_put());
  EXPECT_EQ(kSource, mock_leveldb_wrapper_.observed_source());
  EXPECT_EQ(String16ToUint8Vector(kKey), mock_leveldb_wrapper_.observed_key());
  EXPECT_EQ(String16ToUint8Vector(kValue),
            mock_leveldb_wrapper_.observed_value());
  EXPECT_EQ(2u, mock_leveldb_wrapper_.pending_callbacks().size());

  // Clear, we expect a just the one call to clear in the db since
  // there's no need to load the data prior to deleting it.
  ResetAll(cached_area.get());
  EXPECT_FALSE(IsCacheLoaded(cached_area.get()));
  cached_area->Clear(kPageUrl, kStorageAreaId);
  mock_leveldb_wrapper_.Flush();
  EXPECT_TRUE(IsCacheLoaded(cached_area.get()));
  EXPECT_TRUE(mock_leveldb_wrapper_.observed_delete_all());
  EXPECT_EQ(kSource, mock_leveldb_wrapper_.observed_source());
  EXPECT_EQ(1u, mock_leveldb_wrapper_.pending_callbacks().size());

  // RemoveItem with nothing to remove, expect just one call to load.
  ResetAll(cached_area.get());
  EXPECT_FALSE(IsCacheLoaded(cached_area.get()));
  cached_area->RemoveItem(kKey, kPageUrl, kStorageAreaId);
  mock_leveldb_wrapper_.Flush();
  EXPECT_TRUE(IsCacheLoaded(cached_area.get()));
  EXPECT_TRUE(mock_leveldb_wrapper_.observed_get_all());
  EXPECT_FALSE(mock_leveldb_wrapper_.observed_delete());
  EXPECT_EQ(1u, mock_leveldb_wrapper_.pending_callbacks().size());

  // RemoveItem with something to remove, expect a call to load followed
  // by a call to remove.
  ResetAll(cached_area.get());
  mock_leveldb_wrapper_
      .mutable_get_all_return_values()[String16ToUint8Vector(kKey)] =
      String16ToUint8Vector(kValue);
  EXPECT_FALSE(IsCacheLoaded(cached_area.get()));
  cached_area->RemoveItem(kKey, kPageUrl, kStorageAreaId);
  mock_leveldb_wrapper_.Flush();
  EXPECT_TRUE(IsCacheLoaded(cached_area.get()));
  EXPECT_TRUE(mock_leveldb_wrapper_.observed_get_all());
  EXPECT_TRUE(mock_leveldb_wrapper_.observed_delete());
  EXPECT_EQ(kSource, mock_leveldb_wrapper_.observed_source());
  EXPECT_EQ(String16ToUint8Vector(kKey), mock_leveldb_wrapper_.observed_key());
  EXPECT_EQ(2u, mock_leveldb_wrapper_.pending_callbacks().size());
}

TEST_F(LocalStorageCachedAreaTest, MutationsAreIgnoredUntilLoadCompletion) {
  scoped_refptr<LocalStorageCachedArea> cached_area =
      cached_areas_.GetCachedArea(kOrigin);
  mojom::LevelDBObserver* observer = cached_area.get();

  EXPECT_TRUE(cached_area->GetItem(kKey).is_null());
  EXPECT_TRUE(IsCacheLoaded(cached_area.get()));
  EXPECT_TRUE(IsIgnoringAllMutations(cached_area.get()));

  // Before load completion, the mutation should be ignored.
  observer->KeyAdded(String16ToUint8Vector(kKey), String16ToUint8Vector(kValue),
                     kSource);
  EXPECT_TRUE(cached_area->GetItem(kKey).is_null());

  // Call the load completion callback.
  mock_leveldb_wrapper_.CompleteOnePendingCallback(true);
  mock_leveldb_wrapper_.Flush();
  EXPECT_FALSE(IsIgnoringAllMutations(cached_area.get()));

  // Verify that mutations are now applied.
  observer->KeyAdded(String16ToUint8Vector(kKey), String16ToUint8Vector(kValue),
                     kSource);
  EXPECT_EQ(kValue, cached_area->GetItem(kKey).string());
}

TEST_F(LocalStorageCachedAreaTest, MutationsAreIgnoredUntilClearCompletion) {
  scoped_refptr<LocalStorageCachedArea> cached_area =
      cached_areas_.GetCachedArea(kOrigin);

  cached_area->Clear(kPageUrl, kStorageAreaId);
  mock_leveldb_wrapper_.Flush();
  EXPECT_TRUE(IsIgnoringAllMutations(cached_area.get()));
  mock_leveldb_wrapper_.CompleteOnePendingCallback(true);
  mock_leveldb_wrapper_.Flush();
  EXPECT_FALSE(IsIgnoringAllMutations(cached_area.get()));

  // Verify that calling Clear twice works as expected, the first
  // completion callback should have been cancelled.
  ResetCacheOnly(cached_area.get());
  cached_area->Clear(kPageUrl, kStorageAreaId);
  mock_leveldb_wrapper_.Flush();
  EXPECT_TRUE(IsIgnoringAllMutations(cached_area.get()));
  cached_area->Clear(kPageUrl, kStorageAreaId);
  mock_leveldb_wrapper_.Flush();
  EXPECT_TRUE(IsIgnoringAllMutations(cached_area.get()));
  mock_leveldb_wrapper_.CompleteOnePendingCallback(true);
  mock_leveldb_wrapper_.Flush();
  EXPECT_TRUE(IsIgnoringAllMutations(cached_area.get()));
  mock_leveldb_wrapper_.CompleteOnePendingCallback(true);
  mock_leveldb_wrapper_.Flush();
  EXPECT_FALSE(IsIgnoringAllMutations(cached_area.get()));
}

TEST_F(LocalStorageCachedAreaTest, KeyMutationsAreIgnoredUntilCompletion) {
  scoped_refptr<LocalStorageCachedArea> cached_area =
      cached_areas_.GetCachedArea(kOrigin);
  mojom::LevelDBObserver* observer = cached_area.get();

  // SetItem
  EXPECT_TRUE(cached_area->SetItem(kKey, kValue, kPageUrl, kStorageAreaId));
  mock_leveldb_wrapper_.CompleteOnePendingCallback(true);  // load completion
  mock_leveldb_wrapper_.Flush();
  EXPECT_FALSE(IsIgnoringAllMutations(cached_area.get()));
  EXPECT_TRUE(IsIgnoringKeyMutations(cached_area.get(), kKey));
  observer->KeyDeleted(String16ToUint8Vector(kKey), {0}, kSource);
  mock_leveldb_wrapper_.Flush();
  EXPECT_EQ(kValue, cached_area->GetItem(kKey).string());
  mock_leveldb_wrapper_.CompleteOnePendingCallback(true);  // set completion
  mock_leveldb_wrapper_.Flush();
  EXPECT_FALSE(IsIgnoringKeyMutations(cached_area.get(), kKey));

  // RemoveItem
  cached_area->RemoveItem(kKey, kPageUrl, kStorageAreaId);
  mock_leveldb_wrapper_.Flush();
  EXPECT_TRUE(IsIgnoringKeyMutations(cached_area.get(), kKey));
  mock_leveldb_wrapper_.CompleteOnePendingCallback(true);  // remove completion
  mock_leveldb_wrapper_.Flush();
  EXPECT_FALSE(IsIgnoringKeyMutations(cached_area.get(), kKey));

  // Multiple mutations to the same key.
  EXPECT_TRUE(cached_area->SetItem(kKey, kValue, kPageUrl, kStorageAreaId));
  cached_area->RemoveItem(kKey, kPageUrl, kStorageAreaId);
  mock_leveldb_wrapper_.Flush();
  EXPECT_TRUE(IsIgnoringKeyMutations(cached_area.get(), kKey));
  mock_leveldb_wrapper_.CompleteOnePendingCallback(true);  // set completion
  mock_leveldb_wrapper_.Flush();
  EXPECT_TRUE(IsIgnoringKeyMutations(cached_area.get(), kKey));
  mock_leveldb_wrapper_.CompleteOnePendingCallback(true);  // remove completion
  mock_leveldb_wrapper_.Flush();
  EXPECT_FALSE(IsIgnoringKeyMutations(cached_area.get(), kKey));

  // A failed set item operation should Reset the cache.
  EXPECT_TRUE(cached_area->SetItem(kKey, kValue, kPageUrl, kStorageAreaId));
  mock_leveldb_wrapper_.Flush();
  EXPECT_TRUE(IsIgnoringKeyMutations(cached_area.get(), kKey));
  mock_leveldb_wrapper_.CompleteOnePendingCallback(false);
  mock_leveldb_wrapper_.Flush();
  EXPECT_FALSE(IsCacheLoaded(cached_area.get()));
}

TEST_F(LocalStorageCachedAreaTest, StringEncoding) {
  base::string16 ascii_key = base::ASCIIToUTF16("simplekey");
  base::string16 non_ascii_key = base::ASCIIToUTF16("key");
  non_ascii_key.push_back(0xd83d);
  non_ascii_key.push_back(0xde00);
  EXPECT_EQ(Uint8VectorToString16(String16ToUint8Vector(ascii_key)), ascii_key);
  EXPECT_EQ(Uint8VectorToString16(String16ToUint8Vector(non_ascii_key)),
            non_ascii_key);
  EXPECT_LT(String16ToUint8Vector(ascii_key).size(), ascii_key.size() * 2);
  EXPECT_GT(String16ToUint8Vector(non_ascii_key).size(),
            non_ascii_key.size() * 2);
}

TEST_F(LocalStorageCachedAreaTest, BrowserDisconnect) {
  scoped_refptr<LocalStorageCachedArea> cached_area =
      cached_areas_.GetCachedArea(kOrigin);

  // GetLength to prime the cache.
  mock_leveldb_wrapper_
      .mutable_get_all_return_values()[String16ToUint8Vector(kKey)] =
      String16ToUint8Vector(kValue);
  EXPECT_EQ(1u, cached_area->GetLength());
  EXPECT_TRUE(IsCacheLoaded(cached_area.get()));
  mock_leveldb_wrapper_.CompleteAllPendingCallbacks();
  mock_leveldb_wrapper_.ResetObservations();

  // Now disconnect the pipe from the browser, simulating situations where the
  // browser might be forced to destroy the LevelDBWrapperImpl.
  mock_leveldb_wrapper_.CloseAllBindings();

  // Getters should still function.
  EXPECT_EQ(1u, cached_area->GetLength());
  EXPECT_EQ(kValue, cached_area->GetItem(kKey).string());

  // And setters should also still function.
  cached_area->RemoveItem(kKey, kPageUrl, kStorageAreaId);
  EXPECT_EQ(0u, cached_area->GetLength());
  EXPECT_TRUE(cached_area->GetItem(kKey).is_null());

  // Even resetting the cache should still allow class to function properly.
  ResetCacheOnly(cached_area.get());
  EXPECT_TRUE(cached_area->GetItem(kKey).is_null());
  EXPECT_TRUE(cached_area->SetItem(kKey, kValue, kPageUrl, kStorageAreaId));
  EXPECT_EQ(1u, cached_area->GetLength());
  EXPECT_EQ(kValue, cached_area->GetItem(kKey).string());
}

}  // namespace content
