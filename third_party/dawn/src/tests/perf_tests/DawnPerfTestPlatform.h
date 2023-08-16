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

#ifndef TESTS_PERFTESTS_DAWNPERFTESTPLATFORM_H_
#define TESTS_PERFTESTS_DAWNPERFTESTPLATFORM_H_

#include <dawn_platform/DawnPlatform.h>

#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

namespace utils {
    class Timer;
}

class DawnPerfTestPlatform : public dawn_platform::Platform {
  public:
    // These are trace events according to Google's "Trace Event Format".
    // See https://docs.google.com/document/d/1CvAClvFfyA5R-PhYUmn5OOQtYMH4h6I0nSsKchNAySU
    // Only a subset of the properties are implemented.
    struct TraceEvent final {
        TraceEvent() {
        }
        TraceEvent(char phaseIn,
                   dawn_platform::TraceCategory categoryIn,
                   const char* nameIn,
                   uint64_t idIn,
                   double timestampIn)
            : phase(phaseIn), category(categoryIn), name(nameIn), id(idIn), timestamp(timestampIn) {
        }

        char phase = 0;
        dawn_platform::TraceCategory category;
        const char* name = nullptr;
        uint64_t id = 0;
        std::string threadId;
        double timestamp = 0;
    };

    DawnPerfTestPlatform();
    ~DawnPerfTestPlatform() override;

    void EnableTraceEventRecording(bool enable);
    std::vector<TraceEvent> AcquireTraceEventBuffer();

  private:
    const unsigned char* GetTraceCategoryEnabledFlag(
        dawn_platform::TraceCategory category) override;

    double MonotonicallyIncreasingTime() override;

    std::vector<TraceEvent>* GetLocalTraceEventBuffer();

    uint64_t AddTraceEvent(char phase,
                           const unsigned char* categoryGroupEnabled,
                           const char* name,
                           uint64_t id,
                           double timestamp,
                           int numArgs,
                           const char** argNames,
                           const unsigned char* argTypes,
                           const uint64_t* argValues,
                           unsigned char flags) override;

    bool mRecordTraceEvents = false;
    std::unique_ptr<utils::Timer> mTimer;

    // Trace event record.
    // Each uses their own trace event buffer, but the PerfTestPlatform owns all of them in
    // this map. The map stores all of them so we can iterate through them and flush when
    // AcquireTraceEventBuffer is called.
    std::unordered_map<std::thread::id, std::unique_ptr<std::vector<TraceEvent>>>
        mTraceEventBuffers;
    std::mutex mTraceEventBufferMapMutex;
};

#endif  // TESTS_PERFTESTS_DAWNPERFTESTPLATFORM_H_
