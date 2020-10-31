// Copyright 2019 The Dawn Authors
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

#include "dawn_wire/Wire.h"
#include "gtest/gtest.h"

#include <vector>

class WireWGPUDevicePropertiesTests : public testing::Test {};

// Test that the serialization and deserialization of WGPUDeviceProperties can work correctly.
TEST_F(WireWGPUDevicePropertiesTests, SerializeWGPUDeviceProperties) {
    WGPUDeviceProperties sentWGPUDeviceProperties;
    sentWGPUDeviceProperties.textureCompressionBC = true;
    // Set false to test that the serialization can handle both true and false correctly.
    sentWGPUDeviceProperties.pipelineStatisticsQuery = false;
    sentWGPUDeviceProperties.timestampQuery = true;

    size_t sentWGPUDevicePropertiesSize =
        dawn_wire::SerializedWGPUDevicePropertiesSize(&sentWGPUDeviceProperties);
    std::vector<char> buffer;
    buffer.resize(sentWGPUDevicePropertiesSize);
    dawn_wire::SerializeWGPUDeviceProperties(&sentWGPUDeviceProperties, buffer.data());

    WGPUDeviceProperties receivedWGPUDeviceProperties;
    dawn_wire::DeserializeWGPUDeviceProperties(&receivedWGPUDeviceProperties, buffer.data());
    ASSERT_TRUE(receivedWGPUDeviceProperties.textureCompressionBC);
    ASSERT_FALSE(receivedWGPUDeviceProperties.pipelineStatisticsQuery);
    ASSERT_TRUE(receivedWGPUDeviceProperties.timestampQuery);
}
