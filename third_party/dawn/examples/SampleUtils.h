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

#include <dawn/dawn_wsi.h>
#include <dawn/webgpu_cpp.h>

bool InitSample(int argc, const char** argv);
void DoFlush();
bool ShouldQuit();

struct GLFWwindow;
struct GLFWwindow* GetGLFWWindow();

wgpu::Device CreateCppDawnDevice();
uint64_t GetSwapChainImplementation();
wgpu::TextureFormat GetPreferredSwapChainTextureFormat();
wgpu::SwapChain GetSwapChain(const wgpu::Device& device);
wgpu::TextureView CreateDefaultDepthStencilView(const wgpu::Device& device);
