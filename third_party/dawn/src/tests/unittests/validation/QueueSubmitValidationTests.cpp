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

#include "utils/DawnHelpers.h"

namespace {

class QueueSubmitValidationTest : public ValidationTest {
};

static void StoreTrueMapWriteCallback(DawnBufferMapAsyncStatus status,
                                      void*,
                                      uint64_t,
                                      void* userdata) {
    *static_cast<bool*>(userdata) = true;
}

// Test submitting with a mapped buffer is disallowed
TEST_F(QueueSubmitValidationTest, SubmitWithMappedBuffer) {
    // Create a map-write buffer.
    dawn::BufferDescriptor descriptor;
    descriptor.usage = dawn::BufferUsageBit::MapWrite | dawn::BufferUsageBit::TransferSrc;
    descriptor.size = 4;
    dawn::Buffer buffer = device.CreateBuffer(&descriptor);

    // Create a fake copy destination buffer
    descriptor.usage = dawn::BufferUsageBit::TransferDst;
    dawn::Buffer targetBuffer = device.CreateBuffer(&descriptor);

    // Create a command buffer that reads from the mappable buffer.
    dawn::CommandBuffer commands;
    {
        dawn::CommandEncoder encoder = device.CreateCommandEncoder();
        encoder.CopyBufferToBuffer(buffer, 0, targetBuffer, 0, 4);
        commands = encoder.Finish();
    }

    dawn::Queue queue = device.CreateQueue();

    // Submitting when the buffer has never been mapped should succeed
    queue.Submit(1, &commands);

    // Map the buffer, submitting when the buffer is mapped should fail
    bool mapWriteFinished = false;
    buffer.MapWriteAsync(StoreTrueMapWriteCallback, &mapWriteFinished);
    queue.Submit(0, nullptr);
    ASSERT_TRUE(mapWriteFinished);

    ASSERT_DEVICE_ERROR(queue.Submit(1, &commands));

    // Unmap the buffer, queue submit should succeed
    buffer.Unmap();
    queue.Submit(1, &commands);
}

}  // anonymous namespace
