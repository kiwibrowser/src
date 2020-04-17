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

#ifndef DAWNNATIVE_INSTANCE_H_
#define DAWNNATIVE_INSTANCE_H_

#include "dawn_native/Adapter.h"
#include "dawn_native/BackendConnection.h"
#include "dawn_native/Toggles.h"

#include <array>
#include <memory>
#include <unordered_map>
#include <vector>

namespace dawn_native {

    // This is called InstanceBase for consistency across the frontend, even if the backends don't
    // specialize this class.
    class InstanceBase final {
      public:
        InstanceBase() = default;
        ~InstanceBase() = default;

        InstanceBase(const InstanceBase& other) = delete;
        InstanceBase& operator=(const InstanceBase& other) = delete;

        void DiscoverDefaultAdapters();
        bool DiscoverAdapters(const AdapterDiscoveryOptionsBase* options);

        const std::vector<std::unique_ptr<AdapterBase>>& GetAdapters() const;

        // Used to handle error that happen up to device creation.
        bool ConsumedError(MaybeError maybeError);

        // Used to query the details of a toggle. Return nullptr if toggleName is not a valid name
        // of a toggle supported in Dawn.
        const ToggleInfo* GetToggleInfo(const char* toggleName);

        Toggle ToggleNameToEnum(const char* toggleName);
        const char* ToggleEnumToName(Toggle toggle);

        void EnableBackendValidation(bool enableBackendValidation);
        bool IsBackendValidationEnabled();

      private:
        // Lazily creates connections to all backends that have been compiled.
        void EnsureBackendConnections();

        // Finds the BackendConnection for `type` or returns an error.
        ResultOrError<BackendConnection*> FindBackend(BackendType type);

        MaybeError DiscoverAdaptersInternal(const AdapterDiscoveryOptionsBase* options);

        void EnsureToggleNameToEnumMapInitialized();

        bool mBackendsConnected = false;
        bool mDiscoveredDefaultAdapters = false;

        bool mToggleNameToEnumMapInitialized = false;
        bool mEnableBackendValidation = false;

        std::vector<std::unique_ptr<BackendConnection>> mBackends;
        std::vector<std::unique_ptr<AdapterBase>> mAdapters;

        std::unordered_map<std::string, Toggle> mToggleNameToEnumMap;
    };

}  // namespace dawn_native

#endif  // DAWNNATIVE_INSTANCE_H_
