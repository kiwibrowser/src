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

#include <gtest/gtest.h>

#include "common/RefCounted.h"
#include "common/Result.h"

namespace {

    template <typename T, typename E>
    void TestError(Result<T, E>* result, E expectedError) {
        EXPECT_TRUE(result->IsError());
        EXPECT_FALSE(result->IsSuccess());

        std::unique_ptr<E> storedError = result->AcquireError();
        EXPECT_EQ(*storedError, expectedError);
    }

    template <typename T, typename E>
    void TestSuccess(Result<T, E>* result, T expectedSuccess) {
        EXPECT_FALSE(result->IsError());
        EXPECT_TRUE(result->IsSuccess());

        const T storedSuccess = result->AcquireSuccess();
        EXPECT_EQ(storedSuccess, expectedSuccess);

        // Once the success is acquired, result has an empty
        // payload and is neither in the success nor error state.
        EXPECT_FALSE(result->IsError());
        EXPECT_FALSE(result->IsSuccess());
    }

    static int dummyError = 0xbeef;
    static float dummySuccess = 42.0f;
    static const float dummyConstSuccess = 42.0f;

    class AClass : public RefCounted {
      public:
        int a = 0;
    };

    // Tests using the following overload of TestSuccess make
    // local Ref instances to dummySuccessObj. Tests should
    // ensure any local Ref objects made along the way continue
    // to point to dummySuccessObj.
    template <typename T, typename E>
    void TestSuccess(Result<Ref<T>, E>* result, T* expectedSuccess) {
        EXPECT_FALSE(result->IsError());
        EXPECT_TRUE(result->IsSuccess());

        // AClass starts with a reference count of 1 and stored
        // on the stack in the caller. The result parameter should
        // hold the only other reference to the object.
        EXPECT_EQ(expectedSuccess->GetRefCountForTesting(), 2u);

        const Ref<T> storedSuccess = result->AcquireSuccess();
        EXPECT_EQ(storedSuccess.Get(), expectedSuccess);

        // Once the success is acquired, result has an empty
        // payload and is neither in the success nor error state.
        EXPECT_FALSE(result->IsError());
        EXPECT_FALSE(result->IsSuccess());

        // Once we call AcquireSuccess, result no longer stores
        // the object. storedSuccess should contain the only other
        // reference to the object.
        EXPECT_EQ(storedSuccess->GetRefCountForTesting(), 2u);
    }

    // Result<void, E*>

    // Test constructing an error Result<void, E>
    TEST(ResultOnlyPointerError, ConstructingError) {
        Result<void, int> result(std::make_unique<int>(dummyError));
        TestError(&result, dummyError);
    }

    // Test moving an error Result<void, E>
    TEST(ResultOnlyPointerError, MovingError) {
        Result<void, int> result(std::make_unique<int>(dummyError));
        Result<void, int> movedResult(std::move(result));
        TestError(&movedResult, dummyError);
    }

    // Test returning an error Result<void, E>
    TEST(ResultOnlyPointerError, ReturningError) {
        auto CreateError = []() -> Result<void, int> {
            return {std::make_unique<int>(dummyError)};
        };

        Result<void, int> result = CreateError();
        TestError(&result, dummyError);
    }

    // Test constructing a success Result<void, E>
    TEST(ResultOnlyPointerError, ConstructingSuccess) {
        Result<void, int> result;
        EXPECT_TRUE(result.IsSuccess());
        EXPECT_FALSE(result.IsError());
    }

    // Test moving a success Result<void, E>
    TEST(ResultOnlyPointerError, MovingSuccess) {
        Result<void, int> result;
        Result<void, int> movedResult(std::move(result));
        EXPECT_TRUE(movedResult.IsSuccess());
        EXPECT_FALSE(movedResult.IsError());
    }

    // Test returning a success Result<void, E>
    TEST(ResultOnlyPointerError, ReturningSuccess) {
        auto CreateError = []() -> Result<void, int> { return {}; };

        Result<void, int> result = CreateError();
        EXPECT_TRUE(result.IsSuccess());
        EXPECT_FALSE(result.IsError());
    }

    // Result<T*, E*>

    // Test constructing an error Result<T*, E>
    TEST(ResultBothPointer, ConstructingError) {
        Result<float*, int> result(std::make_unique<int>(dummyError));
        TestError(&result, dummyError);
    }

