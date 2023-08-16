// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file is a modified copy of Chromium's /src/base/containers/stack_container_unittest.cc

#include <gtest/gtest.h>

#include "common/RefCounted.h"
#include "common/StackContainer.h"

#include <algorithm>
#include <cstddef>

namespace {

    class Dummy : public RefCounted {
      public:
        explicit Dummy(int* alive) : mAlive(alive) {
            ++*mAlive;
        }

      private:
        ~Dummy() {
            --*mAlive;
        }

        int* const mAlive;
    };

}  // namespace

TEST(StackContainer, Vector) {
    const int stack_size = 3;
    StackVector<int, stack_size> vect;
    const int* stack_buffer = &vect.stack_data().stack_buffer()[0];

    // The initial |stack_size| elements should appear in the stack buffer.
    EXPECT_EQ(static_cast<size_t>(stack_size), vect.container().capacity());
    for (int i = 0; i < stack_size; i++) {
        vect.container().push_back(i);
        EXPECT_EQ(stack_buffer, &vect.container()[0]);
        EXPECT_TRUE(vect.stack_data().used_stack_buffer_);
    }

    // Adding more elements should push the array onto the heap.
    for (int i = 0; i < stack_size; i++) {
        vect.container().push_back(i + stack_size);
        EXPECT_NE(stack_buffer, &vect.container()[0]);
        EXPECT_FALSE(vect.stack_data().used_stack_buffer_);
    }

    // The array should still be in order.
    for (int i = 0; i < stack_size * 2; i++)
        EXPECT_EQ(i, vect.container()[i]);

    // Resize to smaller. Our STL implementation won't reallocate in this case,
    // otherwise it might use our stack buffer. We reserve right after the resize
    // to guarantee it isn't using the stack buffer, even though it doesn't have
    // much data.
    vect.container().resize(stack_size);
    vect.container().reserve(stack_size * 2);
    EXPECT_FALSE(vect.stack_data().used_stack_buffer_);

    // Copying the small vector to another should use the same allocator and use
    // the now-unused stack buffer. GENERALLY CALLERS SHOULD NOT DO THIS since
    // they have to get the template types just right and it can cause errors.
    std::vector<int, StackAllocator<int, stack_size>> other(vect.container());
    EXPECT_EQ(stack_buffer, &other.front());
    EXPECT_TRUE(vect.stack_data().used_stack_buffer_);
    for (int i = 0; i < stack_size; i++)
        EXPECT_EQ(i, other[i]);
}

TEST(StackContainer, VectorDoubleDelete) {
    // Regression testing for double-delete.
    typedef StackVector<Ref<Dummy>, 2> Vector;
    Vector vect;

    int alive = 0;
    Ref<Dummy> dummy = AcquireRef(new Dummy(&alive));
    EXPECT_EQ(alive, 1);

    vect->push_back(dummy);
    EXPECT_EQ(alive, 1);

    Dummy* dummy_unref = dummy.Get();
    dummy = nullptr;
    EXPECT_EQ(alive, 1);

    auto itr = std::find(vect->begin(), vect->end(), dummy_unref);
    EXPECT_EQ(itr->Get(), dummy_unref);
    vect->erase(itr);
    EXPECT_EQ(alive, 0);

    // Shouldn't crash at exit.
}

namespace {

    template <size_t alignment>
    class AlignedData {
      public:
        AlignedData() {
            memset(data_, 0, alignment);
        }
        ~AlignedData() = default;
        alignas(alignment) char data_[alignment];
    };

}  // anonymous namespace

#define EXPECT_ALIGNED(ptr, align) EXPECT_EQ(0u, reinterpret_cast<uintptr_t>(ptr) & (align - 1))

TEST(StackContainer, BufferAlignment) {
    StackVector<wchar_t, 16> text;
    text->push_back(L'A');
    EXPECT_ALIGNED(&text[0], alignof(wchar_t));

    StackVector<double, 1> doubles;
    doubles->push_back(0.0);
    EXPECT_ALIGNED(&doubles[0], alignof(double));

    StackVector<AlignedData<16>, 1> aligned16;
    aligned16->push_back(AlignedData<16>());
    EXPECT_ALIGNED(&aligned16[0], 16);

#if !defined(DAWN_COMPILER_GCC) || defined(__x86_64__) || defined(__i386__)
    // It seems that non-X86 gcc doesn't respect greater than 16 byte alignment.
    // See http://gcc.gnu.org/bugzilla/show_bug.cgi?id=33721 for details.
    // TODO(sbc):re-enable this if GCC starts respecting higher alignments.
    StackVector<AlignedData<256>, 1> aligned256;
    aligned256->push_back(AlignedData<256>());
    EXPECT_ALIGNED(&aligned256[0], 256);
#endif
}

template class StackVector<int, 2>;
template class StackVector<Ref<Dummy>, 2>;

template <typename T, size_t size>
void CheckStackVectorElements(const StackVector<T, size>& vec, std::initializer_list<T> expected) {
    auto expected_it = expected.begin();
    EXPECT_EQ(vec->size(), expected.size());
    for (T t : vec) {
        EXPECT_NE(expected.end(), expected_it);
        EXPECT_EQ(*expected_it, t);
        ++expected_it;
    }
    EXPECT_EQ(expected.end(), expected_it);
}

TEST(StackContainer, Iteration) {
    StackVector<int, 3> vect;
    vect->push_back(7);
    vect->push_back(11);

    CheckStackVectorElements(vect, {7, 11});
    for (int& i : vect) {
        ++i;
    }
    CheckStackVectorElements(vect, {8, 12});
    vect->push_back(13);
    CheckStackVectorElements(vect, {8, 12, 13});
    vect->resize(5);
    CheckStackVectorElements(vect, {8, 12, 13, 0, 0});
    vect->resize(1);
    CheckStackVectorElements(vect, {8});
}
