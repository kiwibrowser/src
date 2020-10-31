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

#include "tests/perf_tests/DawnPerfTest.h"

#include "tests/ParamGenerator.h"
#include "utils/WGPUHelpers.h"

namespace {

    constexpr unsigned int kNumIterations = 50;

    enum class UploadMethod {
        WriteBuffer,
        CreateBufferMapped,
    };

    // Perf delta exists between ranges [0, 1MB] vs [1MB, MAX_SIZE).
    // These are sample buffer sizes within each range.
    enum class UploadSize {
        BufferSize_1KB = 1 * 1024,
        BufferSize_64KB = 64 * 1024,
        BufferSize_1MB = 1 * 1024 * 1024,

        BufferSize_4MB = 4 * 1024 * 1024,
        BufferSize_16MB = 16 * 1024 * 1024,
    };

    struct BufferUploadParams : AdapterTestParam {
        BufferUploadParams(const AdapterTestParam& param,
                           UploadMethod uploadMethod,
                           UploadSize uploadSize)
            : AdapterTestParam(param), uploadMethod(uploadMethod), uploadSize(uploadSize) {
        }

        UploadMethod uploadMethod;
        UploadSize uploadSize;
    };

    std::ostream& operator<<(std::ostream& ostream, const BufferUploadParams& param) {
        ostream << static_cast<const AdapterTestParam&>(param);

        switch (param.uploadMethod) {
            case UploadMethod::WriteBuffer:
                ostream << "_WriteBuffer";
                break;
            case UploadMethod::CreateBufferMapped:
                ostream << "_CreateBufferMapped";
                break;
        }

        switch (param.uploadSize) {
            case UploadSize::BufferSize_1KB:
                ostream << "_BufferSize_1KB";
                break;
            case UploadSize::BufferSize_64KB:
                ostream << "_BufferSize_64KB";
                break;
            case UploadSize::BufferSize_1MB:
                ostream << "_BufferSize_1MB";
                break;
            case UploadSize::BufferSize_4MB:
                ostream << "_BufferSize_4MB";
                break;
            case UploadSize::BufferSize_16MB:
                ostream << "_BufferSize_16MB";
                break;
        }

        return ostream;
    }

}  // namespace

// Test uploading |kBufferSize| bytes of data |kNumIterations| times.
class BufferUploadPerf : public DawnPerfTestWithParams<BufferUploadParams> {
  public:
    BufferUploadPerf()
        : DawnPerfTestWithParams(kNumIterations, 1),
          data(static_cast<size_t>(GetParam().uploadSize)) {
    }
    ~BufferUploadPerf() override = default;

    void SetUp() override;

  private:
    void Step() override;

    wgpu::Buffer dst;
    std::vector<uint8_t> data;
};

void BufferUploadPerf::SetUp() {
    DawnPerfTestWithParams<BufferUploadParams>::SetUp();

    wgpu::BufferDescriptor desc = {};
    desc.size = data.size();
    desc.usage = wgpu::BufferUsage::CopyDst;

    dst = device.CreateBuffer(&desc);
}

void BufferUploadPerf::Step() {
    switch (GetParam().uploadMethod) {
        case UploadMethod::WriteBuffer: {
            for (unsigned int i = 0; i < kNumIterations; ++i) {
                queue.WriteBuffer(dst, 0, data.data(), data.size());
            }
            // Make sure all WriteBuffer's are flushed.
            queue.Submit(0, nullptr);
            break;
        }

        case UploadMethod::CreateBufferMapped: {
            wgpu::BufferDescriptor desc = {};
            desc.size = data.size();
            desc.usage = wgpu::BufferUsage::CopySrc | wgpu::BufferUsage::MapWrite;

            wgpu::CommandEncoder encoder = device.CreateCommandEncoder();

            for (unsigned int i = 0; i < kNumIterations; ++i) {
                auto result = device.CreateBufferMapped(&desc);
                memcpy(result.data, data.data(), data.size());
                result.buffer.Unmap();
                encoder.CopyBufferToBuffer(result.buffer, 0, dst, 0, data.size());
            }

            wgpu::CommandBuffer commands = encoder.Finish();
            queue.Submit(1, &commands);
            break;
        }
    }
}

TEST_P(BufferUploadPerf, Run) {
    RunTest();
}

DAWN_INSTANTIATE_PERF_TEST_SUITE_P(BufferUploadPerf,
                                   {D3D12Backend(), MetalBackend(), OpenGLBackend(),
                                    VulkanBackend()},
                                   {UploadMethod::WriteBuffer, UploadMethod::CreateBufferMapped},
                                   {UploadSize::BufferSize_1KB, UploadSize::BufferSize_64KB,
                                    UploadSize::BufferSize_1MB, UploadSize::BufferSize_4MB,
                                    UploadSize::BufferSize_16MB});
