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

#ifndef DAWNPLATFORM_TRACING_EVENTTRACER_H_
#define DAWNPLATFORM_TRACING_EVENTTRACER_H_

#include <stdint.h>

namespace dawn_platform {

    class Platform;
    enum class TraceCategory;

    namespace tracing {

        using TraceEventHandle = uint64_t;

        const unsigned char* GetTraceCategoryEnabledFlag(Platform* platform,
                                                         TraceCategory category);

        // TODO(enga): Simplify this API.
        TraceEventHandle AddTraceEvent(Platform* platform,
                                       char phase,
                                       const unsigned char* categoryGroupEnabled,
                                       const char* name,
                                       uint64_t id,
                                       int numArgs,
                                       const char** argNames,
                                       const unsigned char* argTypes,
                                       const uint64_t* argValues,
                                       unsigned char flags);

    }  // namespace tracing
}  // namespace dawn_platform

#endif  // DAWNPLATFORM_TRACING_EVENTTRACER_H_
