// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file defines tests that implementations of GpuMemoryBufferFactory should
// pass in order to be conformant.

#ifndef GPU_IPC_COMMON_GPU_MEMORY_BUFFER_IMPL_TEST_TEMPLATE_H_
#define GPU_IPC_COMMON_GPU_MEMORY_BUFFER_IMPL_TEST_TEMPLATE_H_

#include <stddef.h>
#include <string.h>

#include <memory>

#include "base/bind.h"
#include "build/build_config.h"
#include "gpu/ipc/common/gpu_memory_buffer_support.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/buffer_format_util.h"

#if defined(OS_WIN)
#include "ui/gl/init/gl_factory.h"
#include "ui/gl/test/gl_surface_test_support.h"
#endif

namespace gpu {

template <typename GpuMemoryBufferImplType>
class GpuMemoryBufferImplTest : public testing::Test {
 public:
  GpuMemoryBufferImpl::DestructionCallback CreateGpuMemoryBuffer(
      const gfx::Size& size,
      gfx::BufferFormat format,
      gfx::BufferUsage usage,
      gfx::GpuMemoryBufferHandle* handle,
      bool* destroyed) {
    return base::Bind(&GpuMemoryBufferImplTest::FreeGpuMemoryBuffer,
                      base::Unretained(this),
                      GpuMemoryBufferImplType::AllocateForTesting(
                          size, format, usage, handle),
                      base::Unretained(destroyed));
  }

  GpuMemoryBufferSupport* gpu_memory_buffer_support() {
    return &gpu_memory_buffer_support_;
  }

#if defined(OS_WIN)
  // Overridden from testing::Test:
  void SetUp() override { gl::GLSurfaceTestSupport::InitializeOneOff(); }
  void TearDown() override { gl::init::ShutdownGL(false); }
#endif

 private:
  GpuMemoryBufferSupport gpu_memory_buffer_support_;

  void FreeGpuMemoryBuffer(const base::Closure& free_callback,
                           bool* destroyed,
                           const gpu::SyncToken& sync_token) {
    free_callback.Run();
    if (destroyed)
      *destroyed = true;
  }
};

// Subclass test case for tests that require a Create() method,
// not all implementations have that.
template <typename GpuMemoryBufferImplType>
class GpuMemoryBufferImplCreateTest : public testing::Test {
 public:
  GpuMemoryBufferSupport* gpu_memory_buffer_support() {
    return &gpu_memory_buffer_support_;
  }

