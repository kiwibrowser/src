// Copyright 2019 The Dawn Authors
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

#ifndef DAWNNATIVE_ERRORINJECTOR_H_
#define DAWNNATIVE_ERRORINJECTOR_H_

#include <stdint.h>
#include <type_traits>

namespace dawn_native {

    template <typename ErrorType>
    struct InjectedErrorResult {
        ErrorType error;
        bool injected;
    };

    bool ErrorInjectorEnabled();

    bool ShouldInjectError();

    template <typename ErrorType>
    InjectedErrorResult<ErrorType> MaybeInjectError(ErrorType errorType) {
        return InjectedErrorResult<ErrorType>{errorType, ShouldInjectError()};
    }

    template <typename ErrorType, typename... ErrorTypes>
    InjectedErrorResult<ErrorType> MaybeInjectError(ErrorType errorType, ErrorTypes... errorTypes) {
        if (ShouldInjectError()) {
            return InjectedErrorResult<ErrorType>{errorType, true};
        }
        return MaybeInjectError(errorTypes...);
    }

}  // namespace dawn_native

#if defined(DAWN_ENABLE_ERROR_INJECTION)

#    define INJECT_ERROR_OR_RUN(stmt, ...)                                                   \
        [&]() {                                                                              \
            if (DAWN_UNLIKELY(::dawn_native::ErrorInjectorEnabled())) {                      \
                /* Only used for testing and fuzzing, so it's okay if this is deoptimized */ \
                auto injectedError = ::dawn_native::MaybeInjectError(__VA_ARGS__);           \
                if (injectedError.injected) {                                                \
                    return injectedError.error;                                              \
                }                                                                            \
            }                                                                                \
            return (stmt);                                                                   \
        }()

#else

#    define INJECT_ERROR_OR_RUN(stmt, ...) stmt

#endif

#endif  // DAWNNATIVE_ERRORINJECTOR_H_
