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

#include "dawn_native/opengl/BackendGL.h"

#include "dawn_native/OpenGLBackend.h"
#include "dawn_native/opengl/DeviceGL.h"

namespace dawn_native { namespace opengl {

    // The OpenGL backend's Adapter.

    class Adapter : public AdapterBase {
      public:
        Adapter(InstanceBase* instance) : AdapterBase(instance, BackendType::OpenGL) {
        }

        MaybeError Initialize(const AdapterDiscoveryOptions* options) {
            // Use getProc to populate the dispatch table
            DAWN_TRY(mFunctions.Initialize(options->getProc));

            // Set state that never changes between devices.
            mFunctions.Enable(GL_DEPTH_TEST);
            mFunctions.Enable(GL_SCISSOR_TEST);
            mFunctions.Enable(GL_PRIMITIVE_RESTART_FIXED_INDEX);
            mFunctions.Enable(GL_MULTISAMPLE);

            mPCIInfo.name = reinterpret_cast<const char*>(mFunctions.GetString(GL_RENDERER));

            return {};
        }

        ~Adapter() override = default;

      private:
        OpenGLFunctions mFunctions;

        ResultOrError<DeviceBase*> CreateDeviceImpl(const DeviceDescriptor* descriptor) override {
            // There is no limit on the number of devices created from this adapter because they can
            // all share the same backing OpenGL context.
            return {new Device(this, descriptor, mFunctions)};
        }
    };

    // Implementation of the OpenGL backend's BackendConnection

    Backend::Backend(InstanceBase* instance) : BackendConnection(instance, BackendType::OpenGL) {
    }

    std::vector<std::unique_ptr<AdapterBase>> Backend::DiscoverDefaultAdapters() {
        // The OpenGL backend needs at least "getProcAddress" to discover an adapter.
        return {};
    }

    ResultOrError<std::vector<std::unique_ptr<AdapterBase>>> Backend::DiscoverAdapters(
        const AdapterDiscoveryOptionsBase* optionsBase) {
        // TODO(cwallez@chromium.org): For now only create a single OpenGL adapter because don't
        // know how to handle MakeCurrent.
        if (mCreatedAdapter) {
            return DAWN_VALIDATION_ERROR("The OpenGL backend can only create a single adapter");
        }

        ASSERT(optionsBase->backendType == BackendType::OpenGL);
        const AdapterDiscoveryOptions* options =
            static_cast<const AdapterDiscoveryOptions*>(optionsBase);

        if (options->getProc == nullptr) {
            return DAWN_VALIDATION_ERROR("AdapterDiscoveryOptions::getProc must be set");
        }

        std::unique_ptr<Adapter> adapter = std::make_unique<Adapter>(GetInstance());
        DAWN_TRY(adapter->Initialize(options));

        mCreatedAdapter = true;
        std::vector<std::unique_ptr<AdapterBase>> adapters;
        adapters.push_back(std::unique_ptr<AdapterBase>(adapter.release()));
        return std::move(adapters);
    }

    BackendConnection* Connect(InstanceBase* instance) {
        return new Backend(instance);
    }

}}  // namespace dawn_native::opengl
