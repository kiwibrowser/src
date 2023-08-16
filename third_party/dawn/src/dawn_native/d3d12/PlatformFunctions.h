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

#ifndef DAWNNATIVE_D3D12_PLATFORMFUNCTIONS_H_
#define DAWNNATIVE_D3D12_PLATFORMFUNCTIONS_H_

#include "dawn_native/d3d12/d3d12_platform.h"

#include "common/DynamicLib.h"
#include "dawn_native/Error.h"

#include <d3dcompiler.h>

class DynamicLib;

namespace dawn_native { namespace d3d12 {

    // Loads the functions required from the platform dynamically so that we don't need to rely on
    // them being present in the system. For example linking against d3d12.lib would prevent
    // dawn_native from loading on Windows 7 system where d3d12.dll doesn't exist.
    class PlatformFunctions {
      public:
        PlatformFunctions();
        ~PlatformFunctions();

        MaybeError LoadFunctions();
        bool IsPIXEventRuntimeLoaded() const;
        bool IsDXCAvailable() const;

        // Functions from d3d12.dll
        PFN_D3D12_CREATE_DEVICE d3d12CreateDevice = nullptr;
        PFN_D3D12_GET_DEBUG_INTERFACE d3d12GetDebugInterface = nullptr;

        PFN_D3D12_SERIALIZE_ROOT_SIGNATURE d3d12SerializeRootSignature = nullptr;
        PFN_D3D12_CREATE_ROOT_SIGNATURE_DESERIALIZER d3d12CreateRootSignatureDeserializer = nullptr;
        PFN_D3D12_SERIALIZE_VERSIONED_ROOT_SIGNATURE d3d12SerializeVersionedRootSignature = nullptr;
        PFN_D3D12_CREATE_VERSIONED_ROOT_SIGNATURE_DESERIALIZER
        d3d12CreateVersionedRootSignatureDeserializer = nullptr;

        // Functions from dxgi.dll
        using PFN_DXGI_GET_DEBUG_INTERFACE1 = HRESULT(WINAPI*)(UINT Flags,
                                                               REFIID riid,
                                                               _COM_Outptr_ void** pDebug);
        PFN_DXGI_GET_DEBUG_INTERFACE1 dxgiGetDebugInterface1 = nullptr;

        using PFN_CREATE_DXGI_FACTORY2 = HRESULT(WINAPI*)(UINT Flags,
                                                          REFIID riid,
                                                          _COM_Outptr_ void** ppFactory);
        PFN_CREATE_DXGI_FACTORY2 createDxgiFactory2 = nullptr;

        // Functions from dxcompiler.dll
        using PFN_DXC_CREATE_INSTANCE = HRESULT(WINAPI*)(REFCLSID rclsid,
                                                         REFIID riid,
                                                         _COM_Outptr_ void** ppCompiler);
        PFN_DXC_CREATE_INSTANCE dxcCreateInstance = nullptr;

        // Functions from d3d3compiler.dll
        pD3DCompile d3dCompile = nullptr;

        // Functions from WinPixEventRuntime.dll
        using PFN_PIX_END_EVENT_ON_COMMAND_LIST =
            HRESULT(WINAPI*)(ID3D12GraphicsCommandList* commandList);

        PFN_PIX_END_EVENT_ON_COMMAND_LIST pixEndEventOnCommandList = nullptr;

        using PFN_PIX_BEGIN_EVENT_ON_COMMAND_LIST = HRESULT(
            WINAPI*)(ID3D12GraphicsCommandList* commandList, UINT64 color, _In_ PCSTR formatString);

        PFN_PIX_BEGIN_EVENT_ON_COMMAND_LIST pixBeginEventOnCommandList = nullptr;

        using PFN_SET_MARKER_ON_COMMAND_LIST = HRESULT(
            WINAPI*)(ID3D12GraphicsCommandList* commandList, UINT64 color, _In_ PCSTR formatString);

        PFN_SET_MARKER_ON_COMMAND_LIST pixSetMarkerOnCommandList = nullptr;

        // Functions from D3D11.dll
        PFN_D3D11ON12_CREATE_DEVICE d3d11on12CreateDevice = nullptr;

      private:
        MaybeError LoadD3D12();
        MaybeError LoadD3D11();
        MaybeError LoadDXGI();
        void LoadDXIL();
        void LoadDXCompiler();
        MaybeError LoadFXCompiler();
        void LoadPIXRuntime();

        DynamicLib mD3D12Lib;
        DynamicLib mD3D11Lib;
        DynamicLib mDXGILib;
        DynamicLib mDXILLib;
        DynamicLib mDXCompilerLib;
        DynamicLib mFXCompilerLib;
        DynamicLib mPIXEventRuntimeLib;
    };

}}  // namespace dawn_native::d3d12

#endif  // DAWNNATIVE_VULKAN_VULKANFUNCTIONS_H_
