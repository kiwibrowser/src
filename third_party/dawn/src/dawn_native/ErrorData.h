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

#include <cstdint>
#include <string>
#include <vector>

namespace dawn_native {

    enum class ErrorType : uint32_t;

    class ErrorData {
      public:
        ErrorData();
        ErrorData(ErrorType type, std::string message);

        struct BacktraceRecord {
            const char* file;
            const char* function;
            int line;
        };
        void AppendBacktrace(const char* file, const char* function, int line);

        ErrorType GetType() const;
        const std::string& GetMessage() const;
        const std::vector<BacktraceRecord>& GetBacktrace() const;

      private:
        ErrorType mType;
        std::string mMessage;
        std::vector<BacktraceRecord> mBacktrace;
    };

}  // namespace dawn_native

#endif  // DAWNNATIVE_ERRORDATA_H_
