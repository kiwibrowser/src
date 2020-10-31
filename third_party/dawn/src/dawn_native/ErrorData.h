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

#ifndef DAWNNATIVE_ERRORDATA_H_
#define DAWNNATIVE_ERRORDATA_H_

#include "common/Compiler.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace wgpu {
    enum class ErrorType : uint32_t;
}

namespace dawn {
    using ErrorType = wgpu::ErrorType;
}

namespace dawn_native {
    enum class InternalErrorType : uint32_t;

    class DAWN_NO_DISCARD ErrorData {
      public:
        static DAWN_NO_DISCARD std::unique_ptr<ErrorData> Create(InternalErrorType type,
                                                                 std::string message,
                                                                 const char* file,
                                                                 const char* function,
                                                                 int line);
        ErrorData(InternalErrorType type, std::string message);

        struct BacktraceRecord {
            const char* file;
            const char* function;
            int line;
        };
        void AppendBacktrace(const char* file, const char* function, int line);

        InternalErrorType GetType() const;
        const std::string& GetMessage() const;
        const std::vector<BacktraceRecord>& GetBacktrace() const;

      private:
        InternalErrorType mType;
        std::string mMessage;
        std::vector<BacktraceRecord> mBacktrace;
    };

}  // namespace dawn_native

#endif  // DAWNNATIVE_ERRORDATA_H_
