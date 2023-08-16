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

#include "utils/Timer.h"

#include <stdint.h>
#include <time.h>

namespace utils {

    namespace {

        uint64_t GetCurrentTimeNs() {
            struct timespec currentTime;
            clock_gettime(CLOCK_MONOTONIC, &currentTime);
            return currentTime.tv_sec * 1'000'000'000llu + currentTime.tv_nsec;
        }

    }  // anonymous namespace

    class PosixTimer : public Timer {
      public:
        PosixTimer() : Timer(), mRunning(false) {
        }

        ~PosixTimer() override = default;

        void Start() override {
            mStartTimeNs = GetCurrentTimeNs();
            mRunning = true;
        }

        void Stop() override {
            mStopTimeNs = GetCurrentTimeNs();
            mRunning = false;
        }

        double GetElapsedTime() const override {
            uint64_t endTimeNs;
            if (mRunning) {
                endTimeNs = GetCurrentTimeNs();
            } else {
                endTimeNs = mStopTimeNs;
            }

            return (endTimeNs - mStartTimeNs) * 1e-9;
        }

        double GetAbsoluteTime() override {
            return GetCurrentTimeNs() * 1e-9;
        }

      private:
        bool mRunning;
        uint64_t mStartTimeNs;
        uint64_t mStopTimeNs;
    };

    Timer* CreateTimer() {
        return new PosixTimer();
    }

}  // namespace utils
