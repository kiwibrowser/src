// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/resources/scoped_resource.h"

#include <stddef.h>

#include "cc/test/fake_resource_provider.h"
#include "cc/test/test_context_provider.h"
#include "cc/test/test_shared_bitmap_manager.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace cc {
namespace {

TEST(ScopedResourceTest, NewScopedResource) {
  scoped_refptr<TestContextProvider> context_provider =
      TestContextProvider::Create();
  ASSERT_TRUE(context_provider->BindToCurrentThread());

  std::unique_ptr<viz::SharedBitmapManager> shared_bitmap_manager(
      new TestSharedBitmapManager());
  std::unique_ptr<ResourceProvider> resource_provider =
      FakeResourceProvider::Create(context_provider.get(),
                                   shared_bitmap_manager.get());
  auto texture = std::make_unique<ScopedResource>(resource_provider.get());

  // New scoped textures do not hold a texture yet.
  EXPECT_EQ(0u, texture->id());

  // New scoped textures do not have a size yet.
  EXPECT_EQ(gfx::Size(), texture->size());
  EXPECT_EQ(0u, ResourceUtil::UncheckedSizeInBytes<size_t>(texture->size(),
                                                           texture->format()));
}

TEST(ScopedResourceTest, CreateScopedResource) {
  scoped_refptr<TestContextProvider> context_provider =
      TestContextProvider::Create();
  ASSERT_TRUE(context_provider->BindToCurrentThread());

  std::unique_ptr<viz::SharedBitmapManager> shared_bitmap_manager(
      new TestSharedBitmapManager());
  std::unique_ptr<ResourceProvider> resource_provider =
      FakeResourceProvider::Create(context_provider.get(),
                                   shared_bitmap_manager.get());
  auto texture = std::make_unique<ScopedResource>(resource_provider.get());
  texture->Allocate(gfx::Size(30, 30), ResourceProvider::TEXTURE_HINT_DEFAULT,
                    viz::RGBA_8888, gfx::ColorSpace());

  // The texture has an allocated byte-size now.
  size_t expected_bytes = 30 * 30 * 4;
  EXPECT_EQ(expected_bytes, ResourceUtil::UncheckedSizeInBytes<size_t>(
                                texture->size(), texture->format()));

  EXPECT_LT(0u, texture->id());
  EXPECT_EQ(static_cast<unsigned>(viz::RGBA_8888), texture->format());
  EXPECT_EQ(gfx::Size(30, 30), texture->size());
}

TEST(ScopedResourceTest, ScopedResourceIsDeleted) {
  scoped_refptr<TestContextProvider> context_provider =
      TestContextProvider::Create();
  ASSERT_TRUE(context_provider->BindToCurrentThread());

  std::unique_ptr<viz::SharedBitmapManager> shared_bitmap_manager(
      new TestSharedBitmapManager());
  std::unique_ptr<ResourceProvider> resource_provider =
      FakeResourceProvider::Create(context_provider.get(),
                                   shared_bitmap_manager.get());
  {
    auto texture = std::make_unique<ScopedResource>(resource_provider.get());

    EXPECT_EQ(0u, resource_provider->num_resources());
    texture->Allocate(gfx::Size(30, 30), ResourceProvider::TEXTURE_HINT_DEFAULT,
                      viz::RGBA_8888, gfx::ColorSpace());
    EXPECT_LT(0u, texture->id());
    EXPECT_EQ(1u, resource_provider->num_resources());
  }

  EXPECT_EQ(0u, resource_provider->num_resources());
  {
    auto texture = std::make_unique<ScopedResource>(resource_provider.get());
    EXPECT_EQ(0u, resource_provider->num_resources());
    texture->Allocate(gfx::Size(30, 30), ResourceProvider::TEXTURE_HINT_DEFAULT,
                      viz::RGBA_8888, gfx::ColorSpace());
    EXPECT_LT(0u, texture->id());
    EXPECT_EQ(1u, resource_provider->num_resources());
    texture->Free();
    EXPECT_EQ(0u, resource_provider->num_resources());
  }
}

}  // namespace
}  // namespace cc