    // Test moving an error Result<T*, E>
    TEST(ResultBothPointer, MovingError) {
        Result<float*, int> result(std::make_unique<int>(dummyError));
        Result<float*, int> movedResult(std::move(result));
        TestError(&movedResult, dummyError);
    }

    // Test returning an error Result<T*, E>
    TEST(ResultBothPointer, ReturningError) {
        auto CreateError = []() -> Result<float*, int> {
            return {std::make_unique<int>(dummyError)};
        };

        Result<float*, int> result = CreateError();
        TestError(&result, dummyError);
    }

    // Test constructing a success Result<T*, E>
    TEST(ResultBothPointer, ConstructingSuccess) {
        Result<float*, int> result(&dummySuccess);
        TestSuccess(&result, &dummySuccess);
    }

    // Test moving a success Result<T*, E>
    TEST(ResultBothPointer, MovingSuccess) {
        Result<float*, int> result(&dummySuccess);
        Result<float*, int> movedResult(std::move(result));
        TestSuccess(&movedResult, &dummySuccess);
    }

    // Test returning a success Result<T*, E>
    TEST(ResultBothPointer, ReturningSuccess) {
        auto CreateSuccess = []() -> Result<float*, int*> { return {&dummySuccess}; };

        Result<float*, int*> result = CreateSuccess();
        TestSuccess(&result, &dummySuccess);
    }

    // Tests converting from a Result<TChild*, E>
    TEST(ResultBothPointer, ConversionFromChildClass) {
        struct T {
            int a;
        };
        struct TChild : T {};

        TChild child;
        T* childAsT = &child;
        {
            Result<T*, int> result(&child);
            TestSuccess(&result, childAsT);
        }
        {
            Result<TChild*, int> resultChild(&child);
            Result<T*, int> result(std::move(resultChild));
            TestSuccess(&result, childAsT);
        }
        {
            Result<TChild*, int> resultChild(&child);
            Result<T*, int> result = std::move(resultChild);
            TestSuccess(&result, childAsT);
        }
    }

    // Result<const T*, E>

    // Test constructing an error Result<const T*, E>
    TEST(ResultBothPointerWithConstResult, ConstructingError) {
        Result<const float*, int> result(std::make_unique<int>(dummyError));
        TestError(&result, dummyError);
    }

    // Test moving an error Result<const T*, E>
    TEST(ResultBothPointerWithConstResult, MovingError) {
        Result<const float*, int> result(std::make_unique<int>(dummyError));
        Result<const float*, int> movedResult(std::move(result));
        TestError(&movedResult, dummyError);
    }

    // Test returning an error Result<const T*, E*>
    TEST(ResultBothPointerWithConstResult, ReturningError) {
        auto CreateError = []() -> Result<const float*, int> {
            return {std::make_unique<int>(dummyError)};
        };

        Result<const float*, int> result = CreateError();
        TestError(&result, dummyError);
    }

    // Test constructing a success Result<const T*, E*>
    TEST(ResultBothPointerWithConstResult, ConstructingSuccess) {
        Result<const float*, int> result(&dummyConstSuccess);
        TestSuccess(&result, &dummyConstSuccess);
    }

    // Test moving a success Result<const T*, E*>
    TEST(ResultBothPointerWithConstResult, MovingSuccess) {
        Result<const float*, int> result(&dummyConstSuccess);
        Result<const float*, int> movedResult(std::move(result));
        TestSuccess(&movedResult, &dummyConstSuccess);
    }

    // Test returning a success Result<const T*, E*>
    TEST(ResultBothPointerWithConstResult, ReturningSuccess) {
        auto CreateSuccess = []() -> Result<const float*, int> { return {&dummyConstSuccess}; };

        Result<const float*, int> result = CreateSuccess();
        TestSuccess(&result, &dummyConstSuccess);
    }

    // Result<Ref<T>, E>

    // Test constructing an error Result<Ref<T>, E>
    TEST(ResultRefT, ConstructingError) {
        Result<Ref<AClass>, int> result(std::make_unique<int>(dummyError));
        TestError(&result, dummyError);
    }

    // Test moving an error Result<Ref<T>, E>
    TEST(ResultRefT, MovingError) {
        Result<Ref<AClass>, int> result(std::make_unique<int>(dummyError));
        Result<Ref<AClass>, int> movedResult(std::move(result));
        TestError(&movedResult, dummyError);
    }

