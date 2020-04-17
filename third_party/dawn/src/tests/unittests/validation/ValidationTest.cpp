// Copyright 2017 The Dawn Authors
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

#include "common/Assert.h"
#include "dawn/dawn.h"
#include "dawn_native/NullBackend.h"

ValidationTest::ValidationTest() {
    instance = std::make_unique<dawn_native::Instance>();
    instance->DiscoverDefaultAdapters();

    std::vector<dawn_native::Adapter> adapters = instance->GetAdapters();

    // Validation tests run against the null backend, find the corresponding adapter
    bool foundNullAdapter = false;
    for (auto &currentAdapter : adapters) {
        if (currentAdapter.GetBackendType() == dawn_native::BackendType::Null) {
            adapter = currentAdapter;
            foundNullAdapter = true;
            break;
        }
    }

    ASSERT(foundNullAdapter);
    device = dawn::Device::Acquire(adapter.CreateDevice());

    DawnProcTable procs = dawn_native::GetProcs();
    dawnSetProcs(&procs);

    device.SetErrorCallback(ValidationTest::OnDeviceError, this);
}

ValidationTest::~ValidationTest() {
    // We need to destroy Dawn objects before setting the procs to null otherwise the dawn*Release
    // will call a nullptr
    device = dawn::Device();
    dawnSetProcs(nullptr);
}

void ValidationTest::TearDown() {
    ASSERT_FALSE(mExpectError);
}

void ValidationTest::StartExpectDeviceError() {
    mExpectError = true;
    mError = false;
}
bool ValidationTest::EndExpectDeviceError() {
    mExpectError = false;
    return mError;
}
std::string ValidationTest::GetLastDeviceErrorMessage() const {
    return mDeviceErrorMessage;
}

// static
void ValidationTest::OnDeviceError(const char* message, void* userdata) {
    auto self = static_cast<ValidationTest*>(userdata);
    self->mDeviceErrorMessage = message;

    ASSERT_TRUE(self->mExpectError) << "Got unexpected device error: " << message;
    ASSERT_FALSE(self->mError) << "Got two errors in expect block";
    self->mError = true;
}

ValidationTest::DummyRenderPass::DummyRenderPass(const dawn::Device& device)
    : attachmentFormat(dawn::TextureFormat::R8G8B8A8Unorm), width(400), height(400) {

    dawn::TextureDescriptor descriptor;
    descriptor.dimension = dawn::TextureDimension::e2D;
    descriptor.size.width = width;
    descriptor.size.height = height;
    descriptor.size.depth = 1;
    descriptor.arrayLayerCount = 1;
    descriptor.sampleCount = 1;
    descriptor.format = attachmentFormat;
    descriptor.mipLevelCount = 1;
    descriptor.usage = dawn::TextureUsageBit::OutputAttachment;
    attachment = device.CreateTexture(&descriptor);

    dawn::TextureView view = attachment.CreateDefaultView();
    mColorAttachment.attachment = view;
    mColorAttachment.resolveTarget = nullptr;
    mColorAttachment.clearColor = { 0.0f, 0.0f, 0.0f, 0.0f };
    mColorAttachment.loadOp = dawn::LoadOp::Clear;
    mColorAttachment.storeOp = dawn::StoreOp::Store;
    mColorAttachments[0] = &mColorAttachment;

    colorAttachmentCount = 1;
    colorAttachments = mColorAttachments;
    depthStencilAttachment = nullptr;
}
