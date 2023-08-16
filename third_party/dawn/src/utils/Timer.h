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

#ifndef UTILS_TIMER_H_
#define UTILS_TIMER_H_

namespace utils {

    class Timer {
      public:
        virtual ~Timer() {
        }

        // Timer functionality: Use start() and stop() to record the duration and use
        // getElapsedTime() to query that duration.  If getElapsedTime() is called in between, it
        // will report the elapsed time since start().
        virtual void Start() = 0;
        virtual void Stop() = 0;
        virtual double GetElapsedTime() const = 0;

        // Timestamp functionality: Use getAbsoluteTime() to get an absolute time with an unknown
        // origin. This time moves forward regardless of start()/stop().
        virtual double GetAbsoluteTime() = 0;
    };

    Timer* CreateTimer();

}  // namespace utils

#endif  // UTILS_TIMER_H_