    // Test returning an error Result<Ref<T>, E>
    TEST(ResultRefT, ReturningError) {
        auto CreateError = []() -> Result<Ref<AClass>, int> {
            return {std::make_unique<int>(dummyError)};
        };

        Result<Ref<AClass>, int> result = CreateError();
        TestError(&result, dummyError);
    }

    // Test constructing a success Result<Ref<T>, E>
    TEST(ResultRefT, ConstructingSuccess) {
        AClass success;

        Ref<AClass> refObj(&success);
        Result<Ref<AClass>, int> result(std::move(refObj));
        TestSuccess(&result, &success);
    }

    // Test moving a success Result<Ref<T>, E>
    TEST(ResultRefT, MovingSuccess) {
        AClass success;

        Ref<AClass> refObj(&success);
        Result<Ref<AClass>, int> result(std::move(refObj));
        Result<Ref<AClass>, int> movedResult(std::move(result));
        TestSuccess(&movedResult, &success);
    }

    // Test returning a success Result<Ref<T>, E>
    TEST(ResultRefT, ReturningSuccess) {
        AClass success;
        auto CreateSuccess = [&success]() -> Result<Ref<AClass>, int> {
            return Ref<AClass>(&success);
        };

        Result<Ref<AClass>, int> result = CreateSuccess();
        TestSuccess(&result, &success);
    }

    class OtherClass {
      public:
        int a = 0;
    };
    class Base : public RefCounted {};
    class Child : public OtherClass, public Base {};

    // Test constructing a Result<Ref<TChild>, E>
    TEST(ResultRefT, ConversionFromChildConstructor) {
        Child child;
        Ref<Child> refChild(&child);

        Result<Ref<Base>, int> result(std::move(refChild));
        TestSuccess<Base>(&result, &child);
    }

    // Test copy constructing Result<Ref<TChild>, E>
    TEST(ResultRefT, ConversionFromChildCopyConstructor) {
        Child child;
        Ref<Child> refChild(&child);

        Result<Ref<Child>, int> resultChild(std::move(refChild));
        Result<Ref<Base>, int> result(std::move(resultChild));
        TestSuccess<Base>(&result, &child);
    }

    // Test assignment operator for Result<Ref<TChild>, E>
    TEST(ResultRefT, ConversionFromChildAssignmentOperator) {
        Child child;
        Ref<Child> refChild(&child);

        Result<Ref<Child>, int> resultChild(std::move(refChild));
        Result<Ref<Base>, int> result = std::move(resultChild);
        TestSuccess<Base>(&result, &child);
    }

    // Result<T, E>

    // Test constructing an error Result<T, E>
    TEST(ResultGeneric, ConstructingError) {
        Result<std::vector<float>, int> result(std::make_unique<int>(dummyError));
        TestError(&result, dummyError);
    }

    // Test moving an error Result<T, E>
    TEST(ResultGeneric, MovingError) {
        Result<std::vector<float>, int> result(std::make_unique<int>(dummyError));
        Result<std::vector<float>, int> movedResult(std::move(result));
        TestError(&movedResult, dummyError);
    }

    // Test returning an error Result<T, E>
    TEST(ResultGeneric, ReturningError) {
        auto CreateError = []() -> Result<std::vector<float>, int> {
            return {std::make_unique<int>(dummyError)};
        };

        Result<std::vector<float>, int> result = CreateError();
        TestError(&result, dummyError);
    }

    // Test constructing a success Result<T, E>
    TEST(ResultGeneric, ConstructingSuccess) {
        Result<std::vector<float>, int> result({1.0f});
        TestSuccess(&result, {1.0f});
    }

    // Test moving a success Result<T, E>
    TEST(ResultGeneric, MovingSuccess) {
        Result<std::vector<float>, int> result({1.0f});
        Result<std::vector<float>, int> movedResult(std::move(result));
        TestSuccess(&movedResult, {1.0f});
    }

    // Test returning a success Result<T, E>
    TEST(ResultGeneric, ReturningSuccess) {
        auto CreateSuccess = []() -> Result<std::vector<float>, int> { return {{1.0f}}; };

        Result<std::vector<float>, int> result = CreateSuccess();
        TestSuccess(&result, {1.0f});
    }

}  // anonymous namespace
