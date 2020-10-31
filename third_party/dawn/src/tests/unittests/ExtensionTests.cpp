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

#include <gtest/gtest.h>

#include "dawn_native/Extensions.h"
#include "dawn_native/Instance.h"
#include "dawn_native/null/DeviceNull.h"

class ExtensionTests : public testing::Test {
  public:
    ExtensionTests()
        : testing::Test(),
          mInstanceBase(dawn_native::InstanceBase::Create()),
          mAdapterBase(mInstanceBase.Get()) {
    }

    std::vector<const char*> GetAllExtensionNames() {
        std::vector<const char*> allExtensionNames(kTotalExtensionsCount);
        for (size_t i = 0; i < kTotalExtensionsCount; ++i) {
            allExtensionNames[i] = ExtensionEnumToName(static_cast<dawn_native::Extension>(i));
        }
        return allExtensionNames;
    }

    static constexpr size_t kTotalExtensionsCount =
        static_cast<size_t>(dawn_native::Extension::EnumCount);

  protected:
    Ref<dawn_native::InstanceBase> mInstanceBase;
    dawn_native::null::Adapter mAdapterBase;
};

// Test the creation of a device will fail if the requested extension is not supported on the
// Adapter.
TEST_F(ExtensionTests, AdapterWithRequiredExtensionDisabled) {
    const std::vector<const char*> kAllExtensionNames = GetAllExtensionNames();
    for (size_t i = 0; i < kTotalExtensionsCount; ++i) {
        dawn_native::Extension notSupportedExtension = static_cast<dawn_native::Extension>(i);

        std::vector<const char*> extensionNamesWithoutOne = kAllExtensionNames;
        extensionNamesWithoutOne.erase(extensionNamesWithoutOne.begin() + i);

        mAdapterBase.SetSupportedExtensions(extensionNamesWithoutOne);
        dawn_native::Adapter adapterWithoutExtension(&mAdapterBase);

        dawn_native::DeviceDescriptor deviceDescriptor;
        const char* extensionName = ExtensionEnumToName(notSupportedExtension);
        deviceDescriptor.requiredExtensions = std::vector<const char*>(1, extensionName);
        WGPUDevice deviceWithExtension = adapterWithoutExtension.CreateDevice(&deviceDescriptor);
        ASSERT_EQ(nullptr, deviceWithExtension);
    }
}

// Test Device.GetEnabledExtensions() can return the names of the enabled extensions correctly.
TEST_F(ExtensionTests, GetEnabledExtensions) {
    dawn_native::Adapter adapter(&mAdapterBase);
    for (size_t i = 0; i < kTotalExtensionsCount; ++i) {
        dawn_native::Extension extension = static_cast<dawn_native::Extension>(i);
        const char* extensionName = ExtensionEnumToName(extension);

        dawn_native::DeviceDescriptor deviceDescriptor;
        deviceDescriptor.requiredExtensions = {extensionName};
        dawn_native::DeviceBase* deviceBase =
            reinterpret_cast<dawn_native::DeviceBase*>(adapter.CreateDevice(&deviceDescriptor));
        std::vector<const char*> enabledExtensions = deviceBase->GetEnabledExtensions();
        ASSERT_EQ(1u, enabledExtensions.size());
        ASSERT_EQ(0, std::strcmp(extensionName, enabledExtensions[0]));
    }
}
