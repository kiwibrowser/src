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

#ifndef DAWNNATIVE_ERROR_H_
#define DAWNNATIVE_ERROR_H_

#include "common/Result.h"

#include <string>

namespace dawn_native {

    // This is the content of an error value for MaybeError or ResultOrError, split off to its own
    // file to avoid having all files including headers like <string> and <vector>
    class ErrorData;

    enum class ErrorType : uint32_t { Validation, ContextLost, Unimplemented };

    // MaybeError and ResultOrError are meant to be used as return value for function that are not
    // expected to, but might fail. The handling of error is potentially much slower than successes.
    using MaybeError = Result<void, ErrorData*>;

    template <typename T>
    using ResultOrError = Result<T, ErrorData*>;

    // Returning a success is done like so:
    //   return {}; // for Error
    //   return SomethingOfTypeT; // for ResultOrError<T>
    //
    // Returning an error is done via:
    //   return DAWN_MAKE_ERROR(errorType, "My error message");
    //
    // but shorthand version for specific error types are preferred:
    //   return DAWN_VALIDATION_ERROR("My error message");
#define DAWN_MAKE_ERROR(TYPE, MESSAGE) \
    ::dawn_native::MakeError(TYPE, MESSAGE, __FILE__, __func__, __LINE__)
#define DAWN_VALIDATION_ERROR(MESSAGE) DAWN_MAKE_ERROR(ErrorType::Validation, MESSAGE)
#define DAWN_CONTEXT_LOST_ERROR(MESSAGE) DAWN_MAKE_ERROR(ErrorType::ContextLost, MESSAGE)
#define DAWN_UNIMPLEMENTED_ERROR(MESSAGE) DAWN_MAKE_ERROR(ErrorType::Unimplemented, MESSAGE)

#define DAWN_CONCAT1(x, y) x##y
#define DAWN_CONCAT2(x, y) DAWN_CONCAT1(x, y)
#define DAWN_LOCAL_VAR DAWN_CONCAT2(_localVar, __LINE__)

    // When Errors aren't handled explicitly, calls to functions returning errors should be
    // wrapped in an DAWN_TRY. It will return the error if any, otherwise keep executing
    // the current function.
#define DAWN_TRY(EXPR)                                                           \
    {                                                                            \
        auto DAWN_LOCAL_VAR = EXPR;                                              \
        if (DAWN_UNLIKELY(DAWN_LOCAL_VAR.IsError())) {                           \
            ErrorData* error = DAWN_LOCAL_VAR.AcquireError();                    \
            ::dawn_native::AppendBacktrace(error, __FILE__, __func__, __LINE__); \
            return {std::move(error)};                                           \
        }                                                                        \
    }                                                                            \
    for (;;)                                                                     \
    break

    // DAWN_TRY_ASSIGN is the same as DAWN_TRY for ResultOrError and assigns the success value, if
    // any, to VAR.
#define DAWN_TRY_ASSIGN(VAR, EXPR)                                               \
    {                                                                            \
        auto DAWN_LOCAL_VAR = EXPR;                                              \
        if (DAWN_UNLIKELY(DAWN_LOCAL_VAR.IsError())) {                           \
            ErrorData* error = DAWN_LOCAL_VAR.AcquireError();                    \
            ::dawn_native::AppendBacktrace(error, __FILE__, __func__, __LINE__); \
            return {std::move(error)};                                           \
        }                                                                        \
        VAR = DAWN_LOCAL_VAR.AcquireSuccess();                                   \
    }                                                                            \
    for (;;)                                                                     \
    break

    // Implementation detail of DAWN_TRY and DAWN_TRY_ASSIGN's adding to the Error's backtrace.
    void AppendBacktrace(ErrorData* error, const char* file, const char* function, int line);

    // Implementation detail of DAWN_MAKE_ERROR
    ErrorData* MakeError(ErrorType type,
                         std::string message,
                         const char* file,
                         const char* function,
                         int line);

}  // namespace dawn_native

#endif  // DAWNNATIVE_ERROR_H_
