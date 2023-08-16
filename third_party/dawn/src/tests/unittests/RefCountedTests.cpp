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

#include <gtest/gtest.h>
#include <thread>

#include "common/RefCounted.h"

class RCTest : public RefCounted {
  public:
    RCTest() : RefCounted() {
    }

    RCTest(uint64_t payload) : RefCounted(payload) {
    }

    RCTest(bool* deleted) : mDeleted(deleted) {
    }

    ~RCTest() override {
        if (mDeleted != nullptr) {
            *mDeleted = true;
        }
    }

    RCTest* GetThis() {
        return this;
    }

  private:
    bool* mDeleted = nullptr;
};

struct RCTestDerived : public RCTest {
    RCTestDerived() : RCTest() {
    }

    RCTestDerived(uint64_t payload) : RCTest(payload) {
    }

    RCTestDerived(bool* deleted) : RCTest(deleted) {
    }
};

// Test that RCs start with one ref, and removing it destroys the object.
TEST(RefCounted, StartsWithOneRef) {
    bool deleted = false;
    auto test = new RCTest(&deleted);

    test->Release();
    EXPECT_TRUE(deleted);
}

// Test adding refs keep the RC alive.
TEST(RefCounted, AddingRefKeepsAlive) {
    bool deleted = false;
    auto test = new RCTest(&deleted);

    test->Reference();
    test->Release();
    EXPECT_FALSE(deleted);

    test->Release();
    EXPECT_TRUE(deleted);
}

// Test that Reference and Release atomically change the refcount.
TEST(RefCounted, RaceOnReferenceRelease) {
    bool deleted = false;
    auto* test = new RCTest(&deleted);

    auto referenceManyTimes = [test]() {
        for (uint32_t i = 0; i < 100000; ++i) {
            test->Reference();
        }
    };
    std::thread t1(referenceManyTimes);
    std::thread t2(referenceManyTimes);

    t1.join();
    t2.join();
    EXPECT_EQ(test->GetRefCountForTesting(), 200001u);

    auto releaseManyTimes = [test]() {
        for (uint32_t i = 0; i < 100000; ++i) {
            test->Release();
        }
    };

    std::thread t3(releaseManyTimes);
    std::thread t4(releaseManyTimes);
    t3.join();
    t4.join();
    EXPECT_EQ(test->GetRefCountForTesting(), 1u);

    test->Release();
    EXPECT_TRUE(deleted);
}

// Test Ref remove reference when going out of scope
TEST(Ref, EndOfScopeRemovesRef) {
    bool deleted = false;
    {
        Ref<RCTest> test(new RCTest(&deleted));
        test->Release();
    }
    EXPECT_TRUE(deleted);
}

// Test getting pointer out of the Ref
TEST(Ref, Gets) {
    RCTest* original = new RCTest;
    Ref<RCTest> test(original);
    test->Release();

    EXPECT_EQ(test.Get(), original);
    EXPECT_EQ(&*test, original);
    EXPECT_EQ(test->GetThis(), original);
}

// Test Refs default to null
TEST(Ref, DefaultsToNull) {
    Ref<RCTest> test;

    EXPECT_EQ(test.Get(), nullptr);
    EXPECT_EQ(&*test, nullptr);
    EXPECT_EQ(test->GetThis(), nullptr);
}

// Test Refs can be used inside ifs
TEST(Ref, BoolConversion) {
    Ref<RCTest> empty;
    Ref<RCTest> full(new RCTest);
    full->Release();

    if (!full || empty) {
        EXPECT_TRUE(false);
    }
}

// Test Ref's copy constructor
TEST(Ref, CopyConstructor) {
    bool deleted = false;
    RCTest* original = new RCTest(&deleted);

    Ref<RCTest> source(original);
    EXPECT_EQ(original->GetRefCountForTesting(), 2u);

    Ref<RCTest> destination(source);
    EXPECT_EQ(original->GetRefCountForTesting(), 3u);

    original->Release();
    EXPECT_EQ(original->GetRefCountForTesting(), 2u);

    EXPECT_EQ(source.Get(), original);
    EXPECT_EQ(destination.Get(), original);

    source = nullptr;
    EXPECT_FALSE(deleted);
    EXPECT_EQ(original->GetRefCountForTesting(), 1u);

    destination = nullptr;
    EXPECT_TRUE(deleted);
}

