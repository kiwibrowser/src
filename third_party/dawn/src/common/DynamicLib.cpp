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

#include "common/DynamicLib.h"

#include "common/Platform.h"

#if DAWN_PLATFORM_WINDOWS
#    include "common/windows_with_undefs.h"
#elif DAWN_PLATFORM_POSIX
#    include <dlfcn.h>
#else
#    error "Unsupported platform for DynamicLib"
#endif

DynamicLib::~DynamicLib() {
    Close();
}

DynamicLib::DynamicLib(DynamicLib&& other) {
    std::swap(mHandle, other.mHandle);
}

DynamicLib& DynamicLib::operator=(DynamicLib&& other) {
    std::swap(mHandle, other.mHandle);
    return *this;
}

bool DynamicLib::Valid() const {
    return mHandle != nullptr;
}

bool DynamicLib::Open(const std::string& filename, std::string* error) {
#if DAWN_PLATFORM_WINDOWS
    mHandle = LoadLibraryA(filename.c_str());

    if (mHandle == nullptr && error != nullptr) {
        *error = "Windows Error: " + std::to_string(GetLastError());
    }
#elif DAWN_PLATFORM_POSIX
    mHandle = dlopen(filename.c_str(), RTLD_NOW);

    if (mHandle == nullptr && error != nullptr) {
        *error = dlerror();
    }
#else
#    error "Unsupported platform for DynamicLib"
#endif

    return mHandle != nullptr;
}

void DynamicLib::Close() {
    if (mHandle == nullptr) {
        return;
    }

#if DAWN_PLATFORM_WINDOWS
    FreeLibrary(static_cast<HMODULE>(mHandle));
#elif DAWN_PLATFORM_POSIX
    dlclose(mHandle);
#else
#    error "Unsupported platform for DynamicLib"
#endif

    mHandle = nullptr;
}

void* DynamicLib::GetProc(const std::string& procName, std::string* error) const {
    void* proc = nullptr;

#if DAWN_PLATFORM_WINDOWS
    proc = reinterpret_cast<void*>(GetProcAddress(static_cast<HMODULE>(mHandle), procName.c_str()));

    if (proc == nullptr && error != nullptr) {
        *error = "Windows Error: " + std::to_string(GetLastError());
    }
#elif DAWN_PLATFORM_POSIX
    proc = reinterpret_cast<void*>(dlsym(mHandle, procName.c_str()));

    if (proc == nullptr && error != nullptr) {
        *error = dlerror();
    }
#else
#    error "Unsupported platform for DynamicLib"
#endif

    return proc;
}
