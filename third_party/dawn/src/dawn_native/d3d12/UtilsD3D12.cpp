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

#include "dawn_native/d3d12/UtilsD3D12.h"

#include "common/Assert.h"

namespace dawn_native { namespace d3d12 {

    D3D12_COMPARISON_FUNC ToD3D12ComparisonFunc(dawn::CompareFunction func) {
        switch (func) {
            case dawn::CompareFunction::Always:
                return D3D12_COMPARISON_FUNC_ALWAYS;
            case dawn::CompareFunction::Equal:
                return D3D12_COMPARISON_FUNC_EQUAL;
            case dawn::CompareFunction::Greater:
                return D3D12_COMPARISON_FUNC_GREATER;
            case dawn::CompareFunction::GreaterEqual:
                return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
            case dawn::CompareFunction::Less:
                return D3D12_COMPARISON_FUNC_LESS;
            case dawn::CompareFunction::LessEqual:
                return D3D12_COMPARISON_FUNC_LESS_EQUAL;
            case dawn::CompareFunction::Never:
                return D3D12_COMPARISON_FUNC_NEVER;
            case dawn::CompareFunction::NotEqual:
                return D3D12_COMPARISON_FUNC_NOT_EQUAL;
            default:
                UNREACHABLE();
        }
    }

}}  // namespace dawn_native::d3d12
