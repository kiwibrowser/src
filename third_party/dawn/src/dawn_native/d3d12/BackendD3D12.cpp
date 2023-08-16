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
#include "dawn_native/d3d12/D3D12Error.h"
#include "dawn_native/d3d12/PlatformFunctions.h"

namespace dawn_native { namespace d3d12 {

    namespace {

        ResultOrError<ComPtr<IDXGIFactory4>> CreateFactory(const PlatformFunctions* functions,
                                                           bool enableBackendValidation,
                                                           bool beginCaptureOnStartup) {
            ComPtr<IDXGIFactory4> factory;

            uint32_t dxgiFactoryFlags = 0;

            // Enable the debug layer (requires the Graphics Tools "optional feature").
            {
                if (enableBackendValidation) {
                    ComPtr<ID3D12Debug1> debugController;
                    if (SUCCEEDED(
                            functions->d3d12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
                        ASSERT(debugController != nullptr);
                        debugController->EnableDebugLayer();
                        debugController->SetEnableGPUBasedValidation(true);

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

                if (beginCaptureOnStartup) {
                    ComPtr<IDXGraphicsAnalysis> graphicsAnalysis;
                    if (SUCCEEDED(functions->dxgiGetDebugInterface1(
                            0, IID_PPV_ARGS(&graphicsAnalysis)))) {
                        graphicsAnalysis->BeginCapture();
                    }
                }
            }

            if (FAILED(functions->createDxgiFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory)))) {
                return DAWN_INTERNAL_ERROR("Failed to create a DXGI factory");
            }

            ASSERT(factory != nullptr);
            return std::move(factory);
        }

        ResultOrError<std::unique_ptr<AdapterBase>> CreateAdapterFromIDXGIAdapter(
            Backend* backend,
            ComPtr<IDXGIAdapter> dxgiAdapter) {
            ComPtr<IDXGIAdapter3> dxgiAdapter3;
            DAWN_TRY(CheckHRESULT(dxgiAdapter.As(&dxgiAdapter3), "DXGIAdapter retrieval"));
            std::unique_ptr<Adapter> adapter =
                std::make_unique<Adapter>(backend, std::move(dxgiAdapter3));
            DAWN_TRY(adapter->Initialize());

            return {std::move(adapter)};
        }

    }  // anonymous namespace

    Backend::Backend(InstanceBase* instance)
        : BackendConnection(instance, wgpu::BackendType::D3D12) {
    }

    MaybeError Backend::Initialize() {
        mFunctions = std::make_unique<PlatformFunctions>();
        DAWN_TRY(mFunctions->LoadFunctions());

        const auto instance = GetInstance();

        DAWN_TRY_ASSIGN(mFactory,
                        CreateFactory(mFunctions.get(), instance->IsBackendValidationEnabled(),
                                      instance->IsBeginCaptureOnStartupEnabled()));

        return {};
    }

    ComPtr<IDXGIFactory4> Backend::GetFactory() const {
        return mFactory;
    }

    ResultOrError<IDxcLibrary*> Backend::GetOrCreateDxcLibrary() {
        if (mDxcLibrary == nullptr) {
            DAWN_TRY(CheckHRESULT(
                mFunctions->dxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&mDxcLibrary)),
                "DXC create library"));
            ASSERT(mDxcLibrary != nullptr);
        }
        return mDxcLibrary.Get();
    }

    ResultOrError<IDxcCompiler*> Backend::GetOrCreateDxcCompiler() {
        if (mDxcCompiler == nullptr) {
            DAWN_TRY(CheckHRESULT(
                mFunctions->dxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&mDxcCompiler)),
                "DXC create compiler"));
            ASSERT(mDxcCompiler != nullptr);
        }
        return mDxcCompiler.Get();
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
            ResultOrError<std::unique_ptr<AdapterBase>> adapter =
                CreateAdapterFromIDXGIAdapter(this, dxgiAdapter);
            if (adapter.IsError()) {
                adapter.AcquireError();
                continue;
            }

            adapters.push_back(std::move(adapter.AcquireSuccess()));
        }

        return adapters;
    }

    ResultOrError<std::vector<std::unique_ptr<AdapterBase>>> Backend::DiscoverAdapters(
        const AdapterDiscoveryOptionsBase* optionsBase) {
        ASSERT(optionsBase->backendType == WGPUBackendType_D3D12);
        const AdapterDiscoveryOptions* options =
            static_cast<const AdapterDiscoveryOptions*>(optionsBase);

        ASSERT(options->dxgiAdapter != nullptr);

        std::unique_ptr<AdapterBase> adapter;
        DAWN_TRY_ASSIGN(adapter, CreateAdapterFromIDXGIAdapter(this, options->dxgiAdapter));
        std::vector<std::unique_ptr<AdapterBase>> adapters;
        adapters.push_back(std::move(adapter));
        return std::move(adapters);
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
