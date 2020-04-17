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

#ifndef COMMON_SERIALQUEUE_H_
#define COMMON_SERIALQUEUE_H_

#include "common/SerialStorage.h"

#include <vector>

template <typename T>
class SerialQueue;

template <typename T>
struct SerialStorageTraits<SerialQueue<T>> {
    using Value = T;
    using SerialPair = std::pair<Serial, std::vector<T>>;
    using Storage = std::vector<SerialPair>;
    using StorageIterator = typename Storage::iterator;
    using ConstStorageIterator = typename Storage::const_iterator;
};

// SerialQueue stores an associative list mapping a Serial to T.
// It enforces that the Serials enqueued are strictly non-decreasing.
// This makes it very efficient iterate or clear all items added up
// to some Serial value because they are stored contiguously in memory.
template <typename T>
class SerialQueue : public SerialStorage<SerialQueue<T>> {
  public:
    using SerialPair = typename SerialStorageTraits<SerialQueue<T>>::SerialPair;

    // The serial must be given in (not strictly) increasing order.
    void Enqueue(const T& value, Serial serial);
    void Enqueue(T&& value, Serial serial);
    void Enqueue(const std::vector<T>& values, Serial serial);
    void Enqueue(std::vector<T>&& values, Serial serial);
};

// SerialQueue

template <typename T>
void SerialQueue<T>::Enqueue(const T& value, Serial serial) {
    DAWN_ASSERT(this->Empty() || this->mStorage.back().first <= serial);

    if (this->Empty() || this->mStorage.back().first < serial) {
        this->mStorage.emplace_back(serial, std::vector<T>{});
    }
    this->mStorage.back().second.push_back(value);
}

template <typename T>
void SerialQueue<T>::Enqueue(T&& value, Serial serial) {
    DAWN_ASSERT(this->Empty() || this->mStorage.back().first <= serial);

    if (this->Empty() || this->mStorage.back().first < serial) {
        this->mStorage.emplace_back(serial, std::vector<T>{});
    }
    this->mStorage.back().second.push_back(std::move(value));
}

template <typename T>
void SerialQueue<T>::Enqueue(const std::vector<T>& values, Serial serial) {
    DAWN_ASSERT(values.size() > 0);
    DAWN_ASSERT(this->Empty() || this->mStorage.back().first <= serial);
    this->mStorage.emplace_back(serial, values);
}

template <typename T>
void SerialQueue<T>::Enqueue(std::vector<T>&& values, Serial serial) {
    DAWN_ASSERT(values.size() > 0);
    DAWN_ASSERT(this->Empty() || this->mStorage.back().first <= serial);
    this->mStorage.emplace_back(serial, values);
}

#endif  // COMMON_SERIALQUEUE_H_