// Test Ref's copy assignment
TEST(Ref, CopyAssignment) {
    bool deleted = false;
    RCTest* original = new RCTest(&deleted);

    Ref<RCTest> source(original);
    original->Release();

    Ref<RCTest> destination;
    destination = source;

    EXPECT_EQ(source.Get(), original);
    EXPECT_EQ(destination.Get(), original);

    source = nullptr;
    // This fails when address sanitizer is turned on
    EXPECT_FALSE(deleted);

    destination = nullptr;
    EXPECT_TRUE(deleted);
}

// Test Ref's move constructor
TEST(Ref, MoveConstructor) {
    bool deleted = false;
    RCTest* original = new RCTest(&deleted);

    Ref<RCTest> source(original);
    EXPECT_EQ(original->GetRefCountForTesting(), 2u);

    Ref<RCTest> destination(std::move(source));
    EXPECT_EQ(original->GetRefCountForTesting(), 2u);

    original->Release();
    EXPECT_EQ(original->GetRefCountForTesting(), 1u);

    EXPECT_EQ(source.Get(), nullptr);
    EXPECT_EQ(destination.Get(), original);
    EXPECT_FALSE(deleted);

    destination = nullptr;
    EXPECT_TRUE(deleted);
}

// Test Ref's move assignment
TEST(Ref, MoveAssignment) {
    bool deleted = false;
    RCTest* original = new RCTest(&deleted);

    Ref<RCTest> source(original);
    EXPECT_EQ(original->GetRefCountForTesting(), 2u);

    original->Release();
    EXPECT_EQ(original->GetRefCountForTesting(), 1u);

    Ref<RCTest> destination;
    destination = std::move(source);
    EXPECT_EQ(original->GetRefCountForTesting(), 1u);

    EXPECT_EQ(source.Get(), nullptr);
    EXPECT_EQ(destination.Get(), original);
    EXPECT_FALSE(deleted);

    destination = nullptr;
    EXPECT_TRUE(deleted);
}

// Test move assigment where the destination and source
// point to the same underlying object.
TEST(Ref, MoveAssignmentSameObject) {
    bool deleted = false;
    RCTest* original = new RCTest(&deleted);

    Ref<RCTest> source(original);
    EXPECT_EQ(original->GetRefCountForTesting(), 2u);

    original->Release();
    EXPECT_EQ(original->GetRefCountForTesting(), 1u);

    Ref<RCTest>& referenceToSource = source;
    EXPECT_EQ(original->GetRefCountForTesting(), 1u);

    referenceToSource = std::move(source);

    EXPECT_EQ(source.Get(), original);
    EXPECT_EQ(original->GetRefCountForTesting(), 1u);
    EXPECT_FALSE(deleted);

    source = nullptr;
    EXPECT_TRUE(deleted);
}

// Test the payload initial value is set correctly
TEST(Ref, InitialPayloadValue) {
    RCTest* testDefaultConstructor = new RCTest();
    EXPECT_EQ(testDefaultConstructor->GetRefCountPayload(), 0u);
    testDefaultConstructor->Release();

    RCTest* testZero = new RCTest(uint64_t(0ull));
    EXPECT_EQ(testZero->GetRefCountPayload(), 0u);
    testZero->Release();

    RCTest* testOne = new RCTest(1ull);
    EXPECT_EQ(testOne->GetRefCountPayload(), 1u);
    testOne->Release();
}

// Test that the payload survives ref and release operations
TEST(Ref, PayloadUnchangedByRefCounting) {
    RCTest* test = new RCTest(1ull);
    EXPECT_EQ(test->GetRefCountPayload(), 1u);

    test->Reference();
    EXPECT_EQ(test->GetRefCountPayload(), 1u);
    test->Release();
    EXPECT_EQ(test->GetRefCountPayload(), 1u);

    test->Release();
}

