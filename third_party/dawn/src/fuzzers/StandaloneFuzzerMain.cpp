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

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <vector>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size);

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cout << "Usage: <standalone reproducer> [FILE]" << std::endl;
        return 1;
    }

    std::cout << "WARNING: this is just a best-effort reproducer for fuzzer issues in standalone "
              << "Dawn builds. For the real fuzzer, please build inside Chromium." << std::endl;

    const char* filename = argv[1];
    std::cout << "Reproducing using file: " << filename << std::endl;

    std::vector<char> data;
    {
        FILE* file = fopen(filename, "rb");
        if (!file) {
            std::cerr << "Failed to open " << filename << std::endl;
            return 1;
        }

        fseek(file, 0, SEEK_END);
        long tellFileSize = ftell(file);
        if (tellFileSize <= 0) {
            std::cerr << "Input file of incorrect size: " << filename << std::endl;
            return 1;
        }
        fseek(file, 0, SEEK_SET);

        size_t fileSize = static_cast<size_t>(tellFileSize);
        data.resize(fileSize);

        size_t bytesRead = fread(data.data(), sizeof(char), fileSize, file);
        fclose(file);
        if (bytesRead != fileSize) {
            std::cerr << "Failed to read " << filename << std::endl;
            return 1;
        }
    }

    return LLVMFuzzerTestOneInput(reinterpret_cast<const uint8_t*>(data.data()), data.size());
}