 private:
  GpuMemoryBufferSupport gpu_memory_buffer_support_;
};

TYPED_TEST_CASE_P(GpuMemoryBufferImplTest);

TYPED_TEST_P(GpuMemoryBufferImplTest, CreateFromHandle) {
  const gfx::Size kBufferSize(8, 8);

  for (auto format : gfx::GetBufferFormatsForTesting()) {
    gfx::BufferUsage usages[] = {
        gfx::BufferUsage::GPU_READ,
        gfx::BufferUsage::SCANOUT,
        gfx::BufferUsage::SCANOUT_CAMERA_READ_WRITE,
        gfx::BufferUsage::CAMERA_AND_CPU_READ_WRITE,
        gfx::BufferUsage::SCANOUT_CPU_READ_WRITE,
        gfx::BufferUsage::SCANOUT_VDA_WRITE,
        gfx::BufferUsage::GPU_READ_CPU_READ_WRITE,
        gfx::BufferUsage::GPU_READ_CPU_READ_WRITE_PERSISTENT};
    for (auto usage : usages) {
      if (!TestFixture::gpu_memory_buffer_support()->IsConfigurationSupported(
              TypeParam::kBufferType, format, usage))
        continue;

      bool destroyed = false;
      gfx::GpuMemoryBufferHandle handle;
      GpuMemoryBufferImpl::DestructionCallback destroy_callback =
          TestFixture::CreateGpuMemoryBuffer(kBufferSize, format, usage,
                                             &handle, &destroyed);
      std::unique_ptr<GpuMemoryBufferImpl> buffer(
          TestFixture::gpu_memory_buffer_support()
              ->CreateGpuMemoryBufferImplFromHandle(handle, kBufferSize, format,
                                                    usage, destroy_callback));
      ASSERT_TRUE(buffer);
      EXPECT_EQ(buffer->GetFormat(), format);

      // Check if destruction callback is executed when deleting the buffer.
      buffer.reset();
      ASSERT_TRUE(destroyed);
    }
  }
}

TYPED_TEST_P(GpuMemoryBufferImplTest, Map) {
  // Use a multiple of 4 for both dimensions to support compressed formats.
  const gfx::Size kBufferSize(4, 4);

  for (auto format : gfx::GetBufferFormatsForTesting()) {
    if (!TestFixture::gpu_memory_buffer_support()->IsConfigurationSupported(
            TypeParam::kBufferType, format,
            gfx::BufferUsage::GPU_READ_CPU_READ_WRITE)) {
      continue;
    }

    gfx::GpuMemoryBufferHandle handle;
    GpuMemoryBufferImpl::DestructionCallback destroy_callback =
        TestFixture::CreateGpuMemoryBuffer(
            kBufferSize, format, gfx::BufferUsage::GPU_READ_CPU_READ_WRITE,
            &handle, nullptr);
    std::unique_ptr<GpuMemoryBufferImpl> buffer(
        TestFixture::gpu_memory_buffer_support()
            ->CreateGpuMemoryBufferImplFromHandle(
                handle, kBufferSize, format,
                gfx::BufferUsage::GPU_READ_CPU_READ_WRITE, destroy_callback));
    ASSERT_TRUE(buffer);

    const size_t num_planes = gfx::NumberOfPlanesForBufferFormat(format);

    // Map buffer into user space.
    ASSERT_TRUE(buffer->Map());

    // Copy and compare mapped buffers.
    for (size_t plane = 0; plane < num_planes; ++plane) {
      const size_t row_size_in_bytes =
          gfx::RowSizeForBufferFormat(kBufferSize.width(), format, plane);
      EXPECT_GT(row_size_in_bytes, 0u);

      std::unique_ptr<char[]> data(new char[row_size_in_bytes]);
      memset(data.get(), 0x2a + plane, row_size_in_bytes);

      size_t height = kBufferSize.height() /
                      gfx::SubsamplingFactorForBufferFormat(format, plane);
      for (size_t y = 0; y < height; ++y) {
        memcpy(static_cast<char*>(buffer->memory(plane)) +
                   y * buffer->stride(plane),
               data.get(), row_size_in_bytes);
        EXPECT_EQ(0, memcmp(static_cast<char*>(buffer->memory(plane)) +
                                y * buffer->stride(plane),
                            data.get(), row_size_in_bytes));
      }
    }

    buffer->Unmap();
  }
}

TYPED_TEST_P(GpuMemoryBufferImplTest, PersistentMap) {
  // Use a multiple of 4 for both dimensions to support compressed formats.
  const gfx::Size kBufferSize(4, 4);

  for (auto format : gfx::GetBufferFormatsForTesting()) {
    if (!TestFixture::gpu_memory_buffer_support()->IsConfigurationSupported(
            TypeParam::kBufferType, format,
            gfx::BufferUsage::GPU_READ_CPU_READ_WRITE_PERSISTENT)) {
      continue;
    }

    gfx::GpuMemoryBufferHandle handle;
    GpuMemoryBufferImpl::DestructionCallback destroy_callback =
        TestFixture::CreateGpuMemoryBuffer(
            kBufferSize, format,
            gfx::BufferUsage::GPU_READ_CPU_READ_WRITE_PERSISTENT, &handle,
            nullptr);
    std::unique_ptr<GpuMemoryBufferImpl> buffer(
        TestFixture::gpu_memory_buffer_support()
            ->CreateGpuMemoryBufferImplFromHandle(
                handle, kBufferSize, format,
                gfx::BufferUsage::GPU_READ_CPU_READ_WRITE_PERSISTENT,
                destroy_callback));
    ASSERT_TRUE(buffer);

    // Map buffer into user space.
    ASSERT_TRUE(buffer->Map());

    // Copy and compare mapped buffers.
    size_t num_planes = gfx::NumberOfPlanesForBufferFormat(format);
    for (size_t plane = 0; plane < num_planes; ++plane) {
      const size_t row_size_in_bytes =
          gfx::RowSizeForBufferFormat(kBufferSize.width(), format, plane);
      EXPECT_GT(row_size_in_bytes, 0u);

      std::unique_ptr<char[]> data(new char[row_size_in_bytes]);
      memset(data.get(), 0x2a + plane, row_size_in_bytes);

      size_t height = kBufferSize.height() /
                      gfx::SubsamplingFactorForBufferFormat(format, plane);
      for (size_t y = 0; y < height; ++y) {
        memcpy(static_cast<char*>(buffer->memory(plane)) +
                   y * buffer->stride(plane),
               data.get(), row_size_in_bytes);
        EXPECT_EQ(0, memcmp(static_cast<char*>(buffer->memory(plane)) +
                                y * buffer->stride(plane),
                            data.get(), row_size_in_bytes));
      }
    }

    buffer->Unmap();

    // Remap the buffer, and compare again. It should contain the same data.
    ASSERT_TRUE(buffer->Map());

    for (size_t plane = 0; plane < num_planes; ++plane) {
      const size_t row_size_in_bytes =
          gfx::RowSizeForBufferFormat(kBufferSize.width(), format, plane);

      std::unique_ptr<char[]> data(new char[row_size_in_bytes]);
      memset(data.get(), 0x2a + plane, row_size_in_bytes);

      size_t height = kBufferSize.height() /
                      gfx::SubsamplingFactorForBufferFormat(format, plane);
      for (size_t y = 0; y < height; ++y) {
        EXPECT_EQ(0, memcmp(static_cast<char*>(buffer->memory(plane)) +
                                y * buffer->stride(plane),
                            data.get(), row_size_in_bytes));
      }
    }

    buffer->Unmap();
  }
}

// The GpuMemoryBufferImplTest test case verifies behavior that is expected
// from a GpuMemoryBuffer implementation in order to be conformant.
REGISTER_TYPED_TEST_CASE_P(GpuMemoryBufferImplTest,
                           CreateFromHandle,
                           Map,
                           PersistentMap);

TYPED_TEST_CASE_P(GpuMemoryBufferImplCreateTest);

TYPED_TEST_P(GpuMemoryBufferImplCreateTest, Create) {
  const gfx::GpuMemoryBufferId kBufferId(1);
  const gfx::Size kBufferSize(8, 8);
  gfx::BufferUsage usage = gfx::BufferUsage::GPU_READ;

  for (auto format : gfx::GetBufferFormatsForTesting()) {
    if (!TestFixture::gpu_memory_buffer_support()->IsConfigurationSupported(
            TypeParam::kBufferType, format, usage))
      continue;
    bool destroyed = false;
    gfx::GpuMemoryBufferHandle handle;
    std::unique_ptr<TypeParam> buffer(TypeParam::Create(
        kBufferId, kBufferSize, format, usage,
        base::Bind(
            [](bool* destroyed, const gpu::SyncToken&) { *destroyed = true; },
            base::Unretained(&destroyed))));
    ASSERT_TRUE(buffer);
    EXPECT_EQ(buffer->GetFormat(), format);

    // Check if destruction callback is executed when deleting the buffer.
    buffer.reset();
    ASSERT_TRUE(destroyed);
  }
}
// The GpuMemoryBufferImplCreateTest test case verifies behavior that is
// expected from a GpuMemoryBuffer Create() implementation in order to be
// conformant.
REGISTER_TYPED_TEST_CASE_P(GpuMemoryBufferImplCreateTest, Create);

}  // namespace gpu

#endif  // GPU_IPC_COMMON_GPU_MEMORY_BUFFER_IMPL_TEST_TEMPLATE_H_
