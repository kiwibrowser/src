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

#include <CoreServices/CoreServices.h>
#include <mach/mach.h>
#include <mach/mach_time.h>

namespace utils {

    class OSXTimer : public Timer {
      public:
        OSXTimer() : Timer(), mRunning(false), mSecondCoeff(0) {
        }

        ~OSXTimer() override = default;

        void Start() override {
            mStartTime = mach_absolute_time();
            // Cache secondCoeff
            GetSecondCoeff();
            mRunning = true;
        }

        void Stop() override {
            mStopTime = mach_absolute_time();
            mRunning = false;
        }

        double GetElapsedTime() const override {
            if (mRunning) {
                return mSecondCoeff * (mach_absolute_time() - mStartTime);
            } else {
                return mSecondCoeff * (mStopTime - mStartTime);
            }
        }

        double GetAbsoluteTime() override {
            return GetSecondCoeff() * mach_absolute_time();
        }

      private:
        double GetSecondCoeff() {
            // If this is the first time we've run, get the timebase.
            if (mSecondCoeff == 0.0) {
                mach_timebase_info_data_t timebaseInfo;
                mach_timebase_info(&timebaseInfo);

                mSecondCoeff = timebaseInfo.numer * (1.0 / 1000000000) / timebaseInfo.denom;
            }

            return mSecondCoeff;
        }

        bool mRunning;
        uint64_t mStartTime;
        uint64_t mStopTime;
        double mSecondCoeff;
    };

    Timer* CreateTimer() {
        return new OSXTimer();
    }

}  // namespace utils
