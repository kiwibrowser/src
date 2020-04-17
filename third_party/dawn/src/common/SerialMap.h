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

#ifndef COMMON_SERIALMAP_H_
#define COMMON_SERIALMAP_H_

#include "common/SerialStorage.h"

#include <map>
#include <vector>

template <typename T>
class SerialMap;

template <typename T>
struct SerialStorageTraits<SerialMap<T>> {
    using Value = T;
    using Storage = std::map<Serial, std::vector<T>>;
    using StorageIterator = typename Storage::iterator;
    using ConstStorageIterator = typename Storage::const_iterator;
};

// SerialMap stores a map from Serial to T.
// Unlike SerialQueue, items may be enqueued with Serials in any
// arbitrary order. SerialMap provides useful iterators for iterating
// through T items in order of increasing Serial.
template <typename T>
class SerialMap : public SerialStorage<SerialMap<T>> {
  public:
    void Enqueue(const T& value, Serial serial);
    void Enqueue(T&& value, Serial serial);
    void Enqueue(const std::vector<T>& values, Serial serial);
    void Enqueue(std::vector<T>&& values, Serial serial);
};

// SerialMap

template <typename T>
void SerialMap<T>::Enqueue(const T& value, Serial serial) {
    this->mStorage[serial].emplace_back(value);
}

template <typename T>
void SerialMap<T>::Enqueue(T&& value, Serial serial) {
    this->mStorage[serial].emplace_back(value);
}

template <typename T>
void SerialMap<T>::Enqueue(const std::vector<T>& values, Serial serial) {
    DAWN_ASSERT(values.size() > 0);
    for (const T& value : values) {
        Enqueue(value, serial);
    }
}

template <typename T>
void SerialMap<T>::Enqueue(std::vector<T>&& values, Serial serial) {
    DAWN_ASSERT(values.size() > 0);
    for (const T& value : values) {
        Enqueue(value, serial);
    }
}

#endif  // COMMON_SERIALMAP_H_
