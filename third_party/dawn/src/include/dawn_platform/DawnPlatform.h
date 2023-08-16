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

#ifndef DAWNPLATFORM_DAWNPLATFORM_H_
#define DAWNPLATFORM_DAWNPLATFORM_H_

#include <dawn_native/dawn_native_export.h>

#include <stdint.h>

namespace dawn_platform {

    enum class TraceCategory {
        General,     // General trace events
        Validation,  // Dawn validation
        Recording,   // Native command recording
        GPUWork,     // Actual GPU work
    };

    class DAWN_NATIVE_EXPORT Platform {
      public:
        virtual ~Platform() {
        }
        virtual const unsigned char* GetTraceCategoryEnabledFlag(TraceCategory category) = 0;

        virtual double MonotonicallyIncreasingTime() = 0;

        virtual uint64_t AddTraceEvent(char phase,
                                       const unsigned char* categoryGroupEnabled,
                                       const char* name,
                                       uint64_t id,
                                       double timestamp,
                                       int numArgs,
                                       const char** argNames,
                                       const unsigned char* argTypes,
                                       const uint64_t* argValues,
                                       unsigned char flags) = 0;
    };

}  // namespace dawn_platform

#endif  // DAWNPLATFORM_DAWNPLATFORM_H_
