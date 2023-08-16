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

#include "DawnWireServerFuzzer.h"

#include "dawn_native/DawnNative.h"
#include "testing/libfuzzer/libfuzzer_exports.h"

extern "C" int LLVMFuzzerInitialize(int* argc, char*** argv) {
    return DawnWireServerFuzzer::Initialize(argc, argv);
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    return DawnWireServerFuzzer::Run(
        data, size,
        [](dawn_native::Instance* instance) {
            std::vector<dawn_native::Adapter> adapters = instance->GetAdapters();

            wgpu::Device device;
            for (dawn_native::Adapter adapter : adapters) {
                wgpu::AdapterProperties properties;
                adapter.GetProperties(&properties);

                if (properties.backendType == wgpu::BackendType::D3D12 &&
                    properties.adapterType == wgpu::AdapterType::CPU) {
                    device = wgpu::Device::Acquire(adapter.CreateDevice());
                    break;
                }
            }
            return device;
        },
        true /* supportsErrorInjection */);
}
