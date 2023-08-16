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

#ifndef DAWNWIRE_SERVER_OBJECTSTORAGE_H_
#define DAWNWIRE_SERVER_OBJECTSTORAGE_H_

#include "dawn_wire/WireCmd_autogen.h"
#include "dawn_wire/WireServer.h"

#include <algorithm>
#include <map>

namespace dawn_wire { namespace server {

    template <typename T>
    struct ObjectDataBase {
        // The backend-provided handle and generation to this object.
        T handle;
        uint32_t generation = 0;

        // Whether this object has been allocated, used by the KnownObjects queries
        // TODO(cwallez@chromium.org): make this an internal bit vector in KnownObjects.
        bool allocated;
    };

    // Stores what the backend knows about the type.
    template <typename T>
    struct ObjectData : public ObjectDataBase<T> {};

    enum class BufferMapWriteState { Unmapped, Mapped, MapError };

    template <>
    struct ObjectData<WGPUBuffer> : public ObjectDataBase<WGPUBuffer> {
        // TODO(enga): Use a tagged pointer to save space.
        std::unique_ptr<MemoryTransferService::ReadHandle> readHandle;
        std::unique_ptr<MemoryTransferService::WriteHandle> writeHandle;
        BufferMapWriteState mapWriteState = BufferMapWriteState::Unmapped;
    };

    // Keeps track of the mapping between client IDs and backend objects.
    template <typename T>
    class KnownObjects {
      public:
        using Data = ObjectData<T>;

        KnownObjects() {
            // Reserve ID 0 so that it can be used to represent nullptr for optional object values
            // in the wire format. However don't tag it as allocated so that it is an error to ask
            // KnownObjects for ID 0.
            Data reservation;
            reservation.handle = nullptr;
            reservation.allocated = false;
            mKnown.push_back(std::move(reservation));
        }

        // Get a backend objects for a given client ID.
        // Returns nullptr if the ID hasn't previously been allocated.
        const Data* Get(uint32_t id) const {
            if (id >= mKnown.size()) {
                return nullptr;
            }

            const Data* data = &mKnown[id];

            if (!data->allocated) {
                return nullptr;
            }

            return data;
        }
        Data* Get(uint32_t id) {
            if (id >= mKnown.size()) {
                return nullptr;
            }

            Data* data = &mKnown[id];

            if (!data->allocated) {
                return nullptr;
            }

            return data;
        }

        // Allocates the data for a given ID and returns it.
        // Returns nullptr if the ID is already allocated, or too far ahead, or if ID is 0 (ID 0 is
        // reserved for nullptr). Invalidates all the Data*
        Data* Allocate(uint32_t id) {
            if (id == 0 || id > mKnown.size()) {
                return nullptr;
            }

            Data data;
            data.allocated = true;
            data.handle = nullptr;

            if (id >= mKnown.size()) {
                mKnown.push_back(std::move(data));
                return &mKnown.back();
            }

            if (mKnown[id].allocated) {
                return nullptr;
            }

            mKnown[id] = std::move(data);
            return &mKnown[id];
        }

        // Marks an ID as deallocated
        void Free(uint32_t id) {
            ASSERT(id < mKnown.size());
            mKnown[id].allocated = false;
        }

        std::vector<T> AcquireAllHandles() {
            std::vector<T> objects;
            for (Data& data : mKnown) {
                if (data.allocated && data.handle != nullptr) {
                    objects.push_back(data.handle);
                    data.allocated = false;
                    data.handle = nullptr;
                }
            }

            return objects;
        }

      private:
        std::vector<Data> mKnown;
    };

    // ObjectIds are lost in deserialization. Store the ids of deserialized
    // objects here so they can be used in command handlers. This is useful
    // for creating ReturnWireCmds which contain client ids
    template <typename T>
    class ObjectIdLookupTable {
      public:
        void Store(T key, ObjectId id) {
            mTable[key] = id;
        }

        // Return the cached ObjectId, or 0 (null handle)
        ObjectId Get(T key) const {
            const auto it = mTable.find(key);
            if (it != mTable.end()) {
                return it->second;
            }
            return 0;
        }

        void Remove(T key) {
            auto it = mTable.find(key);
            if (it != mTable.end()) {
                mTable.erase(it);
            }
        }

      private:
        std::map<T, ObjectId> mTable;
    };

}}  // namespace dawn_wire::server

#endif  // DAWNWIRE_SERVER_OBJECTSTORAGE_H_
