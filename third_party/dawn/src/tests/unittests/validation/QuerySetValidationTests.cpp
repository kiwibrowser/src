// Copyright 2020 The Dawn Authors
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

class QuerySetValidationTest : public ValidationTest {
  protected:
    void SetUp() override {
        ValidationTest::SetUp();

        // Initialize the device with required extensions
        deviceWithPipelineStatistics =
            CreateDeviceFromAdapter(adapter, {"pipeline_statistics_query"});
        deviceWithTimestamp = CreateDeviceFromAdapter(adapter, {"timestamp_query"});
    }

    wgpu::QuerySet CreateQuerySet(
        wgpu::Device cDevice,
        wgpu::QueryType queryType,
        uint32_t queryCount,
        std::vector<wgpu::PipelineStatisticName> pipelineStatistics = {}) {
        wgpu::QuerySetDescriptor descriptor;
        descriptor.type = queryType;
        descriptor.count = queryCount;

        if (pipelineStatistics.size() > 0) {
            descriptor.pipelineStatistics = pipelineStatistics.data();
            descriptor.pipelineStatisticsCount = pipelineStatistics.size();
        }

        return cDevice.CreateQuerySet(&descriptor);
    }

    wgpu::Device deviceWithPipelineStatistics;
    wgpu::Device deviceWithTimestamp;
};

// Test creating query set with/without extensions
TEST_F(QuerySetValidationTest, Creation) {
    // Create query set for Occlusion query
    {
        // Success on default device without any extension enabled
        // Occlusion query does not require any extension.
        CreateQuerySet(device, wgpu::QueryType::Occlusion, 1);

        // Success on the device with extension enabled.
        CreateQuerySet(deviceWithPipelineStatistics, wgpu::QueryType::Occlusion, 1);
        CreateQuerySet(deviceWithTimestamp, wgpu::QueryType::Occlusion, 1);
    }

    // Create query set for PipelineStatistics query
    {
        // Fail on default device without any extension enabled
        ASSERT_DEVICE_ERROR(CreateQuerySet(device, wgpu::QueryType::PipelineStatistics, 1,
                                           {wgpu::PipelineStatisticName::VertexShaderInvocations}));

        // Success on the device if the extension is enabled.
        CreateQuerySet(deviceWithPipelineStatistics, wgpu::QueryType::PipelineStatistics, 1,
                       {wgpu::PipelineStatisticName::VertexShaderInvocations});
    }

    // Create query set for Timestamp query
    {
        // Fail on default device without any extension enabled
        ASSERT_DEVICE_ERROR(CreateQuerySet(device, wgpu::QueryType::Timestamp, 1));

        // Success on the device if the extension is enabled.
        CreateQuerySet(deviceWithTimestamp, wgpu::QueryType::Timestamp, 1);
    }
}

// Test creating query set with invalid type
TEST_F(QuerySetValidationTest, InvalidQueryType) {
    ASSERT_DEVICE_ERROR(CreateQuerySet(device, static_cast<wgpu::QueryType>(0xFFFFFFFF), 1));
}

// Test creating query set with unnecessary pipeline statistics
TEST_F(QuerySetValidationTest, UnnecessaryPipelineStatistics) {
    // Fail to create with pipeline statistics for Occlusion query
    {
        ASSERT_DEVICE_ERROR(CreateQuerySet(device, wgpu::QueryType::Occlusion, 1,
                                           {wgpu::PipelineStatisticName::VertexShaderInvocations}));
    }

    // Fail to create with pipeline statistics for Timestamp query
    {
        ASSERT_DEVICE_ERROR(CreateQuerySet(deviceWithTimestamp, wgpu::QueryType::Timestamp, 1,
                                           {wgpu::PipelineStatisticName::VertexShaderInvocations}));
    }
}

// Test creating query set with invalid pipeline statistics
TEST_F(QuerySetValidationTest, InvalidPipelineStatistics) {
    // Success to create with all pipeline statistics names which are not in the same order as
    // defined in webgpu header file
    {
        CreateQuerySet(deviceWithPipelineStatistics, wgpu::QueryType::PipelineStatistics, 1,
                       {wgpu::PipelineStatisticName::ClipperInvocations,
                        wgpu::PipelineStatisticName::ClipperPrimitivesOut,
                        wgpu::PipelineStatisticName::ComputeShaderInvocations,
                        wgpu::PipelineStatisticName::FragmentShaderInvocations,
                        wgpu::PipelineStatisticName::VertexShaderInvocations});
    }

    // Fail to create with empty pipeline statistics
    {
        ASSERT_DEVICE_ERROR(CreateQuerySet(deviceWithPipelineStatistics,
                                           wgpu::QueryType::PipelineStatistics, 1, {}));
    }

    // Fail to create with invalid pipeline statistics
    {
        ASSERT_DEVICE_ERROR(CreateQuerySet(deviceWithPipelineStatistics,
                                           wgpu::QueryType::PipelineStatistics, 1,
                                           {static_cast<wgpu::PipelineStatisticName>(0xFFFFFFFF)}));
    }

    // Fail to create with duplicate pipeline statistics
    {
        ASSERT_DEVICE_ERROR(CreateQuerySet(deviceWithPipelineStatistics,
                                           wgpu::QueryType::PipelineStatistics, 1,
                                           {wgpu::PipelineStatisticName::VertexShaderInvocations,
                                            wgpu::PipelineStatisticName::VertexShaderInvocations}));
    }
}

