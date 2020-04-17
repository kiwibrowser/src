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

#ifndef DAWNNATIVE_OBJECTBASE_H_
#define DAWNNATIVE_OBJECTBASE_H_

#include "dawn_native/RefCounted.h"

namespace dawn_native {

    class DeviceBase;

    class ObjectBase : public RefCounted {
      public:
        struct ErrorTag {};
        static constexpr ErrorTag kError = {};

        ObjectBase(DeviceBase* device);
        ObjectBase(DeviceBase* device, ErrorTag tag);
        virtual ~ObjectBase();

        DeviceBase* GetDevice() const;
        bool IsError() const;

      private:
        DeviceBase* mDevice;
        // TODO(cwallez@chromium.org): This most likely adds 4 bytes to most Dawn objects, see if
        // that bit can be hidden in the refcount once it is a single 64bit refcount.
        // See https://bugs.chromium.org/p/dawn/issues/detail?id=105
        bool mIsError;
    };

}  // namespace dawn_native

#endif  // DAWNNATIVE_OBJECTBASE_H_
