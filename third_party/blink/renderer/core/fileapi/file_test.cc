// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/fileapi/file.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/platform/file_metadata.h"

namespace blink {

TEST(FileTest, nativeFile) {
  File* const file = File::Create("/native/path");
  EXPECT_TRUE(file->HasBackingFile());
  EXPECT_EQ("/native/path", file->GetPath());
  EXPECT_TRUE(file->FileSystemURL().IsEmpty());
}

TEST(FileTest, blobBackingFile) {
  const scoped_refptr<BlobDataHandle> blob_data_handle =
      BlobDataHandle::Create();
  File* const file = File::Create("name", 0.0, blob_data_handle);
  EXPECT_FALSE(file->HasBackingFile());
  EXPECT_TRUE(file->GetPath().IsEmpty());
  EXPECT_TRUE(file->FileSystemURL().IsEmpty());
}

TEST(FileTest, fileSystemFileWithNativeSnapshot) {
  FileMetadata metadata;
  metadata.platform_path = "/native/snapshot";
  File* const file =
      File::CreateForFileSystemFile("name", metadata, File::kIsUserVisible);
  EXPECT_TRUE(file->HasBackingFile());
  EXPECT_EQ("/native/snapshot", file->GetPath());
  EXPECT_TRUE(file->FileSystemURL().IsEmpty());
}

TEST(FileTest, fileSystemFileWithNativeSnapshotAndSize) {
  FileMetadata metadata;
  metadata.length = 1024ll;
  metadata.platform_path = "/native/snapshot";
  File* const file =
      File::CreateForFileSystemFile("name", metadata, File::kIsUserVisible);
  EXPECT_TRUE(file->HasBackingFile());
  EXPECT_EQ("/native/snapshot", file->GetPath());
  EXPECT_TRUE(file->FileSystemURL().IsEmpty());
}

TEST(FileTest, fileSystemFileWithoutNativeSnapshot) {
  KURL url("filesystem:http://example.com/isolated/hash/non-native-file");
  FileMetadata metadata;
  File* const file =
      File::CreateForFileSystemFile(url, metadata, File::kIsUserVisible);
  EXPECT_FALSE(file->HasBackingFile());
  EXPECT_TRUE(file->GetPath().IsEmpty());
  EXPECT_EQ(url, file->FileSystemURL());
}

TEST(FileTest, hsaSameSource) {
  File* const native_file_a1 = File::Create("/native/pathA");
  File* const native_file_a2 = File::Create("/native/pathA");
  File* const native_file_b = File::Create("/native/pathB");

  const scoped_refptr<BlobDataHandle> blob_data_a = BlobDataHandle::Create();
  const scoped_refptr<BlobDataHandle> blob_data_b = BlobDataHandle::Create();
  File* const blob_file_a1 = File::Create("name", 0.0, blob_data_a);
  File* const blob_file_a2 = File::Create("name", 0.0, blob_data_a);
  File* const blob_file_b = File::Create("name", 0.0, blob_data_b);

  KURL url_a("filesystem:http://example.com/isolated/hash/non-native-file-A");
  KURL url_b("filesystem:http://example.com/isolated/hash/non-native-file-B");
  FileMetadata metadata;
  File* const file_system_file_a1 =
      File::CreateForFileSystemFile(url_a, metadata, File::kIsUserVisible);
  File* const file_system_file_a2 =
      File::CreateForFileSystemFile(url_a, metadata, File::kIsUserVisible);
  File* const file_system_file_b =
      File::CreateForFileSystemFile(url_b, metadata, File::kIsUserVisible);

  EXPECT_FALSE(native_file_a1->HasSameSource(*blob_file_a1));
  EXPECT_FALSE(blob_file_a1->HasSameSource(*file_system_file_a1));
  EXPECT_FALSE(file_system_file_a1->HasSameSource(*native_file_a1));

  EXPECT_TRUE(native_file_a1->HasSameSource(*native_file_a1));
  EXPECT_TRUE(native_file_a1->HasSameSource(*native_file_a2));
  EXPECT_FALSE(native_file_a1->HasSameSource(*native_file_b));

  EXPECT_TRUE(blob_file_a1->HasSameSource(*blob_file_a1));
  EXPECT_TRUE(blob_file_a1->HasSameSource(*blob_file_a2));
  EXPECT_FALSE(blob_file_a1->HasSameSource(*blob_file_b));

  EXPECT_TRUE(file_system_file_a1->HasSameSource(*file_system_file_a1));
  EXPECT_TRUE(file_system_file_a1->HasSameSource(*file_system_file_a2));
  EXPECT_FALSE(file_system_file_a1->HasSameSource(*file_system_file_b));
}

}  // namespace blink