// Test destroying a destroyed query set
TEST_F(QuerySetValidationTest, DestroyDestroyedQuerySet) {
    wgpu::QuerySetDescriptor descriptor;
    descriptor.type = wgpu::QueryType::Occlusion;
    descriptor.count = 1;
    wgpu::QuerySet querySet = device.CreateQuerySet(&descriptor);
    querySet.Destroy();
    querySet.Destroy();
}

class TimestampQueryValidationTest : public QuerySetValidationTest {};

// Test write timestamp on command encoder
TEST_F(TimestampQueryValidationTest, WriteTimestampOnCommandEncoder) {
    wgpu::QuerySet timestampQuerySet =
        CreateQuerySet(deviceWithTimestamp, wgpu::QueryType::Timestamp, 2);
    wgpu::QuerySet occlusionQuerySet =
        CreateQuerySet(deviceWithTimestamp, wgpu::QueryType::Occlusion, 2);

    // Success on command encoder
    {
        wgpu::CommandEncoder encoder = deviceWithTimestamp.CreateCommandEncoder();
        encoder.WriteTimestamp(timestampQuerySet, 0);
        encoder.Finish();
    }

    // Not allow to write timestamp from another device
    {
        // Write timestamp from default device
        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        encoder.WriteTimestamp(timestampQuerySet, 0);
        ASSERT_DEVICE_ERROR(encoder.Finish());
    }

    // Not allow to write timestamp to the query set with other query type
    {
        wgpu::CommandEncoder encoder = deviceWithTimestamp.CreateCommandEncoder();
        encoder.WriteTimestamp(occlusionQuerySet, 0);
        ASSERT_DEVICE_ERROR(encoder.Finish());
    }

    // Fail to write timestamp to the index which exceeds the number of queries in query set
    {
        wgpu::CommandEncoder encoder = deviceWithTimestamp.CreateCommandEncoder();
        encoder.WriteTimestamp(timestampQuerySet, 2);
        ASSERT_DEVICE_ERROR(encoder.Finish());
    }

    // Fail to submit timestamp query with a destroyed query set
    {
        wgpu::CommandEncoder encoder = deviceWithTimestamp.CreateCommandEncoder();
        encoder.WriteTimestamp(timestampQuerySet, 0);
        wgpu::CommandBuffer commands = encoder.Finish();

        wgpu::Queue queue = deviceWithTimestamp.GetDefaultQueue();
        timestampQuerySet.Destroy();
        ASSERT_DEVICE_ERROR(queue.Submit(1, &commands));
    }
}

// Test write timestamp on compute pass encoder
TEST_F(TimestampQueryValidationTest, WriteTimestampOnComputePassEncoder) {
    wgpu::QuerySet timestampQuerySet =
        CreateQuerySet(deviceWithTimestamp, wgpu::QueryType::Timestamp, 2);
    wgpu::QuerySet occlusionQuerySet =
        CreateQuerySet(deviceWithTimestamp, wgpu::QueryType::Occlusion, 2);

    // Success on compute pass encoder
    {
        wgpu::CommandEncoder encoder = deviceWithTimestamp.CreateCommandEncoder();
        wgpu::ComputePassEncoder pass = encoder.BeginComputePass();
        pass.WriteTimestamp(timestampQuerySet, 0);
        pass.EndPass();
        encoder.Finish();
    }

    // Not allow to write timestamp from another device
    {
        // Write timestamp from default device
        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        wgpu::ComputePassEncoder pass = encoder.BeginComputePass();
        pass.WriteTimestamp(timestampQuerySet, 0);
        pass.EndPass();
        ASSERT_DEVICE_ERROR(encoder.Finish());
    }

    // Not allow to write timestamp to the query set with other query type
    {
        wgpu::CommandEncoder encoder = deviceWithTimestamp.CreateCommandEncoder();
        wgpu::ComputePassEncoder pass = encoder.BeginComputePass();
        pass.WriteTimestamp(occlusionQuerySet, 0);
        pass.EndPass();
        ASSERT_DEVICE_ERROR(encoder.Finish());
    }

    // Fail to write timestamp to the index which exceeds the number of queries in query set
    {
        wgpu::CommandEncoder encoder = deviceWithTimestamp.CreateCommandEncoder();
        wgpu::ComputePassEncoder pass = encoder.BeginComputePass();
        pass.WriteTimestamp(timestampQuerySet, 2);
        pass.EndPass();
        ASSERT_DEVICE_ERROR(encoder.Finish());
    }

    // Fail to submit timestamp query with a destroyed query set
    {
        wgpu::CommandEncoder encoder = deviceWithTimestamp.CreateCommandEncoder();
        wgpu::ComputePassEncoder pass = encoder.BeginComputePass();
        pass.WriteTimestamp(timestampQuerySet, 0);
        pass.EndPass();
        wgpu::CommandBuffer commands = encoder.Finish();

        wgpu::Queue queue = deviceWithTimestamp.GetDefaultQueue();
        timestampQuerySet.Destroy();
        ASSERT_DEVICE_ERROR(queue.Submit(1, &commands));
    }
}

