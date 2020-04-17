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

#include "dawn_native/d3d12/PlatformFunctions.h"

#include "common/DynamicLib.h"

namespace dawn_native { namespace d3d12 {

    PlatformFunctions::PlatformFunctions() {
    }
    PlatformFunctions::~PlatformFunctions() {
    }

    MaybeError PlatformFunctions::LoadFunctions() {
        DAWN_TRY(LoadD3D12());
        DAWN_TRY(LoadDXGI());
        DAWN_TRY(LoadD3DCompiler());
        LoadPIXRuntime();
        return {};
    }

    MaybeError PlatformFunctions::LoadD3D12() {
        std::string error;
        if (!mD3D12Lib.Open("d3d12.dll", &error) ||
            !mD3D12Lib.GetProc(&d3d12CreateDevice, "D3D12CreateDevice", &error) ||
            !mD3D12Lib.GetProc(&d3d12GetDebugInterface, "D3D12GetDebugInterface", &error) ||
            !mD3D12Lib.GetProc(&d3d12SerializeRootSignature, "D3D12SerializeRootSignature",
                               &error) ||
            !mD3D12Lib.GetProc(&d3d12CreateRootSignatureDeserializer,
                               "D3D12CreateRootSignatureDeserializer", &error) ||
            !mD3D12Lib.GetProc(&d3d12SerializeVersionedRootSignature,
                               "D3D12SerializeVersionedRootSignature", &error) ||
            !mD3D12Lib.GetProc(&d3d12CreateVersionedRootSignatureDeserializer,
                               "D3D12CreateVersionedRootSignatureDeserializer", &error)) {
            return DAWN_CONTEXT_LOST_ERROR(error.c_str());
        }

        return {};
    }

    MaybeError PlatformFunctions::LoadDXGI() {
        std::string error;
        if (!mDXGILib.Open("dxgi.dll", &error) ||
            !mDXGILib.GetProc(&dxgiGetDebugInterface1, "DXGIGetDebugInterface1", &error) ||
            !mDXGILib.GetProc(&createDxgiFactory2, "CreateDXGIFactory2", &error)) {
            return DAWN_CONTEXT_LOST_ERROR(error.c_str());
        }

        return {};
    }

    MaybeError PlatformFunctions::LoadD3DCompiler() {
        std::string error;
        if (!mD3DCompilerLib.Open("d3dcompiler_47.dll", &error) ||
            !mD3DCompilerLib.GetProc(&d3dCompile, "D3DCompile", &error)) {
            return DAWN_CONTEXT_LOST_ERROR(error.c_str());
        }

        return {};
    }

    bool PlatformFunctions::isPIXEventRuntimeLoaded() const {
        return mPIXEventRuntimeLib.Valid();
    }

    void PlatformFunctions::LoadPIXRuntime() {
        if (!mPIXEventRuntimeLib.Open("WinPixEventRuntime.dll") ||
            !mPIXEventRuntimeLib.GetProc(&pixBeginEventOnCommandList,
                                         "PIXBeginEventOnCommandList") ||
            !mPIXEventRuntimeLib.GetProc(&pixEndEventOnCommandList, "PIXEndEventOnCommandList") ||
            !mPIXEventRuntimeLib.GetProc(&pixSetMarkerOnCommandList, "PIXSetMarkerOnCommandList")) {
            mPIXEventRuntimeLib.Close();
        }
    }

}}  // namespace dawn_native::d3d12
