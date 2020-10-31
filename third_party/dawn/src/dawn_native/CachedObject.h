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

#ifndef DAWNNATIVE_CACHED_OBJECT_H_
#define DAWNNATIVE_CACHED_OBJECT_H_

#include "dawn_native/ObjectBase.h"

namespace dawn_native {

    // Some objects are cached so that instead of creating new duplicate objects,
    // we increase the refcount of an existing object.
    // When an object is successfully created, the device should call
    // SetIsCachedReference() and insert the object into the cache.
    class CachedObject : public ObjectBase {
      public:
        using ObjectBase::ObjectBase;

        bool IsCachedReference() const;

      private:
        friend class DeviceBase;
        void SetIsCachedReference();

        bool mIsCachedReference = false;
    };

}  // namespace dawn_native

#endif  // DAWNNATIVE_CACHED_OBJECT_H_
