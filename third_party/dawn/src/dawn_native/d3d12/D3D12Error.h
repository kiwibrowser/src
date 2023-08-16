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

#ifndef DAWNNATIVE_D3D12_D3D12ERROR_H_
#define DAWNNATIVE_D3D12_D3D12ERROR_H_

#include <d3d12.h>
#include "dawn_native/Error.h"
#include "dawn_native/ErrorInjector.h"

namespace dawn_native { namespace d3d12 {

    constexpr HRESULT E_FAKE_ERROR_FOR_TESTING = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0xFF);
    constexpr HRESULT E_FAKE_OUTOFMEMORY_ERROR_FOR_TESTING =
        MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0xFE);

    // Returns a success only if result of HResult is success
    MaybeError CheckHRESULTImpl(HRESULT result, const char* context);

    // Uses CheckRESULT but returns OOM specific error when recoverable.
    MaybeError CheckOutOfMemoryHRESULTImpl(HRESULT result, const char* context);

#define CheckHRESULT(resultIn, contextIn)   \
    ::dawn_native::d3d12::CheckHRESULTImpl( \
        INJECT_ERROR_OR_RUN(resultIn, E_FAKE_ERROR_FOR_TESTING), contextIn)
#define CheckOutOfMemoryHRESULT(resultIn, contextIn)                        \
    ::dawn_native::d3d12::CheckOutOfMemoryHRESULTImpl(                      \
        INJECT_ERROR_OR_RUN(resultIn, E_FAKE_OUTOFMEMORY_ERROR_FOR_TESTING, \
                            E_FAKE_ERROR_FOR_TESTING),                      \
        contextIn)

}}  // namespace dawn_native::d3d12

#endif  // DAWNNATIVE_D3D12_D3D12ERROR_H_