// Test write timestamp on render pass encoder
TEST_F(TimestampQueryValidationTest, WriteTimestampOnRenderPassEncoder) {
    DummyRenderPass renderPass(deviceWithTimestamp);

    wgpu::QuerySet timestampQuerySet =
        CreateQuerySet(deviceWithTimestamp, wgpu::QueryType::Timestamp, 2);
    wgpu::QuerySet occlusionQuerySet =
        CreateQuerySet(deviceWithTimestamp, wgpu::QueryType::Occlusion, 2);

    // Success on render pass encoder
    {
        wgpu::CommandEncoder encoder = deviceWithTimestamp.CreateCommandEncoder();
        wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass);
        pass.WriteTimestamp(timestampQuerySet, 0);
        pass.EndPass();
        encoder.Finish();
    }

    // Not allow to write timestamp from another device
    {
        // Write timestamp from default device
        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass);
        pass.WriteTimestamp(timestampQuerySet, 0);
        pass.EndPass();
        ASSERT_DEVICE_ERROR(encoder.Finish());
    }

    // Not allow to write timestamp to the query set with other query type
    {
        wgpu::CommandEncoder encoder = deviceWithTimestamp.CreateCommandEncoder();
        wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass);
        pass.WriteTimestamp(occlusionQuerySet, 0);
        pass.EndPass();
        ASSERT_DEVICE_ERROR(encoder.Finish());
    }

    // Fail to write timestamp to the index which exceeds the number of queries in query set
    {
        wgpu::CommandEncoder encoder = deviceWithTimestamp.CreateCommandEncoder();
        wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass);
        pass.WriteTimestamp(timestampQuerySet, 2);
        pass.EndPass();
        ASSERT_DEVICE_ERROR(encoder.Finish());
    }

    // Fail to submit timestamp query with a destroyed query set
    {
        wgpu::CommandEncoder encoder = deviceWithTimestamp.CreateCommandEncoder();
        wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass);
        pass.WriteTimestamp(timestampQuerySet, 0);
        pass.EndPass();
        wgpu::CommandBuffer commands = encoder.Finish();

        wgpu::Queue queue = deviceWithTimestamp.GetDefaultQueue();
        timestampQuerySet.Destroy();
        ASSERT_DEVICE_ERROR(queue.Submit(1, &commands));
    }
}

class ResolveQuerySetValidationTest : public QuerySetValidationTest {
  protected:
    wgpu::Buffer CreateBuffer(wgpu::Device cDevice, uint64_t size, wgpu::BufferUsage usage) {
        wgpu::BufferDescriptor descriptor;
        descriptor.size = size;
        descriptor.usage = usage;

        return cDevice.CreateBuffer(&descriptor);
    }
};

// Test resolve query set with invalid query set, first query and query count
TEST_F(ResolveQuerySetValidationTest, ResolveInvalidQuerySetAndIndexCount) {
    constexpr uint32_t kQueryCount = 4;

    wgpu::QuerySet querySet = CreateQuerySet(device, wgpu::QueryType::Occlusion, kQueryCount);
    wgpu::Buffer destination =
        CreateBuffer(device, kQueryCount * sizeof(uint64_t), wgpu::BufferUsage::QueryResolve);

    // Success
    {
        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        encoder.ResolveQuerySet(querySet, 0, kQueryCount, destination, 0);
        wgpu::CommandBuffer commands = encoder.Finish();

        wgpu::Queue queue = device.GetDefaultQueue();
        queue.Submit(1, &commands);
    }

    // Fail to resolve query set from another device
    {
        wgpu::CommandEncoder encoder = deviceWithTimestamp.CreateCommandEncoder();
        encoder.ResolveQuerySet(querySet, 0, kQueryCount, destination, 0);
        ASSERT_DEVICE_ERROR(encoder.Finish());
    }

    //  Fail to resolve query set if first query out of range
    {
        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        encoder.ResolveQuerySet(querySet, kQueryCount, 0, destination, 0);
        ASSERT_DEVICE_ERROR(encoder.Finish());
    }

    //  Fail to resolve query set if the sum of first query and query count is larger than queries
    //  number in the query set
    {
        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        encoder.ResolveQuerySet(querySet, 1, kQueryCount, destination, 0);
        ASSERT_DEVICE_ERROR(encoder.Finish());
    }

    // Fail to resolve a destroyed query set
    {
        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        encoder.ResolveQuerySet(querySet, 0, kQueryCount, destination, 0);
        wgpu::CommandBuffer commands = encoder.Finish();

        wgpu::Queue queue = device.GetDefaultQueue();
        querySet.Destroy();
        ASSERT_DEVICE_ERROR(queue.Submit(1, &commands));
    }
}

