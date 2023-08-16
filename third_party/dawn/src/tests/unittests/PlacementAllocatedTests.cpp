// Copyright 2020 The Dawn Authors
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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "common/PlacementAllocated.h"

using namespace testing;

namespace {

    enum class DestructedClass {
        Foo,
        Bar,
    };

    class MockDestructor {
      public:
        MOCK_METHOD(void, Call, (void*, DestructedClass));
    };

    std::unique_ptr<StrictMock<MockDestructor>> mockDestructor;

    class PlacementAllocatedTests : public Test {
        void SetUp() override {
            mockDestructor = std::make_unique<StrictMock<MockDestructor>>();
        }

        void TearDown() override {
            mockDestructor = nullptr;
        }
    };

    struct Foo : PlacementAllocated {
        virtual ~Foo() {
            mockDestructor->Call(this, DestructedClass::Foo);
        }
    };

    struct Bar : Foo {
        ~Bar() override {
            mockDestructor->Call(this, DestructedClass::Bar);
        }
    };
}  // namespace

// Test that deletion calls the destructor and does not free memory.
TEST_F(PlacementAllocatedTests, DeletionDoesNotFreeMemory) {
    void* ptr = malloc(sizeof(Foo));

    Foo* foo = new (ptr) Foo();

    EXPECT_CALL(*mockDestructor, Call(foo, DestructedClass::Foo));
    delete foo;

    // Touch the memory, this shouldn't crash.
    static_assert(sizeof(Foo) >= sizeof(uint32_t), "");
    *reinterpret_cast<uint32_t*>(foo) = 42;

    free(ptr);
}

// Test that destructing an instance of a derived class calls the derived, then base destructor, and
// does not free memory.
TEST_F(PlacementAllocatedTests, DeletingDerivedClassCallsBaseDestructor) {
    void* ptr = malloc(sizeof(Bar));

    Bar* bar = new (ptr) Bar();

    {
        InSequence s;
        EXPECT_CALL(*mockDestructor, Call(bar, DestructedClass::Bar));
        EXPECT_CALL(*mockDestructor, Call(bar, DestructedClass::Foo));
        delete bar;
    }

    // Touch the memory, this shouldn't crash.
    static_assert(sizeof(Bar) >= sizeof(uint32_t), "");
    *reinterpret_cast<uint32_t*>(bar) = 42;

    free(ptr);
}

// Test that destructing an instance of a base class calls the derived, then base destructor, and
// does not free memory.
TEST_F(PlacementAllocatedTests, DeletingBaseClassCallsDerivedDestructor) {
    void* ptr = malloc(sizeof(Bar));

    Foo* foo = new (ptr) Bar();

    {
        InSequence s;
        EXPECT_CALL(*mockDestructor, Call(foo, DestructedClass::Bar));
        EXPECT_CALL(*mockDestructor, Call(foo, DestructedClass::Foo));
        delete foo;
    }

    // Touch the memory, this shouldn't crash.
    static_assert(sizeof(Bar) >= sizeof(uint32_t), "");
    *reinterpret_cast<uint32_t*>(foo) = 42;

    free(ptr);
}
