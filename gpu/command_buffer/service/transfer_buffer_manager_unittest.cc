// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/command_buffer/service/transfer_buffer_manager.h"

#include <stddef.h>

#include <memory>

#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using base::SharedMemory;

namespace gpu {

const static size_t kBufferSize = 1024;

class TransferBufferManagerTest : public testing::Test {
 protected:
  void SetUp() override {
    transfer_buffer_manager_ = std::make_unique<TransferBufferManager>(nullptr);
  }

  std::unique_ptr<TransferBufferManager> transfer_buffer_manager_;
};

TEST_F(TransferBufferManagerTest, ZeroHandleMapsToNull) {
  EXPECT_TRUE(NULL == transfer_buffer_manager_->GetTransferBuffer(0).get());
}

TEST_F(TransferBufferManagerTest, NegativeHandleMapsToNull) {
  EXPECT_TRUE(NULL == transfer_buffer_manager_->GetTransferBuffer(-1).get());
}

TEST_F(TransferBufferManagerTest, OutOfRangeHandleMapsToNull) {
  EXPECT_TRUE(NULL == transfer_buffer_manager_->GetTransferBuffer(1).get());
}

TEST_F(TransferBufferManagerTest, CanRegisterTransferBuffer) {
  std::unique_ptr<base::SharedMemory> shm(new base::SharedMemory());
  shm->CreateAndMapAnonymous(kBufferSize);
  base::SharedMemory* shm_raw_pointer = shm.get();
  std::unique_ptr<SharedMemoryBufferBacking> backing(
      new SharedMemoryBufferBacking(std::move(shm), kBufferSize));
  SharedMemoryBufferBacking* backing_raw_ptr = backing.get();

  EXPECT_TRUE(
      transfer_buffer_manager_->RegisterTransferBuffer(1, std::move(backing)));
  scoped_refptr<Buffer> registered =
      transfer_buffer_manager_->GetTransferBuffer(1);

  // Shared-memory ownership is transfered. It should be the same memory.
  EXPECT_EQ(backing_raw_ptr, registered->backing());
  EXPECT_EQ(shm_raw_pointer, backing_raw_ptr->shared_memory());
}

class FakeBufferBacking : public BufferBacking {
 public:
  void* GetMemory() const override {
    return reinterpret_cast<void*>(0xBADF00D0);
  }
  size_t GetSize() const override { return 42; }
  static std::unique_ptr<BufferBacking> Make() {
    return std::unique_ptr<BufferBacking>(new FakeBufferBacking);
  }
};

TEST_F(TransferBufferManagerTest, CanDestroyTransferBuffer) {
  EXPECT_TRUE(transfer_buffer_manager_->RegisterTransferBuffer(
      1, std::unique_ptr<BufferBacking>(new FakeBufferBacking)));
  transfer_buffer_manager_->DestroyTransferBuffer(1);
  scoped_refptr<Buffer> registered =
      transfer_buffer_manager_->GetTransferBuffer(1);

  scoped_refptr<Buffer> null_buffer;
  EXPECT_EQ(null_buffer, registered);
}

TEST_F(TransferBufferManagerTest, CannotRegregisterTransferBufferId) {
  EXPECT_TRUE(transfer_buffer_manager_->RegisterTransferBuffer(
      1, FakeBufferBacking::Make()));
  EXPECT_FALSE(transfer_buffer_manager_->RegisterTransferBuffer(
      1, FakeBufferBacking::Make()));
  EXPECT_FALSE(transfer_buffer_manager_->RegisterTransferBuffer(
      1, FakeBufferBacking::Make()));
}

TEST_F(TransferBufferManagerTest, CanReuseTransferBufferIdAfterDestroying) {
  EXPECT_TRUE(transfer_buffer_manager_->RegisterTransferBuffer(
      1, FakeBufferBacking::Make()));
  transfer_buffer_manager_->DestroyTransferBuffer(1);
  EXPECT_TRUE(transfer_buffer_manager_->RegisterTransferBuffer(
      1, FakeBufferBacking::Make()));
}

TEST_F(TransferBufferManagerTest, DestroyUnusedTransferBufferIdDoesNotCrash) {
  transfer_buffer_manager_->DestroyTransferBuffer(1);
}

TEST_F(TransferBufferManagerTest, CannotRegisterNullTransferBuffer) {
  EXPECT_FALSE(transfer_buffer_manager_->RegisterTransferBuffer(
      0, FakeBufferBacking::Make()));
}

TEST_F(TransferBufferManagerTest, CannotRegisterNegativeTransferBufferId) {
  std::unique_ptr<base::SharedMemory> shm(new base::SharedMemory());
  shm->CreateAndMapAnonymous(kBufferSize);
  EXPECT_FALSE(transfer_buffer_manager_->RegisterTransferBuffer(
      -1, FakeBufferBacking::Make()));
}

}  // namespace gpu
