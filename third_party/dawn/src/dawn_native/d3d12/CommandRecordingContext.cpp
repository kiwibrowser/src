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
#include "dawn_native/d3d12/CommandRecordingContext.h"
#include "dawn_native/d3d12/CommandAllocatorManager.h"
#include "dawn_native/d3d12/D3D12Error.h"
#include "dawn_native/d3d12/DeviceD3D12.h"
#include "dawn_native/d3d12/HeapD3D12.h"
#include "dawn_native/d3d12/ResidencyManagerD3D12.h"

namespace dawn_native { namespace d3d12 {

    void CommandRecordingContext::AddToSharedTextureList(Texture* texture) {
        ASSERT(IsOpen());
        mSharedTextures.insert(texture);
    }

    MaybeError CommandRecordingContext::Open(ID3D12Device* d3d12Device,
                                             CommandAllocatorManager* commandAllocationManager) {
        ASSERT(!IsOpen());
        ID3D12CommandAllocator* commandAllocator;
        DAWN_TRY_ASSIGN(commandAllocator, commandAllocationManager->ReserveCommandAllocator());
        if (mD3d12CommandList != nullptr) {
            MaybeError error = CheckHRESULT(mD3d12CommandList->Reset(commandAllocator, nullptr),
                                            "D3D12 resetting command list");
            if (error.IsError()) {
                mD3d12CommandList.Reset();
                DAWN_TRY(std::move(error));
            }
        } else {
            ComPtr<ID3D12GraphicsCommandList> d3d12GraphicsCommandList;
            DAWN_TRY(CheckHRESULT(
                d3d12Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator,
                                               nullptr, IID_PPV_ARGS(&d3d12GraphicsCommandList)),
                "D3D12 creating direct command list"));
            mD3d12CommandList = std::move(d3d12GraphicsCommandList);
            // Store a cast to ID3D12GraphicsCommandList4. This is required to use the D3D12 render
            // pass APIs introduced in Windows build 1809.
            mD3d12CommandList.As(&mD3d12CommandList4);
        }

        mIsOpen = true;

        return {};
    }

    MaybeError CommandRecordingContext::ExecuteCommandList(Device* device) {
        if (IsOpen()) {
            // Shared textures must be transitioned to common state after the last usage in order
            // for them to be used by other APIs like D3D11. We ensure this by transitioning to the
            // common state right before command list submission. TransitionUsageNow itself ensures
            // no unnecessary transitions happen if the resources is already in the common state.
            for (Texture* texture : mSharedTextures) {
                texture->TrackAllUsageAndTransitionNow(this, D3D12_RESOURCE_STATE_COMMON);
            }

            MaybeError error =
                CheckHRESULT(mD3d12CommandList->Close(), "D3D12 closing pending command list");
            if (error.IsError()) {
                Release();
                DAWN_TRY(std::move(error));
            }
            DAWN_TRY(device->GetResidencyManager()->EnsureHeapsAreResident(
                mHeapsPendingUsage.data(), mHeapsPendingUsage.size()));

            ID3D12CommandList* d3d12CommandList = GetCommandList();
            device->GetCommandQueue()->ExecuteCommandLists(1, &d3d12CommandList);

            mIsOpen = false;
            mSharedTextures.clear();
            mHeapsPendingUsage.clear();
        }
        return {};
    }

    void CommandRecordingContext::TrackHeapUsage(Heap* heap, Serial serial) {
        // Before tracking the heap, check the last serial it was recorded on to ensure we aren't
        // tracking it more than once.
        if (heap->GetLastUsage() < serial) {
            heap->SetLastUsage(serial);
            mHeapsPendingUsage.push_back(heap);
        }
    }

    ID3D12GraphicsCommandList* CommandRecordingContext::GetCommandList() const {
        ASSERT(mD3d12CommandList != nullptr);
        ASSERT(IsOpen());
        return mD3d12CommandList.Get();
    }

    // This function will fail on Windows versions prior to 1809. Support must be queried through
    // the device before calling.
    ID3D12GraphicsCommandList4* CommandRecordingContext::GetCommandList4() const {
        ASSERT(IsOpen());
        ASSERT(mD3d12CommandList.Get() != nullptr);
        return mD3d12CommandList4.Get();
    }

    void CommandRecordingContext::Release() {
        mD3d12CommandList.Reset();
        mD3d12CommandList4.Reset();
        mIsOpen = false;
        mSharedTextures.clear();
        mHeapsPendingUsage.clear();
    }

    bool CommandRecordingContext::IsOpen() const {
        return mIsOpen;
    }

}}  // namespace dawn_native::d3d12
