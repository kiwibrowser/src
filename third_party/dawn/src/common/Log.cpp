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

#include "common/Log.h"

#include "common/Assert.h"
#include "common/Platform.h"

#include <cstdio>

#if defined(DAWN_PLATFORM_ANDROID)
#    include <android/log.h>
#endif

namespace dawn {

    namespace {

        const char* SeverityName(LogSeverity severity) {
            switch (severity) {
                case LogSeverity::Debug:
                    return "Debug";
                case LogSeverity::Info:
                    return "Info";
                case LogSeverity::Warning:
                    return "Warning";
                case LogSeverity::Error:
                    return "Error";
                default:
                    UNREACHABLE();
                    return "";
            }
        }

#if defined(DAWN_PLATFORM_ANDROID)
        android_LogPriority AndroidLogPriority(LogSeverity severity) {
            switch (severity) {
                case LogSeverity::Debug:
                    return ANDROID_LOG_INFO;
                case LogSeverity::Info:
                    return ANDROID_LOG_INFO;
                case LogSeverity::Warning:
                    return ANDROID_LOG_WARN;
                case LogSeverity::Error:
                    return ANDROID_LOG_ERROR;
                default:
                    UNREACHABLE();
                    return ANDROID_LOG_ERROR;
            }
        }
#endif  // defined(DAWN_PLATFORM_ANDROID)

    }  // anonymous namespace

    LogMessage::LogMessage(LogSeverity severity) : mSeverity(severity) {
    }

    LogMessage::~LogMessage() {
        std::string fullMessage = mStream.str();

        // If this message has been moved, its stream is empty.
        if (fullMessage.empty()) {
            return;
        }

        const char* severityName = SeverityName(mSeverity);

        FILE* outputStream = stdout;
        if (mSeverity == LogSeverity::Warning || mSeverity == LogSeverity::Error) {
            outputStream = stderr;
        }

#if defined(DAWN_PLATFORM_ANDROID)
        android_LogPriority androidPriority = AndroidLogPriority(mSeverity);
        __android_log_print(androidPriority, "Dawn", "%s: %s\n", severityName, fullMessage.c_str());
#else   // defined(DAWN_PLATFORM_ANDROID)
        // Note: we use fprintf because <iostream> includes static initializers.
        fprintf(outputStream, "%s: %s\n", severityName, fullMessage.c_str());
        fflush(outputStream);
#endif  // defined(DAWN_PLATFORM_ANDROID)
    }

    LogMessage DebugLog() {
        return {LogSeverity::Debug};
    }

    LogMessage InfoLog() {
        return {LogSeverity::Info};
    }

    LogMessage WarningLog() {
        return {LogSeverity::Warning};
    }

    LogMessage ErrorLog() {
        return {LogSeverity::Error};
    }

    LogMessage DebugLog(const char* file, const char* function, int line) {
        LogMessage message = DebugLog();
        message << file << ":" << line << "(" << function << ")";
        return message;
    }

}  // namespace dawn
