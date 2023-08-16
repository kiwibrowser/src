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

#include "DawnWireServerFuzzer.h"

#include "common/Assert.h"
#include "common/Log.h"
#include "common/SystemUtils.h"
#include "dawn/dawn_proc.h"
#include "dawn/webgpu_cpp.h"
#include "dawn_native/DawnNative.h"
#include "dawn_wire/WireServer.h"
#include "utils/SystemUtils.h"

#include <fstream>
#include <vector>

namespace {

    class DevNull : public dawn_wire::CommandSerializer {
      public:
        void* GetCmdSpace(size_t size) override {
            if (size > buf.size()) {
                buf.resize(size);
            }
            return buf.data();
        }
        bool Flush() override {
            return true;
        }

      private:
        std::vector<char> buf;
    };

    std::unique_ptr<dawn_native::Instance> sInstance;
    WGPUProcDeviceCreateSwapChain sOriginalDeviceCreateSwapChain = nullptr;

    std::string sInjectedErrorTestcaseOutDir;
    uint64_t sOutputFileNumber = 0;

    bool sCommandsComplete = false;

    WGPUSwapChain ErrorDeviceCreateSwapChain(WGPUDevice device,
                                             WGPUSurface surface,
                                             const WGPUSwapChainDescriptor*) {
        WGPUSwapChainDescriptor desc = {};
        // A 0 implementation will trigger a swapchain creation error.
        desc.implementation = 0;
        return sOriginalDeviceCreateSwapChain(device, surface, &desc);
    }

    void CommandsCompleteCallback(WGPUFenceCompletionStatus status, void* userdata) {
        sCommandsComplete = true;
    }

}  // namespace

int DawnWireServerFuzzer::Initialize(int* argc, char*** argv) {
    ASSERT(argc != nullptr && argv != nullptr);

    // The first argument (the fuzzer binary) always stays the same.
    int argcOut = 1;

    for (int i = 1; i < *argc; ++i) {
        constexpr const char kInjectedErrorTestcaseDirArg[] = "--injected-error-testcase-dir=";
        if (strstr((*argv)[i], kInjectedErrorTestcaseDirArg) == (*argv)[i]) {
            sInjectedErrorTestcaseOutDir = (*argv)[i] + strlen(kInjectedErrorTestcaseDirArg);
            const char* sep = GetPathSeparator();
            if (sInjectedErrorTestcaseOutDir.back() != *sep) {
                sInjectedErrorTestcaseOutDir += sep;
            }
            // Log so that it's clear the fuzzer found the argument.
            dawn::InfoLog() << "Generating injected errors, output dir is: \""
                            << sInjectedErrorTestcaseOutDir << "\"";
            continue;
        }

        // Move any unconsumed arguments to the next slot in the output array.
        (*argv)[argcOut++] = (*argv)[i];
    }

    // Write the argument count
    *argc = argcOut;

    // TODO(crbug.com/1038952): The Instance must be static because destructing the vkInstance with
    // Swiftshader crashes libFuzzer. When this is fixed, move this into Run so that error injection
    // for adapter discovery can be fuzzed.
    sInstance = std::make_unique<dawn_native::Instance>();
    sInstance->DiscoverDefaultAdapters();

    return 0;
}

int DawnWireServerFuzzer::Run(const uint8_t* data,
                              size_t size,
                              MakeDeviceFn MakeDevice,
                              bool supportsErrorInjection) {
    bool didInjectError = false;

    if (supportsErrorInjection) {
        dawn_native::EnableErrorInjector();

        // Clear the error injector since it has the previous run's call counts.
        dawn_native::ClearErrorInjector();

        // If we're outputing testcases with injected errors, we run the fuzzer on the original
        // input data, and prepend injected errors to it. In the case, where we're NOT outputing,
        // we use the first bytes as the injected error index.
        if (sInjectedErrorTestcaseOutDir.empty() && size >= sizeof(uint64_t)) {
            // Otherwise, use the first bytes as the injected error index.
            dawn_native::InjectErrorAt(*reinterpret_cast<const uint64_t*>(data));
            didInjectError = true;

            data += sizeof(uint64_t);
            size -= sizeof(uint64_t);
        }
    }

    DawnProcTable procs = dawn_native::GetProcs();

    // Swapchains receive a pointer to an implementation. The fuzzer will pass garbage in so we
    // intercept calls to create swapchains and make sure they always return error swapchains.
    // This is ok for fuzzing because embedders of dawn_wire would always define their own
    // swapchain handling.
    sOriginalDeviceCreateSwapChain = procs.deviceCreateSwapChain;
    procs.deviceCreateSwapChain = ErrorDeviceCreateSwapChain;

    dawnProcSetProcs(&procs);

    wgpu::Device device = MakeDevice(sInstance.get());
    if (!device) {
        // We should only ever fail device creation if an error was injected.
        ASSERT(didInjectError);
        return 0;
    }

    DevNull devNull;
    dawn_wire::WireServerDescriptor serverDesc = {};
    serverDesc.device = device.Get();
    serverDesc.procs = &procs;
    serverDesc.serializer = &devNull;

    std::unique_ptr<dawn_wire::WireServer> wireServer(new dawn_wire::WireServer(serverDesc));

    wireServer->HandleCommands(reinterpret_cast<const char*>(data), size);

    // Wait for all previous commands before destroying the server.
    // TODO(enga): Improve this when we improve/finalize how processing events happens.
    {
        wgpu::Queue queue = device.GetDefaultQueue();
        wgpu::Fence fence = queue.CreateFence();
        queue.Signal(fence, 1u);
        fence.OnCompletion(1u, CommandsCompleteCallback, 0);
        while (!sCommandsComplete) {
            device.Tick();
            utils::USleep(100);
        }
    }

    // Destroy the server before the device because it needs to free all objects.
    wireServer = nullptr;
    device = nullptr;

    // If we support error injection, and an output directory was provided, output copies of the
    // original testcase data, prepended with the injected error index.
    if (supportsErrorInjection && !sInjectedErrorTestcaseOutDir.empty()) {
        const uint64_t injectedCallCount = dawn_native::AcquireErrorInjectorCallCount();

        auto WriteTestcase = [&](uint64_t i) {
            std::ofstream outFile(
                sInjectedErrorTestcaseOutDir + "injected_error_testcase_" +
                    std::to_string(sOutputFileNumber++),
                std::ios_base::out | std::ios_base::binary | std::ios_base::trunc);
            outFile.write(reinterpret_cast<const char*>(&i), sizeof(i));
            outFile.write(reinterpret_cast<const char*>(data), size);
        };

        for (uint64_t i = 0; i < injectedCallCount; ++i) {
            WriteTestcase(i);
        }

        // Also add a testcase where the injected error is so large no errors should occur.
        WriteTestcase(std::numeric_limits<uint64_t>::max());
    }

    return 0;
}
