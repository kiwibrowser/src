// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/service/display/texture_deleter.h"

#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/viz/common/resources/single_release_callback.h"
#include "components/viz/test/test_context_provider.h"
#include "components/viz/test/test_web_graphics_context_3d.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace viz {
namespace {

TEST(TextureDeleterTest, Destroy) {
  auto deleter =
      std::make_unique<TextureDeleter>(base::ThreadTaskRunnerHandle::Get());

  scoped_refptr<TestContextProvider> context_provider =
      TestContextProvider::Create();
  context_provider->BindToCurrentThread();

  GLuint texture_id = 0u;
  context_provider->ContextGL()->GenTextures(1, &texture_id);

  EXPECT_TRUE(context_provider->HasOneRef());
  EXPECT_EQ(1u, context_provider->TestContext3d()->NumTextures());

  std::unique_ptr<SingleReleaseCallback> cb =
      deleter->GetReleaseCallback(context_provider, texture_id);
  EXPECT_FALSE(context_provider->HasOneRef());
  EXPECT_EQ(1u, context_provider->TestContext3d()->NumTextures());

  // When the deleter is destroyed, it immediately drops its ref on the
  // ContextProvider, and deletes the texture.
  deleter = nullptr;
  EXPECT_TRUE(context_provider->HasOneRef());
  EXPECT_EQ(0u, context_provider->TestContext3d()->NumTextures());

  // Run the scoped release callback before destroying it, but it won't do
  // anything.
  cb->Run(gpu::SyncToken(), false);
}

TEST(TextureDeleterTest, NullTaskRunner) {
  auto deleter = std::make_unique<TextureDeleter>(nullptr);

  scoped_refptr<TestContextProvider> context_provider =
      TestContextProvider::Create();
  context_provider->BindToCurrentThread();

  GLuint texture_id = 0u;
  context_provider->ContextGL()->GenTextures(1, &texture_id);

  EXPECT_TRUE(context_provider->HasOneRef());
  EXPECT_EQ(1u, context_provider->TestContext3d()->NumTextures());

  std::unique_ptr<SingleReleaseCallback> cb =
      deleter->GetReleaseCallback(context_provider, texture_id);
  EXPECT_FALSE(context_provider->HasOneRef());
  EXPECT_EQ(1u, context_provider->TestContext3d()->NumTextures());

  cb->Run(gpu::SyncToken(), false);

  // With no task runner the callback will immediately drops its ref on the
  // ContextProvider and delete the texture.
  EXPECT_TRUE(context_provider->HasOneRef());
  EXPECT_EQ(0u, context_provider->TestContext3d()->NumTextures());
}

}  // namespace
}  // namespace viz
