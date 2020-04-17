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

#ifndef DAWNNATIVE_D3D12_D3D12PLATFORM_H_
#define DAWNNATIVE_D3D12_D3D12PLATFORM_H_

#include <d3d12.h>
#include <dxgi1_4.h>
#include <wrl.h>

#include <dxgidebug.h>

using Microsoft::WRL::ComPtr;

// Remove windows.h macros after d3d12's include of windows.h
#include "common/Platform.h"
#if defined(DAWN_PLATFORM_WINDOWS)
#    include "common/windows_with_undefs.h"
#endif

#endif  // DAWNNATIVE_D3D12_D3D12PLATFORM_H_
