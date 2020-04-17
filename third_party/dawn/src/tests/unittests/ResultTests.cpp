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

#include "common/Result.h"

namespace {

template<typename T, typename E>
void TestError(Result<T, E>* result, E expectedError) {
    ASSERT_TRUE(result->IsError());
    ASSERT_FALSE(result->IsSuccess());

    E storedError = result->AcquireError();
    ASSERT_EQ(storedError, expectedError);
}

template<typename T, typename E>
void TestSuccess(Result<T, E>* result, T expectedSuccess) {
    ASSERT_FALSE(result->IsError());
    ASSERT_TRUE(result->IsSuccess());

    T storedSuccess = result->AcquireSuccess();
    ASSERT_EQ(storedSuccess, expectedSuccess);
}

static int dummyError = 0xbeef;
static float dummySuccess = 42.0f;

// Result<void, E*>

// Test constructing an error Result<void, E*>
TEST(ResultOnlyPointerError, ConstructingError) {
    Result<void, int*> result(&dummyError);
    TestError(&result, &dummyError);
}

// Test moving an error Result<void, E*>
TEST(ResultOnlyPointerError, MovingError) {
    Result<void, int*> result(&dummyError);
    Result<void, int*> movedResult(std::move(result));
    TestError(&movedResult, &dummyError);
}

// Test returning an error Result<void, E*>
TEST(ResultOnlyPointerError, ReturningError) {
    auto CreateError = []() -> Result<void, int*> {
        return {&dummyError};
    };

    Result<void, int*> result = CreateError();
    TestError(&result, &dummyError);
}

// Test constructing a success Result<void, E*>
TEST(ResultOnlyPointerError, ConstructingSuccess) {
    Result<void, int*> result;
    ASSERT_TRUE(result.IsSuccess());
    ASSERT_FALSE(result.IsError());
}

// Test moving a success Result<void, E*>
TEST(ResultOnlyPointerError, MovingSuccess) {
    Result<void, int*> result;
    Result<void, int*> movedResult(std::move(result));
    ASSERT_TRUE(movedResult.IsSuccess());
    ASSERT_FALSE(movedResult.IsError());
}

// Test returning a success Result<void, E*>
TEST(ResultOnlyPointerError, ReturningSuccess) {
    auto CreateError = []() -> Result<void, int*> {
        return {};
    };

    Result<void, int*> result = CreateError();
    ASSERT_TRUE(result.IsSuccess());
    ASSERT_FALSE(result.IsError());
}

// Result<T*, E*>

// Test constructing an error Result<T*, E*>
TEST(ResultBothPointer, ConstructingError) {
    Result<float*, int*> result(&dummyError);
    TestError(&result, &dummyError);
}

// Test moving an error Result<T*, E*>
TEST(ResultBothPointer, MovingError) {
    Result<float*, int*> result(&dummyError);
    Result<float*, int*> movedResult(std::move(result));
    TestError(&movedResult, &dummyError);
}

// Test returning an error Result<T*, E*>
TEST(ResultBothPointer, ReturningError) {
    auto CreateError = []() -> Result<float*, int*> {
        return {&dummyError};
    };

    Result<float*, int*> result = CreateError();
    TestError(&result, &dummyError);
}

// Test constructing a success Result<T*, E*>
TEST(ResultBothPointer, ConstructingSuccess) {
    Result<float*, int*> result(&dummySuccess);
    TestSuccess(&result, &dummySuccess);
}

// Test moving a success Result<T*, E*>
TEST(ResultBothPointer, MovingSuccess) {
    Result<float*, int*> result(&dummySuccess);
    Result<float*, int*> movedResult(std::move(result));
    TestSuccess(&movedResult, &dummySuccess);
}

// Test returning a success Result<T*, E*>
TEST(ResultBothPointer, ReturningSuccess) {
    auto CreateSuccess = []() -> Result<float*, int*> {
        return {&dummySuccess};
    };

    Result<float*, int*> result = CreateSuccess();
    TestSuccess(&result, &dummySuccess);
}

// Result<T, E>

// Test constructing an error Result<T, E>
TEST(ResultGeneric, ConstructingError) {
    Result<std::vector<float>, int*> result(&dummyError);
    TestError(&result, &dummyError);
}

// Test moving an error Result<T, E>
TEST(ResultGeneric, MovingError) {
    Result<std::vector<float>, int*> result(&dummyError);
    Result<std::vector<float>, int*> movedResult(std::move(result));
    TestError(&movedResult, &dummyError);
}

// Test returning an error Result<T, E>
TEST(ResultGeneric, ReturningError) {
    auto CreateError = []() -> Result<std::vector<float>, int*> {
        return {&dummyError};
    };

    Result<std::vector<float>, int*> result = CreateError();
    TestError(&result, &dummyError);
}

// Test constructing a success Result<T, E>
TEST(ResultGeneric, ConstructingSuccess) {
    Result<std::vector<float>, int*> result({1.0f});
    TestSuccess(&result, {1.0f});
}

// Test moving a success Result<T, E>
TEST(ResultGeneric, MovingSuccess) {
    Result<std::vector<float>, int*> result({1.0f});
    Result<std::vector<float>, int*> movedResult(std::move(result));
    TestSuccess(&movedResult, {1.0f});
}

// Test returning a success Result<T, E>
TEST(ResultGeneric, ReturningSuccess) {
    auto CreateSuccess = []() -> Result<std::vector<float>, int*> {
        return {{1.0f}};
    };

    Result<std::vector<float>, int*> result = CreateSuccess();
    TestSuccess(&result, {1.0f});
}

}  // anonymous namespace