// Test resolve query set with invalid query set, first query and query count
TEST_F(ResolveQuerySetValidationTest, ResolveToInvalidBufferAndOffset) {
    constexpr uint32_t kQueryCount = 4;
    constexpr uint64_t kBufferSize = kQueryCount * sizeof(uint64_t);

    wgpu::QuerySet querySet = CreateQuerySet(device, wgpu::QueryType::Occlusion, kQueryCount);
    wgpu::Buffer destination = CreateBuffer(device, kBufferSize, wgpu::BufferUsage::QueryResolve);

    // Success
    {
        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        encoder.ResolveQuerySet(querySet, 1, kQueryCount - 1, destination, 8);
        wgpu::CommandBuffer commands = encoder.Finish();

        wgpu::Queue queue = device.GetDefaultQueue();
        queue.Submit(1, &commands);
    }

    // Fail to resolve query set to a buffer created from another device
    {
        wgpu::Buffer bufferOnTimestamp =
            CreateBuffer(deviceWithTimestamp, kBufferSize, wgpu::BufferUsage::QueryResolve);
        wgpu::CommandEncoder encoder = deviceWithTimestamp.CreateCommandEncoder();
        encoder.ResolveQuerySet(querySet, 0, kQueryCount, bufferOnTimestamp, 0);
        ASSERT_DEVICE_ERROR(encoder.Finish());
    }

    //  Fail to resolve query set to a buffer if offset is not a multiple of 8 bytes
    {
        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        encoder.ResolveQuerySet(querySet, 0, kQueryCount, destination, 4);
        ASSERT_DEVICE_ERROR(encoder.Finish());
    }

    //  Fail to resolve query set to a buffer if the data size overflow the buffer
    {
        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        encoder.ResolveQuerySet(querySet, 0, kQueryCount, destination, 8);
        ASSERT_DEVICE_ERROR(encoder.Finish());
    }

    //  Fail to resolve query set to a buffer if the offset is past the end of the buffer
    {
        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        encoder.ResolveQuerySet(querySet, 0, 1, destination, kBufferSize);
        ASSERT_DEVICE_ERROR(encoder.Finish());
    }

    //  Fail to resolve query set to a buffer does not have the usage of QueryResolve
    {
        wgpu::Buffer dstBuffer = CreateBuffer(device, kBufferSize, wgpu::BufferUsage::CopyDst);
        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        encoder.ResolveQuerySet(querySet, 0, kQueryCount, dstBuffer, 0);
        ASSERT_DEVICE_ERROR(encoder.Finish());
    }

    // Fail to resolve query set to a destroyed buffer.
    {
        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        encoder.ResolveQuerySet(querySet, 0, kQueryCount, destination, 0);
        wgpu::CommandBuffer commands = encoder.Finish();

        wgpu::Queue queue = device.GetDefaultQueue();
        destination.Destroy();
        ASSERT_DEVICE_ERROR(queue.Submit(1, &commands));
    }
}

// Check that in 32bit mode the computation of queryCount * sizeof(uint64_t) doesn't overflow (which
// would skip validation).
TEST_F(ResolveQuerySetValidationTest, BufferOverflowOn32Bits) {
    // If compiling for 32-bits mode, the data size calculated by queryCount * sizeof(uint64_t)
    // is 8, which is less than the buffer size.
    constexpr uint32_t kQueryCount = std::numeric_limits<uint32_t>::max() / sizeof(uint64_t) + 2;

    wgpu::QuerySet querySet = CreateQuerySet(device, wgpu::QueryType::Occlusion, kQueryCount);
    wgpu::Buffer destination = CreateBuffer(device, 1024, wgpu::BufferUsage::QueryResolve);
    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    encoder.ResolveQuerySet(querySet, 0, kQueryCount, destination, 0);

    ASSERT_DEVICE_ERROR(encoder.Finish());
}
