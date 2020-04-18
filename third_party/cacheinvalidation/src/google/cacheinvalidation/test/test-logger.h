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

#ifndef GOOGLE_CACHEINVALIDATION_TEST_TEST_LOGGER_H_
#define GOOGLE_CACHEINVALIDATION_TEST_TEST_LOGGER_H_

#include "google/cacheinvalidation/include/system-resources.h"
#include "google/cacheinvalidation/deps/logging.h"

namespace invalidation {

// A simple logger implementation for testing.
class TestLogger : public Logger {
 public:
  virtual ~TestLogger();

  // Overrides from Logger.
  virtual void Log(LogLevel level, const char* file, int line,
                   const char* format, ...);

  virtual void SetSystemResources(SystemResources* resources);
};

}  // namespace invalidation

#endif  // GOOGLE_CACHEINVALIDATION_TEST_TEST_LOGGER_H_
