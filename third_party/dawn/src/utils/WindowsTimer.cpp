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

#include <windows.h>

namespace utils {

    class WindowsTimer : public Timer {
      public:
        WindowsTimer() : Timer(), mRunning(false), mFrequency(0) {
        }

        ~WindowsTimer() override = default;

        void Start() override {
            LARGE_INTEGER curTime;
            QueryPerformanceCounter(&curTime);
            mStartTime = curTime.QuadPart;

            // Cache the frequency
            GetFrequency();

            mRunning = true;
        }

        void Stop() override {
            LARGE_INTEGER curTime;
            QueryPerformanceCounter(&curTime);
            mStopTime = curTime.QuadPart;

            mRunning = false;
        }

        double GetElapsedTime() const override {
            LONGLONG endTime;
            if (mRunning) {
                LARGE_INTEGER curTime;
                QueryPerformanceCounter(&curTime);
                endTime = curTime.QuadPart;
            } else {
                endTime = mStopTime;
            }

            return static_cast<double>(endTime - mStartTime) / mFrequency;
        }

        double GetAbsoluteTime() override {
            LARGE_INTEGER curTime;
            QueryPerformanceCounter(&curTime);

            return static_cast<double>(curTime.QuadPart) / GetFrequency();
        }

      private:
        LONGLONG GetFrequency() {
            if (mFrequency == 0) {
                LARGE_INTEGER frequency = {};
                QueryPerformanceFrequency(&frequency);

                mFrequency = frequency.QuadPart;
            }

            return mFrequency;
        }

        bool mRunning;
        LONGLONG mStartTime;
        LONGLONG mStopTime;
        LONGLONG mFrequency;
    };

    Timer* CreateTimer() {
        return new WindowsTimer();
    }

}  // namespace utils
