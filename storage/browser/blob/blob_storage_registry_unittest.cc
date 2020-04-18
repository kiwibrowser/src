// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "storage/browser/blob/blob_storage_registry.h"

#include "base/callback.h"
#include "storage/browser/blob/blob_entry.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace storage {
namespace {

TEST(BlobStorageRegistry, UUIDRegistration) {
  const std::string kBlob1 = "Blob1";
  const std::string kType = "type1";
  const std::string kDisposition = "disp1";
  BlobStorageRegistry registry;

  EXPECT_FALSE(registry.DeleteEntry(kBlob1));
  EXPECT_EQ(0u, registry.blob_count());

  BlobEntry* entry = registry.CreateEntry(kBlob1, kType, kDisposition);
  ASSERT_NE(nullptr, entry);
  EXPECT_EQ(BlobStatus::PENDING_QUOTA, entry->status());
  EXPECT_EQ(kType, entry->content_type());
  EXPECT_EQ(kDisposition, entry->content_disposition());
  EXPECT_EQ(0u, entry->refcount());

  EXPECT_EQ(entry, registry.GetEntry(kBlob1));
  EXPECT_TRUE(registry.DeleteEntry(kBlob1));
  entry = registry.CreateEntry(kBlob1, kType, kDisposition);

  EXPECT_EQ(1u, registry.blob_count());
}

TEST(BlobStorageRegistry, URLRegistration) {
  const std::string kBlob = "Blob1";
  const std::string kType = "type1";
  const std::string kDisposition = "disp1";
  const std::string kBlob2 = "Blob2";
  const GURL kURL = GURL("blob://Blob1");
  const GURL kURL2 = GURL("blob://Blob2");
  BlobStorageRegistry registry;

  EXPECT_FALSE(registry.IsURLMapped(kURL));
  EXPECT_EQ(nullptr, registry.GetEntryFromURL(kURL, nullptr));
  EXPECT_FALSE(registry.DeleteURLMapping(kURL, nullptr));
  EXPECT_FALSE(registry.CreateUrlMapping(kURL, kBlob));
  EXPECT_EQ(0u, registry.url_count());
  BlobEntry* entry = registry.CreateEntry(kBlob, kType, kDisposition);

  EXPECT_FALSE(registry.IsURLMapped(kURL));
  EXPECT_TRUE(registry.CreateUrlMapping(kURL, kBlob));
  EXPECT_FALSE(registry.CreateUrlMapping(kURL, kBlob2));

  EXPECT_TRUE(registry.IsURLMapped(kURL));
  EXPECT_EQ(entry, registry.GetEntryFromURL(kURL, nullptr));
  std::string uuid;
  EXPECT_EQ(entry, registry.GetEntryFromURL(kURL, &uuid));
  EXPECT_EQ(kBlob, uuid);
  EXPECT_EQ(1u, registry.url_count());

  registry.CreateEntry(kBlob2, kType, kDisposition);
  EXPECT_TRUE(registry.CreateUrlMapping(kURL2, kBlob2));
  EXPECT_EQ(2u, registry.url_count());
  EXPECT_TRUE(registry.DeleteURLMapping(kURL2, &uuid));
  EXPECT_EQ(kBlob2, uuid);
  EXPECT_FALSE(registry.IsURLMapped(kURL2));

  // Both urls point to the same blob.
  EXPECT_TRUE(registry.CreateUrlMapping(kURL2, kBlob));
  std::string uuid2;
  EXPECT_EQ(registry.GetEntryFromURL(kURL, &uuid),
            registry.GetEntryFromURL(kURL2, &uuid2));
  EXPECT_EQ(uuid, uuid2);
}

}  // namespace
}  // namespace storage
