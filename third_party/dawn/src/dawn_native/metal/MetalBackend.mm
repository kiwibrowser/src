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

// MetalBackend.cpp: contains the definition of symbols exported by MetalBackend.h so that they
// can be compiled twice: once export (shared library), once not exported (static library)

#include "dawn_native/MetalBackend.h"

#include "dawn_native/Texture.h"
#include "dawn_native/metal/DeviceMTL.h"

namespace dawn_native { namespace metal {

    id<MTLDevice> GetMetalDevice(WGPUDevice cDevice) {
        Device* device = reinterpret_cast<Device*>(cDevice);
        return device->GetMTLDevice();
    }

    ExternalImageDescriptorIOSurface::ExternalImageDescriptorIOSurface()
        : ExternalImageDescriptor(ExternalImageDescriptorType::IOSurface) {
    }

    WGPUTexture WrapIOSurface(WGPUDevice cDevice,
                              const ExternalImageDescriptorIOSurface* cDescriptor) {
        Device* device = reinterpret_cast<Device*>(cDevice);
        TextureBase* texture = device->CreateTextureWrappingIOSurface(
            cDescriptor, cDescriptor->ioSurface, cDescriptor->plane);
        return reinterpret_cast<WGPUTexture>(texture);
    }

    void WaitForCommandsToBeScheduled(WGPUDevice cDevice) {
        Device* device = reinterpret_cast<Device*>(cDevice);
        device->WaitForCommandsToBeScheduled();
    }

}}  // namespace dawn_native::metal
