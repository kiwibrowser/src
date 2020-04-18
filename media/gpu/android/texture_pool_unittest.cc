// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/gpu/android/texture_pool.h"

#include <memory>

#include "base/memory/weak_ptr.h"
#include "base/run_loop.h"
#include "base/test/scoped_task_environment.h"
#include "base/threading/thread_task_runner_handle.h"
#include "gpu/command_buffer/common/command_buffer_id.h"
#include "gpu/command_buffer/common/constants.h"
#include "gpu/command_buffer/service/sequence_id.h"
#include "gpu/ipc/common/gpu_messages.h"
#include "media/gpu/android/texture_wrapper.h"
#include "media/gpu/fake_command_buffer_helper.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace media {

using testing::_;
using testing::NiceMock;
using testing::Return;

// SupportsWeakPtr so it's easy to tell when it has been destroyed.
class MockTextureWrapper : public NiceMock<TextureWrapper>,
                           public base::SupportsWeakPtr<MockTextureWrapper> {
 public:
  MockTextureWrapper() {}
  ~MockTextureWrapper() override {}

  MOCK_METHOD0(ForceContextLost, void());
};

class TexturePoolTest : public testing::Test {
 public:
  void SetUp() override {
    task_runner_ = base::ThreadTaskRunnerHandle::Get();
    helper_ = base::MakeRefCounted<FakeCommandBufferHelper>(task_runner_);
    texture_pool_ = new TexturePool(helper_);
    // Random sync token that HasData().
    sync_token_ = gpu::SyncToken(gpu::CommandBufferNamespace::GPU_IO,
                                 gpu::CommandBufferId::FromUnsafeValue(1), 1);
    ASSERT_TRUE(sync_token_.HasData());
  }

  ~TexturePoolTest() override {
    helper_->StubLost();
    base::RunLoop().RunUntilIdle();
  }

  using WeakTexture = base::WeakPtr<MockTextureWrapper>;

  WeakTexture CreateAndAddTexture() {
    std::unique_ptr<MockTextureWrapper> texture =
        std::make_unique<MockTextureWrapper>();
    WeakTexture texture_weak = texture->AsWeakPtr();

    texture_pool_->AddTexture(std::move(texture));

    return texture_weak;
  }

  base::test::ScopedTaskEnvironment scoped_task_environment_;

  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;

  gpu::SyncToken sync_token_;

  scoped_refptr<FakeCommandBufferHelper> helper_;
  scoped_refptr<TexturePool> texture_pool_;
};

TEST_F(TexturePoolTest, AddAndReleaseTexturesWithContext) {
  // Test that adding then deleting a texture destroys it.
  WeakTexture texture = CreateAndAddTexture();
  // The texture should not be notified that the context was lost.
  EXPECT_CALL(*texture.get(), ForceContextLost()).Times(0);
  texture_pool_->ReleaseTexture(texture.get(), sync_token_);

  // The texture should still exist until the sync token is cleared.
  ASSERT_TRUE(texture);

  // Once the sync token is released, then the context should be made current
  // and the texture should be destroyed.
  helper_->ReleaseSyncToken(sync_token_);
  base::RunLoop().RunUntilIdle();
  ASSERT_FALSE(texture);
}

TEST_F(TexturePoolTest, AddAndReleaseTexturesWithoutContext) {
  // Test that adding then deleting a texture destroys it, and marks that the
  // context is lost, if the context can't be made current.
  WeakTexture texture = CreateAndAddTexture();
  helper_->ContextLost();
  EXPECT_CALL(*texture, ForceContextLost()).Times(1);
  texture_pool_->ReleaseTexture(texture.get(), sync_token_);
  ASSERT_TRUE(texture);

  helper_->ReleaseSyncToken(sync_token_);
  base::RunLoop().RunUntilIdle();
  ASSERT_FALSE(texture);
}

TEST_F(TexturePoolTest, TexturesAreReleasedOnStubDestructionWithContext) {
  // Add multiple textures, and test that they're all destroyed when the stub
  // says that it's destroyed.
  std::vector<TextureWrapper*> raw_textures;
  std::vector<WeakTexture> textures;

  for (int i = 0; i < 3; i++) {
    textures.push_back(CreateAndAddTexture());
    raw_textures.push_back(textures.back().get());
    // The context should not be lost.
    EXPECT_CALL(*textures.back(), ForceContextLost()).Times(0);
  }

  helper_->StubLost();

  // TextureWrappers should be destroyed.
  for (auto& texture : textures)
    ASSERT_FALSE(texture);

  // It should be okay to release the textures after they're destroyed, and
  // nothing should crash.
  for (auto* raw_texture : raw_textures)
    texture_pool_->ReleaseTexture(raw_texture, sync_token_);
}

TEST_F(TexturePoolTest, TexturesAreReleasedOnStubDestructionWithoutContext) {
  std::vector<TextureWrapper*> raw_textures;
  std::vector<WeakTexture> textures;

  for (int i = 0; i < 3; i++) {
    textures.push_back(CreateAndAddTexture());
    raw_textures.push_back(textures.back().get());
    EXPECT_CALL(*textures.back(), ForceContextLost()).Times(1);
  }

  helper_->ContextLost();
  helper_->StubLost();

  for (auto& texture : textures)
    ASSERT_FALSE(texture);

  // It should be okay to release the textures after they're destroyed, and
  // nothing should crash.
  for (auto* raw_texture : raw_textures)
    texture_pool_->ReleaseTexture(raw_texture, sync_token_);
}

TEST_F(TexturePoolTest, NonEmptyPoolAfterStubDestructionDoesntCrash) {
  // Make sure that we can delete the stub, and verify that pool teardown still
  // works (doesn't crash) even though the pool is not empty.
  CreateAndAddTexture();

  helper_->StubLost();
}

TEST_F(TexturePoolTest,
       NonEmptyPoolAfterStubWithoutContextDestructionDoesntCrash) {
  // Make sure that we can delete the stub, and verify that pool teardown still
  // works (doesn't crash) even though the pool is not empty.
  CreateAndAddTexture();

  helper_->ContextLost();
  helper_->StubLost();
}

TEST_F(TexturePoolTest, TexturePoolRetainsReferenceWhileWaiting) {
  // Dropping our reference to |texture_pool_| while it's waiting for a sync
  // token shouldn't prevent the wait from completing.
  WeakTexture texture = CreateAndAddTexture();
  texture_pool_->ReleaseTexture(texture.get(), sync_token_);

  // The texture should still exist until the sync token is cleared.
  ASSERT_TRUE(texture);

  // Drop the texture pool while it's waiting.  Nothing should happen.
  texture_pool_ = nullptr;
  ASSERT_TRUE(texture);

  // The texture should be destroyed after the sync token completes.
  helper_->ReleaseSyncToken(sync_token_);
  base::RunLoop().RunUntilIdle();
  ASSERT_FALSE(texture);
}

TEST_F(TexturePoolTest, TexturePoolReleasesImmediatelyWithoutSyncToken) {
  // If we don't provide a sync token, then it should release the texture.
  WeakTexture texture = CreateAndAddTexture();
  texture_pool_->ReleaseTexture(texture.get(), gpu::SyncToken());
  base::RunLoop().RunUntilIdle();
  ASSERT_FALSE(texture);
}

}  // namespace media
