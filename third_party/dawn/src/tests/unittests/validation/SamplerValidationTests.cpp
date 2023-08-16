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

#include "tests/unittests/validation/ValidationTest.h"

#include "utils/WGPUHelpers.h"

#include <cmath>

namespace {

    class SamplerValidationTest : public ValidationTest {};

    // Test NaN and INFINITY values are not allowed
    TEST_F(SamplerValidationTest, InvalidLOD) {
        {
            wgpu::SamplerDescriptor samplerDesc = utils::GetDefaultSamplerDescriptor();
            device.CreateSampler(&samplerDesc);
        }
        {
            wgpu::SamplerDescriptor samplerDesc = utils::GetDefaultSamplerDescriptor();
            samplerDesc.lodMinClamp = NAN;
            ASSERT_DEVICE_ERROR(device.CreateSampler(&samplerDesc));
        }
        {
            wgpu::SamplerDescriptor samplerDesc = utils::GetDefaultSamplerDescriptor();
            samplerDesc.lodMaxClamp = NAN;
            ASSERT_DEVICE_ERROR(device.CreateSampler(&samplerDesc));
        }
        {
            wgpu::SamplerDescriptor samplerDesc = utils::GetDefaultSamplerDescriptor();
            samplerDesc.lodMaxClamp = INFINITY;
            device.CreateSampler(&samplerDesc);
        }
        {
            wgpu::SamplerDescriptor samplerDesc = utils::GetDefaultSamplerDescriptor();
            samplerDesc.lodMaxClamp = INFINITY;
            samplerDesc.lodMinClamp = INFINITY;
            device.CreateSampler(&samplerDesc);
        }
    }

}  // anonymous namespace
