// Copyright 2018 The Dawn Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "tests/unittests/validation/ValidationTest.h"

#include "utils/WGPUHelpers.h"

namespace {

    class QueueSubmitValidationTest : public ValidationTest {};

    // Test submitting with a mapped buffer is disallowed
    TEST_F(QueueSubmitValidationTest, SubmitWithMappedBuffer) {
        // Create a map-write buffer.
        wgpu::BufferDescriptor descriptor;
        descriptor.usage = wgpu::BufferUsage::MapWrite | wgpu::BufferUsage::CopySrc;
        descriptor.size = 4;
        wgpu::Buffer buffer = device.CreateBuffer(&descriptor);

        // Create a fake copy destination buffer
        descriptor.usage = wgpu::BufferUsage::CopyDst;
        wgpu::Buffer targetBuffer = device.CreateBuffer(&descriptor);

        // Create a command buffer that reads from the mappable buffer.
        wgpu::CommandBuffer commands;
        {
            wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
            encoder.CopyBufferToBuffer(buffer, 0, targetBuffer, 0, 4);
            commands = encoder.Finish();
        }

        wgpu::Queue queue = device.GetDefaultQueue();

        // Submitting when the buffer has never been mapped should succeed
        queue.Submit(1, &commands);

        // Map the buffer, submitting when the buffer is mapped should fail
        buffer.MapWriteAsync(nullptr, nullptr);

        // Try submitting before the callback is fired.
        ASSERT_DEVICE_ERROR(queue.Submit(1, &commands));

        WaitForAllOperations(device);

        // Try submitting after the callback is fired.
        ASSERT_DEVICE_ERROR(queue.Submit(1, &commands));

        // Unmap the buffer, queue submit should succeed
        buffer.Unmap();
        queue.Submit(1, &commands);
    }

    class QueueWriteBufferValidationTest : public ValidationTest {
      private:
        void SetUp() override {
            ValidationTest::SetUp();
            queue = device.GetDefaultQueue();
        }

      protected:
        wgpu::Buffer CreateBuffer(uint64_t size) {
            wgpu::BufferDescriptor descriptor;
            descriptor.size = size;
            descriptor.usage = wgpu::BufferUsage::CopyDst;
            return device.CreateBuffer(&descriptor);
        }

        wgpu::Queue queue;
    };

    // Test the success case for WriteBuffer
    TEST_F(QueueWriteBufferValidationTest, Success) {
        wgpu::Buffer buf = CreateBuffer(4);

        uint32_t foo = 0x01020304;
        queue.WriteBuffer(buf, 0, &foo, sizeof(foo));
    }

    // Test error case for WriteBuffer out of bounds
    TEST_F(QueueWriteBufferValidationTest, OutOfBounds) {
        wgpu::Buffer buf = CreateBuffer(4);

        uint32_t foo[2] = {0, 0};
        ASSERT_DEVICE_ERROR(queue.WriteBuffer(buf, 0, foo, 8));
    }

    // Test error case for WriteBuffer out of bounds with an overflow
    TEST_F(QueueWriteBufferValidationTest, OutOfBoundsOverflow) {
        wgpu::Buffer buf = CreateBuffer(1024);

        uint32_t foo[2] = {0, 0};

        // An offset that when added to "4" would overflow to be zero and pass validation without
        // overflow checks.
        uint64_t offset = uint64_t(int64_t(0) - int64_t(4));

        ASSERT_DEVICE_ERROR(queue.WriteBuffer(buf, offset, foo, 4));
    }

    // Test error case for WriteBuffer with the wrong usage
    TEST_F(QueueWriteBufferValidationTest, WrongUsage) {
        wgpu::BufferDescriptor descriptor;
        descriptor.size = 4;
        descriptor.usage = wgpu::BufferUsage::Vertex;
        wgpu::Buffer buf = device.CreateBuffer(&descriptor);

        uint32_t foo = 0;
        ASSERT_DEVICE_ERROR(queue.WriteBuffer(buf, 0, &foo, sizeof(foo)));
    }

    // Test WriteBuffer with unaligned size
    TEST_F(QueueWriteBufferValidationTest, UnalignedSize) {
        wgpu::Buffer buf = CreateBuffer(4);

        uint16_t value = 123;
        ASSERT_DEVICE_ERROR(queue.WriteBuffer(buf, 0, &value, sizeof(value)));
    }

    // Test WriteBuffer with unaligned offset
    TEST_F(QueueWriteBufferValidationTest, UnalignedOffset) {
        wgpu::Buffer buf = CreateBuffer(8);

        uint32_t value = 0x01020304;
        ASSERT_DEVICE_ERROR(queue.WriteBuffer(buf, 2, &value, sizeof(value)));
    }

    // Test WriteBuffer with destroyed buffer
    TEST_F(QueueWriteBufferValidationTest, DestroyedBuffer) {
        wgpu::Buffer buf = CreateBuffer(4);
        buf.Destroy();

        uint32_t value = 0;
        ASSERT_DEVICE_ERROR(queue.WriteBuffer(buf, 0, &value, sizeof(value)));
    }

    // Test WriteBuffer with mapped buffer
    TEST_F(QueueWriteBufferValidationTest, MappedBuffer) {
        // CreateBufferMapped
        {
            wgpu::BufferDescriptor descriptor;
            descriptor.size = 4;
            descriptor.usage = wgpu::BufferUsage::CopyDst;
            wgpu::CreateBufferMappedResult result = device.CreateBufferMapped(&descriptor);

            uint32_t value = 0;
            ASSERT_DEVICE_ERROR(queue.WriteBuffer(result.buffer, 0, &value, sizeof(value)));
        }

        // mappedAtCreation
        {
            wgpu::BufferDescriptor descriptor;
            descriptor.size = 4;
            descriptor.usage = wgpu::BufferUsage::CopyDst;
            descriptor.mappedAtCreation = true;
            wgpu::Buffer buffer = device.CreateBuffer(&descriptor);

            uint32_t value = 0;
            ASSERT_DEVICE_ERROR(queue.WriteBuffer(buffer, 0, &value, sizeof(value)));
        }

        // MapReadAsync
        {
            wgpu::BufferDescriptor descriptor;
            descriptor.size = 4;
            descriptor.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::MapRead;
            wgpu::Buffer buf = device.CreateBuffer(&descriptor);

            buf.MapReadAsync(nullptr, nullptr);
            uint32_t value = 0;
            ASSERT_DEVICE_ERROR(queue.WriteBuffer(buf, 0, &value, sizeof(value)));
        }

        // MapWriteAsync
        {
            wgpu::BufferDescriptor descriptor;
            descriptor.size = 4;
            descriptor.usage = wgpu::BufferUsage::CopySrc | wgpu::BufferUsage::MapWrite;
            wgpu::Buffer buf = device.CreateBuffer(&descriptor);

            buf.MapWriteAsync(nullptr, nullptr);
            uint32_t value = 0;
            ASSERT_DEVICE_ERROR(queue.WriteBuffer(buf, 0, &value, sizeof(value)));
        }
    }

}  // anonymous namespace
