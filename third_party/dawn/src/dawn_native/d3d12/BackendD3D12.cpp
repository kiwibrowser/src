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

#include "dawn_native/d3d12/BackendD3D12.h"

#include "dawn_native/D3D12Backend.h"
#include "dawn_native/Instance.h"
#include "dawn_native/d3d12/AdapterD3D12.h"
#include "dawn_native/d3d12/PlatformFunctions.h"

namespace dawn_native { namespace d3d12 {

    namespace {

        ResultOrError<ComPtr<IDXGIFactory4>> CreateFactory(const PlatformFunctions* functions,
                                                           bool enableBackendValidation) {
            ComPtr<IDXGIFactory4> factory;

            uint32_t dxgiFactoryFlags = 0;

            // Enable the debug layer (requires the Graphics Tools "optional feature").
            {
                if (enableBackendValidation) {
                    ComPtr<ID3D12Debug> debugController;
                    if (SUCCEEDED(
                            functions->d3d12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
                        ASSERT(debugController != nullptr);
                        debugController->EnableDebugLayer();

                        // Enable additional debug layers.
                        dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
                    }

                    ComPtr<IDXGIDebug1> dxgiDebug;
                    if (SUCCEEDED(functions->dxgiGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug)))) {
                        ASSERT(dxgiDebug != nullptr);
                        dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL,
                                                     DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_ALL));
                    }
                }
            }

            if (FAILED(functions->createDxgiFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory)))) {
                return DAWN_CONTEXT_LOST_ERROR("Failed to create a DXGI factory");
            }

            ASSERT(factory != nullptr);
            return factory;
        }

    }  // anonymous namespace

    Backend::Backend(InstanceBase* instance) : BackendConnection(instance, BackendType::D3D12) {
    }

    MaybeError Backend::Initialize() {
        mFunctions = std::make_unique<PlatformFunctions>();
        DAWN_TRY(mFunctions->LoadFunctions());

        DAWN_TRY_ASSIGN(
            mFactory, CreateFactory(mFunctions.get(), GetInstance()->IsBackendValidationEnabled()));

        return {};
    }

    ComPtr<IDXGIFactory4> Backend::GetFactory() const {
        return mFactory;
    }

    const PlatformFunctions* Backend::GetFunctions() const {
        return mFunctions.get();
    }

    std::vector<std::unique_ptr<AdapterBase>> Backend::DiscoverDefaultAdapters() {
        std::vector<std::unique_ptr<AdapterBase>> adapters;

        for (uint32_t adapterIndex = 0;; ++adapterIndex) {
            ComPtr<IDXGIAdapter1> dxgiAdapter = nullptr;
            if (mFactory->EnumAdapters1(adapterIndex, &dxgiAdapter) == DXGI_ERROR_NOT_FOUND) {
                break;  // No more adapters to enumerate.
            }

            ASSERT(dxgiAdapter != nullptr);

            std::unique_ptr<Adapter> adapter = std::make_unique<Adapter>(this, dxgiAdapter);
            if (GetInstance()->ConsumedError(adapter->Initialize())) {
                continue;
            }

            adapters.push_back(std::move(adapter));
        }

        return adapters;
    }

    BackendConnection* Connect(InstanceBase* instance) {
        Backend* backend = new Backend(instance);

        if (instance->ConsumedError(backend->Initialize())) {
            delete backend;
            return nullptr;
        }

        return backend;
    }

}}  // namespace dawn_native::d3d12
