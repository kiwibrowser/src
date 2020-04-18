// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/service/display_embedder/server_shared_bitmap_manager.h"

#include "components/viz/common/resources/resource_sizes.h"
#include "mojo/public/cpp/system/platform_handle.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace viz {
namespace {

class ServerSharedBitmapManagerTest : public testing::Test {
 protected:
  void SetUp() override {
    manager_ = std::make_unique<ServerSharedBitmapManager>();
  }

  void TearDown() override { manager_.reset(); }

  ServerSharedBitmapManager* manager() const { return manager_.get(); }

 private:
  std::unique_ptr<ServerSharedBitmapManager> manager_;
};

TEST_F(ServerSharedBitmapManagerTest, TestCreate) {
  gfx::Size bitmap_size(1, 1);
  size_t size_in_bytes;
  EXPECT_TRUE(
      ResourceSizes::MaybeSizeInBytes(bitmap_size, RGBA_8888, &size_in_bytes));
  std::unique_ptr<base::SharedMemory> bitmap(new base::SharedMemory());
  bitmap->CreateAndMapAnonymous(size_in_bytes);
  memset(bitmap->memory(), 0xff, size_in_bytes);
  SharedBitmapId id = SharedBitmap::GenerateId();

  base::SharedMemoryHandle handle = bitmap->handle().Duplicate();
  mojo::ScopedSharedBufferHandle buffer_handle = mojo::WrapSharedMemoryHandle(
      handle, size_in_bytes,
      mojo::UnwrappedSharedMemoryHandleProtection::kReadWrite);
  manager()->ChildAllocatedSharedBitmap(std::move(buffer_handle), id);

  std::unique_ptr<SharedBitmap> large_bitmap;
  large_bitmap =
      manager()->GetSharedBitmapFromId(gfx::Size(1024, 1024), RGBA_8888, id);
  EXPECT_TRUE(large_bitmap.get() == nullptr);

  std::unique_ptr<SharedBitmap> very_large_bitmap;
  very_large_bitmap = manager()->GetSharedBitmapFromId(
      gfx::Size(1, (1 << 30) | 1), RGBA_8888, id);
  EXPECT_TRUE(very_large_bitmap.get() == nullptr);

  std::unique_ptr<SharedBitmap> negative_size_bitmap;
  negative_size_bitmap =
      manager()->GetSharedBitmapFromId(gfx::Size(-1, 1024), RGBA_8888, id);
  EXPECT_TRUE(negative_size_bitmap.get() == nullptr);

  SharedBitmapId id2 = SharedBitmap::GenerateId();
  std::unique_ptr<SharedBitmap> invalid_bitmap;
  invalid_bitmap =
      manager()->GetSharedBitmapFromId(bitmap_size, RGBA_8888, id2);
  EXPECT_TRUE(invalid_bitmap.get() == nullptr);

  std::unique_ptr<SharedBitmap> shared_bitmap;
  shared_bitmap = manager()->GetSharedBitmapFromId(bitmap_size, RGBA_8888, id);
  ASSERT_TRUE(shared_bitmap.get() != nullptr);
  EXPECT_EQ(memcmp(shared_bitmap->pixels(), bitmap->memory(), 4), 0);

  std::unique_ptr<SharedBitmap> large_bitmap2;
  large_bitmap2 =
      manager()->GetSharedBitmapFromId(gfx::Size(1024, 1024), RGBA_8888, id);
  EXPECT_TRUE(large_bitmap2.get() == nullptr);

  std::unique_ptr<SharedBitmap> shared_bitmap2;
  shared_bitmap2 = manager()->GetSharedBitmapFromId(bitmap_size, RGBA_8888, id);
  EXPECT_TRUE(shared_bitmap2->pixels() == shared_bitmap->pixels());
  shared_bitmap2.reset();
  EXPECT_EQ(memcmp(shared_bitmap->pixels(), bitmap->memory(), size_in_bytes),
            0);

  manager()->ChildDeletedSharedBitmap(id);

  memset(bitmap->memory(), 0, size_in_bytes);

  EXPECT_EQ(memcmp(shared_bitmap->pixels(), bitmap->memory(), size_in_bytes),
            0);
  bitmap.reset();
  shared_bitmap.reset();
}

TEST_F(ServerSharedBitmapManagerTest, AddDuplicate) {
  gfx::Size bitmap_size(1, 1);
  size_t size_in_bytes;
  EXPECT_TRUE(
      ResourceSizes::MaybeSizeInBytes(bitmap_size, RGBA_8888, &size_in_bytes));
  std::unique_ptr<base::SharedMemory> bitmap(new base::SharedMemory());
  bitmap->CreateAndMapAnonymous(size_in_bytes);
  memset(bitmap->memory(), 0xff, size_in_bytes);
  SharedBitmapId id = SharedBitmap::GenerateId();

  base::SharedMemoryHandle handle = bitmap->handle().Duplicate();
  mojo::ScopedSharedBufferHandle buffer_handle = mojo::WrapSharedMemoryHandle(
      handle, size_in_bytes,
      mojo::UnwrappedSharedMemoryHandleProtection::kReadWrite);
  manager()->ChildAllocatedSharedBitmap(std::move(buffer_handle), id);

  std::unique_ptr<base::SharedMemory> bitmap2(new base::SharedMemory());
  bitmap2->CreateAndMapAnonymous(size_in_bytes);
  memset(bitmap2->memory(), 0x00, size_in_bytes);

  base::SharedMemoryHandle handle2 = bitmap->handle().Duplicate();
  buffer_handle = mojo::WrapSharedMemoryHandle(
      handle2, size_in_bytes,
      mojo::UnwrappedSharedMemoryHandleProtection::kReadWrite);
  manager()->ChildAllocatedSharedBitmap(std::move(buffer_handle), id);

  std::unique_ptr<SharedBitmap> shared_bitmap;
  shared_bitmap = manager()->GetSharedBitmapFromId(bitmap_size, RGBA_8888, id);
  ASSERT_TRUE(shared_bitmap.get() != nullptr);
  EXPECT_EQ(memcmp(shared_bitmap->pixels(), bitmap->memory(), size_in_bytes),
            0);
  manager()->ChildDeletedSharedBitmap(id);
}

TEST_F(ServerSharedBitmapManagerTest, SharedMemoryHandle) {
  gfx::Size bitmap_size(1, 1);
  size_t size_in_bytes;
  EXPECT_TRUE(
      ResourceSizes::MaybeSizeInBytes(bitmap_size, RGBA_8888, &size_in_bytes));
  std::unique_ptr<base::SharedMemory> bitmap(new base::SharedMemory());
  bitmap->CreateAndMapAnonymous(size_in_bytes);
  memset(bitmap->memory(), 0xff, size_in_bytes);
  base::UnguessableToken shared_memory_guid = bitmap->handle().GetGUID();
  EXPECT_FALSE(shared_memory_guid.is_empty());

  SharedBitmapId id = SharedBitmap::GenerateId();
  base::SharedMemoryHandle handle = bitmap->handle().Duplicate();
  mojo::ScopedSharedBufferHandle buffer_handle = mojo::WrapSharedMemoryHandle(
      handle, size_in_bytes,
      mojo::UnwrappedSharedMemoryHandleProtection::kReadWrite);
  manager()->ChildAllocatedSharedBitmap(std::move(buffer_handle), id);

  std::unique_ptr<SharedBitmap> shared_bitmap;
  shared_bitmap =
      manager()->GetSharedBitmapFromId(gfx::Size(1, 1), RGBA_8888, id);
  EXPECT_EQ(shared_bitmap->GetCrossProcessGUID(), shared_memory_guid);

  manager()->ChildDeletedSharedBitmap(id);
}

}  // namespace
}  // namespace viz
