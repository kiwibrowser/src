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

#ifndef COMMON_REFCOUNTED_H_
#define COMMON_REFCOUNTED_H_

#include <atomic>
#include <cstdint>

class RefCounted {
  public:
    RefCounted(uint64_t payload = 0);

    uint64_t GetRefCountForTesting() const;
    uint64_t GetRefCountPayload() const;

    // Dawn API
    void Reference();
    void Release();

  protected:
    virtual ~RefCounted() = default;
    // A Derived class may override this if they require a custom deleter.
    virtual void DeleteThis();

  private:
    std::atomic_uint64_t mRefCount;
};

template <typename T>
class Ref {
  public:
    Ref() = default;

    constexpr Ref(std::nullptr_t) {
    }

    Ref& operator=(std::nullptr_t) {
        Release();
        mPointee = nullptr;
        return *this;
    }

    template <typename U>
    Ref(U* p) : mPointee(p) {
        static_assert(std::is_convertible<U*, T*>::value, "");
        Reference();
    }

    Ref(const Ref<T>& other) : mPointee(other.mPointee) {
        Reference();
    }
    template <typename U>
    Ref(const Ref<U>& other) : mPointee(other.mPointee) {
        static_assert(std::is_convertible<U*, T*>::value, "");
        Reference();
    }

    Ref<T>& operator=(const Ref<T>& other) {
        if (&other == this)
            return *this;

        other.Reference();
        Release();
        mPointee = other.mPointee;

        return *this;
    }

    template <typename U>
    Ref<T>& operator=(const Ref<U>& other) {
        static_assert(std::is_convertible<U*, T*>::value, "");

        other.Reference();
        Release();
        mPointee = other.mPointee;

        return *this;
    }

    template <typename U>
    Ref(Ref<U>&& other) {
        static_assert(std::is_convertible<U*, T*>::value, "");
        mPointee = other.mPointee;
        other.mPointee = nullptr;
    }

    Ref<T>& operator=(Ref<T>&& other) {
        if (&other == this)
            return *this;

        Release();
        mPointee = other.mPointee;
        other.mPointee = nullptr;

        return *this;
    }
    template <typename U>
    Ref<T>& operator=(Ref<U>&& other) {
        static_assert(std::is_convertible<U*, T*>::value, "");

        Release();
        mPointee = other.mPointee;
        other.mPointee = nullptr;

        return *this;
    }

    ~Ref() {
        Release();
        mPointee = nullptr;
    }

    bool operator==(const T* other) const {
        return mPointee == other;
    }

    bool operator!=(const T* other) const {
        return mPointee != other;
    }

    operator bool() {
        return mPointee != nullptr;
    }

    const T& operator*() const {
        return *mPointee;
    }
    T& operator*() {
        return *mPointee;
    }

    const T* operator->() const {
        return mPointee;
    }
    T* operator->() {
        return mPointee;
    }

    const T* Get() const {
        return mPointee;
    }
    T* Get() {
        return mPointee;
    }

    T* Detach() {
        T* pointee = mPointee;
        mPointee = nullptr;
        return pointee;
    }

  private:
    // Friend is needed so that instances of Ref<U> can assign mPointee
    // members of Ref<T>.
    template <typename U>
    friend class Ref;

    void Reference() const {
        if (mPointee != nullptr) {
            mPointee->Reference();
        }
    }
    void Release() const {
        if (mPointee != nullptr) {
            mPointee->Release();
        }
    }

    T* mPointee = nullptr;
};

template <typename T>
Ref<T> AcquireRef(T* pointee) {
    Ref<T> ref(pointee);
    ref->Release();
    return ref;
}

#endif  // COMMON_REFCOUNTED_H_
