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

#ifndef COMMON_LOG_H_
#define COMMON_LOG_H_

// Dawn targets shouldn't use iostream or printf directly for several reasons:
//  - iostream adds static initializers which we want to avoid.
//  - printf and iostream don't show up in logcat on Android so printf debugging doesn't work but
//  log-message debugging does.
//  - log severity helps provide intent compared to a printf.
//
// Logging should in general be avoided: errors should go through the regular WebGPU error reporting
// mechanism and others form of logging should (TODO: eventually) go through the logging dependency
// injection, so for example they show up in Chromium's about:gpu page. Nonetheless there are some
// cases where logging is necessary and when this file was first introduced we needed to replace all
// uses of iostream so we could see them in Android's logcat.
//
// Regular logging is done using the [Debug|Info|Warning|Error]Log() function this way:
//
//   InfoLog() << things << that << ostringstream << supports; // No need for a std::endl or "\n"
//
// It creates a LogMessage object that isn't stored anywhere and gets its destructor called
// immediately which outputs the stored ostringstream in the right place.
//
// This file also contains DAWN_DEBUG for "printf debugging" which works on Android and
// additionally outputs the file, line and function name. Use it this way:
//
//   // Pepper this throughout code to get a log of the execution
//   DAWN_DEBUG();
//
//   // Get more information
//   DAWN_DEBUG() << texture.GetFormat();

#include <sstream>

namespace dawn {

    // Log levels mostly used to signal intent where the log message is produced and used to route
    // the message to the correct output.
    enum class LogSeverity {
        Debug,
        Info,
        Warning,
        Error,
    };

    // Essentially an ostringstream that will print itself in its destructor.
    class LogMessage {
      public:
        LogMessage(LogSeverity severity);
        ~LogMessage();

        LogMessage(LogMessage&& other) = default;
        LogMessage& operator=(LogMessage&& other) = default;

        template <typename T>
        LogMessage& operator<<(T&& value) {
            mStream << value;
            return *this;
        }

      private:
        LogMessage(const LogMessage& other) = delete;
        LogMessage& operator=(const LogMessage& other) = delete;

        LogSeverity mSeverity;
        std::ostringstream mStream;
    };

    // Short-hands to create a LogMessage with the respective severity.
    LogMessage DebugLog();
    LogMessage InfoLog();
    LogMessage WarningLog();
    LogMessage ErrorLog();

    // DAWN_DEBUG is a helper macro that creates a DebugLog and outputs file/line/function
    // information
    LogMessage DebugLog(const char* file, const char* function, int line);
#define DAWN_DEBUG() ::dawn::DebugLog(__FILE__, __func__, __LINE__)

}  // namespace dawn

#endif  // COMMON_LOG_H_
