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

#ifndef COMMON_SERIALSTORAGE_H_
#define COMMON_SERIALSTORAGE_H_

#include "common/Assert.h"
#include "common/Serial.h"

#include <cstdint>
#include <utility>

template <typename T>
struct SerialStorageTraits {};

template <typename Derived>
class SerialStorage {
  private:
    using Value = typename SerialStorageTraits<Derived>::Value;
    using Storage = typename SerialStorageTraits<Derived>::Storage;
    using StorageIterator = typename SerialStorageTraits<Derived>::StorageIterator;
    using ConstStorageIterator = typename SerialStorageTraits<Derived>::ConstStorageIterator;

  public:
    class Iterator {
      public:
        Iterator(StorageIterator start);
        Iterator& operator++();

        bool operator==(const Iterator& other) const;
        bool operator!=(const Iterator& other) const;
        Value& operator*() const;

      private:
        StorageIterator mStorageIterator;
        // Special case the mSerialIterator when it should be equal to mStorageIterator.begin()
        // otherwise we could ask mStorageIterator.begin() when mStorageIterator is mStorage.end()
        // which is invalid. mStorageIterator.begin() is tagged with a nullptr.
        Value* mSerialIterator;
    };

    class ConstIterator {
      public:
        ConstIterator(ConstStorageIterator start);
        ConstIterator& operator++();

        bool operator==(const ConstIterator& other) const;
        bool operator!=(const ConstIterator& other) const;
        const Value& operator*() const;

      private:
        ConstStorageIterator mStorageIterator;
        const Value* mSerialIterator;
    };

    class BeginEnd {
      public:
        BeginEnd(StorageIterator start, StorageIterator end);

        Iterator begin() const;
        Iterator end() const;

      private:
        StorageIterator mStartIt;
        StorageIterator mEndIt;
    };

    class ConstBeginEnd {
      public:
        ConstBeginEnd(ConstStorageIterator start, ConstStorageIterator end);

        ConstIterator begin() const;
        ConstIterator end() const;

      private:
        ConstStorageIterator mStartIt;
        ConstStorageIterator mEndIt;
    };

    // Derived classes may specialize constraits for elements stored
    // Ex.) SerialQueue enforces that the serial must be given in (not strictly)
    //      increasing order
    template <typename... Params>
    void Enqueue(Params&&... args, Serial serial) {
        Derived::Enqueue(std::forward<Params>(args)..., serial);
    }

    bool Empty() const;

    // The UpTo variants of Iterate and Clear affect all values associated to a serial
    // that is smaller OR EQUAL to the given serial. Iterating is done like so:
    //     for (const T& value : queue.IterateAll()) { stuff(T); }
    ConstBeginEnd IterateAll() const;
    ConstBeginEnd IterateUpTo(Serial serial) const;
    BeginEnd IterateAll();
    BeginEnd IterateUpTo(Serial serial);

    void Clear();
    void ClearUpTo(Serial serial);

    Serial FirstSerial() const;
    Serial LastSerial() const;

  protected:
    // Returns the first StorageIterator that a serial bigger than serial.
    ConstStorageIterator FindUpTo(Serial serial) const;
    StorageIterator FindUpTo(Serial serial);
    Storage mStorage;
};

// SerialStorage

template <typename Derived>
bool SerialStorage<Derived>::Empty() const {
    return mStorage.empty();
}

template <typename Derived>
typename SerialStorage<Derived>::ConstBeginEnd SerialStorage<Derived>::IterateAll() const {
    return {mStorage.begin(), mStorage.end()};
}

template <typename Derived>
typename SerialStorage<Derived>::ConstBeginEnd SerialStorage<Derived>::IterateUpTo(
    Serial serial) const {
    return {mStorage.begin(), FindUpTo(serial)};
}

template <typename Derived>
typename SerialStorage<Derived>::BeginEnd SerialStorage<Derived>::IterateAll() {
    return {mStorage.begin(), mStorage.end()};
}

template <typename Derived>
typename SerialStorage<Derived>::BeginEnd SerialStorage<Derived>::IterateUpTo(Serial serial) {
    return {mStorage.begin(), FindUpTo(serial)};
}

template <typename Derived>
void SerialStorage<Derived>::Clear() {
    mStorage.clear();
}

template <typename Derived>
void SerialStorage<Derived>::ClearUpTo(Serial serial) {
    mStorage.erase(mStorage.begin(), FindUpTo(serial));
}

template <typename Derived>
Serial SerialStorage<Derived>::FirstSerial() const {
    DAWN_ASSERT(!Empty());
    return mStorage.begin()->first;
}

template <typename Derived>
Serial SerialStorage<Derived>::LastSerial() const {
    DAWN_ASSERT(!Empty());
    return mStorage.back().first;
}

