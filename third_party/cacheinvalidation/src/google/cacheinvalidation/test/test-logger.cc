// Copyright 2012 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "google/cacheinvalidation/deps/string_util.h"
#include "google/cacheinvalidation/test/test-logger.h"

namespace invalidation {

TestLogger::~TestLogger() {}

void TestLogger::Log(LogLevel level, const char* file, int line,
                     const char* format, ...) {
  va_list ap;
  va_start(ap, format);
  string result;
  StringAppendV(&result, format, ap);
  switch (level) {
    case FINE_LEVEL:
    case INFO_LEVEL:
      LogMessage(file, line, logging::LOG_INFO).stream() << result;
      break;

    case WARNING_LEVEL:
      LogMessage(file, line, logging::LOG_WARNING).stream() << result;
      break;

    case SEVERE_LEVEL:
      LogMessage(file, line, logging::LOG_ERROR).stream() << result;
      break;

    default:
      LOG(FATAL) << "unknown log level: " << level;
      break;
  }
  va_end(ap);
}

void TestLogger::SetSystemResources(SystemResources* resources) {
  // Nothing to do (logger uses no other resources).
}

}  // namespace invalidation