// Test that Detach pulls out the pointer and stops tracking it.
TEST(Ref, Detach) {
    bool deleted = false;
    RCTest* original = new RCTest(&deleted);

    Ref<RCTest> test(original);
    original->Release();

    RCTest* detached = test.Detach();
    EXPECT_EQ(detached, original);
    EXPECT_EQ(detached->GetRefCountForTesting(), 1u);
    EXPECT_EQ(test.Get(), nullptr);

    detached->Release();
    EXPECT_TRUE(deleted);
}

// Test constructor passed a derived pointer
TEST(Ref, DerivedPointerConstructor) {
    bool deleted = false;
    {
        Ref<RCTest> test(new RCTestDerived(&deleted));
        test->Release();
    }
    EXPECT_TRUE(deleted);
}

// Test copy constructor of derived class
TEST(Ref, DerivedCopyConstructor) {
    bool deleted = false;
    Ref<RCTestDerived> testDerived(new RCTestDerived(&deleted));
    testDerived->Release();

    {
        Ref<RCTest> testBase(testDerived);
        EXPECT_EQ(testBase->GetRefCountForTesting(), 2u);
        EXPECT_EQ(testDerived->GetRefCountForTesting(), 2u);
    }

    EXPECT_EQ(testDerived->GetRefCountForTesting(), 1u);
}

// Test Ref constructed with nullptr
TEST(Ref, ConstructedWithNullptr) {
    Ref<RCTest> test(nullptr);
    EXPECT_EQ(test.Get(), nullptr);
}

// Test Ref's copy assignment with derived class
TEST(Ref, CopyAssignmentDerived) {
    bool deleted = false;

    RCTestDerived* original = new RCTestDerived(&deleted);
    Ref<RCTestDerived> source(original);
    original->Release();
    EXPECT_EQ(original->GetRefCountForTesting(), 1u);

    Ref<RCTest> destination;
    destination = source;
    EXPECT_EQ(original->GetRefCountForTesting(), 2u);

    EXPECT_EQ(source.Get(), original);
    EXPECT_EQ(destination.Get(), original);

    source = nullptr;
    EXPECT_EQ(original->GetRefCountForTesting(), 1u);
    EXPECT_FALSE(deleted);

    destination = nullptr;
    EXPECT_TRUE(deleted);
}

// Test Ref's move constructor with derived class
TEST(Ref, MoveConstructorDerived) {
    bool deleted = false;
    RCTestDerived* original = new RCTestDerived(&deleted);

    Ref<RCTestDerived> source(original);
    EXPECT_EQ(original->GetRefCountForTesting(), 2u);

    Ref<RCTest> destination(std::move(source));
    EXPECT_EQ(original->GetRefCountForTesting(), 2u);

    original->Release();
    EXPECT_EQ(original->GetRefCountForTesting(), 1u);

    EXPECT_EQ(source.Get(), nullptr);
    EXPECT_EQ(destination.Get(), original);
    EXPECT_FALSE(deleted);

    destination = nullptr;
    EXPECT_TRUE(deleted);
}

// Test Ref's move assignment with derived class
TEST(Ref, MoveAssignmentDerived) {
    bool deleted = false;
    RCTestDerived* original = new RCTestDerived(&deleted);

    Ref<RCTestDerived> source(original);
    EXPECT_EQ(original->GetRefCountForTesting(), 2u);

    original->Release();
    EXPECT_EQ(original->GetRefCountForTesting(), 1u);

    Ref<RCTest> destination;
    destination = std::move(source);

    EXPECT_EQ(original->GetRefCountForTesting(), 1u);

    EXPECT_EQ(source.Get(), nullptr);
    EXPECT_EQ(destination.Get(), original);
    EXPECT_FALSE(deleted);

    destination = nullptr;
    EXPECT_TRUE(deleted);
}