template <typename Derived>
typename SerialStorage<Derived>::ConstStorageIterator SerialStorage<Derived>::FindUpTo(
    Serial serial) const {
    auto it = mStorage.begin();
    while (it != mStorage.end() && it->first <= serial) {
        it++;
    }
    return it;
}

template <typename Derived>
typename SerialStorage<Derived>::StorageIterator SerialStorage<Derived>::FindUpTo(Serial serial) {
    auto it = mStorage.begin();
    while (it != mStorage.end() && it->first <= serial) {
        it++;
    }
    return it;
}

// SerialStorage::BeginEnd

template <typename Derived>
SerialStorage<Derived>::BeginEnd::BeginEnd(typename SerialStorage<Derived>::StorageIterator start,
                                           typename SerialStorage<Derived>::StorageIterator end)
    : mStartIt(start), mEndIt(end) {
}

template <typename Derived>
typename SerialStorage<Derived>::Iterator SerialStorage<Derived>::BeginEnd::begin() const {
    return {mStartIt};
}

template <typename Derived>
typename SerialStorage<Derived>::Iterator SerialStorage<Derived>::BeginEnd::end() const {
    return {mEndIt};
}

// SerialStorage::Iterator

template <typename Derived>
SerialStorage<Derived>::Iterator::Iterator(typename SerialStorage<Derived>::StorageIterator start)
    : mStorageIterator(start), mSerialIterator(nullptr) {
}

template <typename Derived>
typename SerialStorage<Derived>::Iterator& SerialStorage<Derived>::Iterator::operator++() {
    Value* vectorData = mStorageIterator->second.data();

    if (mSerialIterator == nullptr) {
        mSerialIterator = vectorData + 1;
    } else {
        mSerialIterator++;
    }

    if (mSerialIterator >= vectorData + mStorageIterator->second.size()) {
        mSerialIterator = nullptr;
        mStorageIterator++;
    }

    return *this;
}

template <typename Derived>
bool SerialStorage<Derived>::Iterator::operator==(
    const typename SerialStorage<Derived>::Iterator& other) const {
    return other.mStorageIterator == mStorageIterator && other.mSerialIterator == mSerialIterator;
}

template <typename Derived>
bool SerialStorage<Derived>::Iterator::operator!=(
    const typename SerialStorage<Derived>::Iterator& other) const {
    return !(*this == other);
}

template <typename Derived>
typename SerialStorage<Derived>::Value& SerialStorage<Derived>::Iterator::operator*() const {
    if (mSerialIterator == nullptr) {
        return *mStorageIterator->second.begin();
    }
    return *mSerialIterator;
}

// SerialStorage::ConstBeginEnd

template <typename Derived>
SerialStorage<Derived>::ConstBeginEnd::ConstBeginEnd(
    typename SerialStorage<Derived>::ConstStorageIterator start,
    typename SerialStorage<Derived>::ConstStorageIterator end)
    : mStartIt(start), mEndIt(end) {
}

template <typename Derived>
typename SerialStorage<Derived>::ConstIterator SerialStorage<Derived>::ConstBeginEnd::begin()
    const {
    return {mStartIt};
}

template <typename Derived>
typename SerialStorage<Derived>::ConstIterator SerialStorage<Derived>::ConstBeginEnd::end() const {
    return {mEndIt};
}

// SerialStorage::ConstIterator

template <typename Derived>
SerialStorage<Derived>::ConstIterator::ConstIterator(
    typename SerialStorage<Derived>::ConstStorageIterator start)
    : mStorageIterator(start), mSerialIterator(nullptr) {
}

template <typename Derived>
typename SerialStorage<Derived>::ConstIterator& SerialStorage<Derived>::ConstIterator::
operator++() {
    const Value* vectorData = mStorageIterator->second.data();

    if (mSerialIterator == nullptr) {
        mSerialIterator = vectorData + 1;
    } else {
        mSerialIterator++;
    }

    if (mSerialIterator >= vectorData + mStorageIterator->second.size()) {
        mSerialIterator = nullptr;
        mStorageIterator++;
    }

    return *this;
}

template <typename Derived>
bool SerialStorage<Derived>::ConstIterator::operator==(
    const typename SerialStorage<Derived>::ConstIterator& other) const {
    return other.mStorageIterator == mStorageIterator && other.mSerialIterator == mSerialIterator;
}

template <typename Derived>
bool SerialStorage<Derived>::ConstIterator::operator!=(
    const typename SerialStorage<Derived>::ConstIterator& other) const {
    return !(*this == other);
}

template <typename Derived>
const typename SerialStorage<Derived>::Value& SerialStorage<Derived>::ConstIterator::operator*()
    const {
    if (mSerialIterator == nullptr) {
        return *mStorageIterator->second.begin();
    }
    return *mSerialIterator;
}

#endif  // COMMON_SERIALSTORAGE_H_
