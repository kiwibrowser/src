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

#include "tests/perf_tests/DawnPerfTest.h"

#include <algorithm>
#include <fstream>
#include <limits>

#include "common/Assert.h"
#include "common/Log.h"
#include "dawn_platform/tracing/TraceEvent.h"
#include "tests/perf_tests/DawnPerfTestPlatform.h"
#include "utils/Timer.h"

namespace {

    DawnPerfTestEnvironment* gTestEnv = nullptr;

    void DumpTraceEventsToJSONFile(
        const std::vector<DawnPerfTestPlatform::TraceEvent>& traceEventBuffer,
        const char* traceFile) {
        std::ofstream outFile;
        outFile.open(traceFile, std::ios_base::app);

        for (const DawnPerfTestPlatform::TraceEvent& traceEvent : traceEventBuffer) {
            const char* category = nullptr;
            switch (traceEvent.category) {
                case dawn_platform::TraceCategory::General:
                    category = "general";
                    break;
                case dawn_platform::TraceCategory::Validation:
                    category = "validation";
                    break;
                case dawn_platform::TraceCategory::Recording:
                    category = "recording";
                    break;
                case dawn_platform::TraceCategory::GPUWork:
                    category = "gpu";
                    break;
                default:
                    UNREACHABLE();
            }

            uint64_t microseconds = static_cast<uint64_t>(traceEvent.timestamp * 1000.0 * 1000.0);

            outFile << ", { "
                    << "\"name\": \"" << traceEvent.name << "\", "
                    << "\"cat\": \"" << category << "\", "
                    << "\"ph\": \"" << traceEvent.phase << "\", "
                    << "\"id\": " << traceEvent.id << ", "
                    << "\"tid\": " << traceEvent.threadId << ", "
                    << "\"ts\": " << microseconds << ", "
                    << "\"pid\": \"Dawn\""
                    << " }";
        }
        outFile.close();
    }

}  // namespace

void InitDawnPerfTestEnvironment(int argc, char** argv) {
    gTestEnv = new DawnPerfTestEnvironment(argc, argv);
    DawnTestEnvironment::SetEnvironment(gTestEnv);
    testing::AddGlobalTestEnvironment(gTestEnv);
}

DawnPerfTestEnvironment::DawnPerfTestEnvironment(int argc, char** argv)
    : DawnTestEnvironment(argc, argv) {
    size_t argLen = 0;  // Set when parsing --arg=X arguments
    for (int i = 1; i < argc; ++i) {
        if (strcmp("--calibration", argv[i]) == 0) {
            mIsCalibrating = true;
            continue;
        }

        constexpr const char kOverrideStepsArg[] = "--override-steps=";
        argLen = sizeof(kOverrideStepsArg) - 1;
        if (strncmp(argv[i], kOverrideStepsArg, argLen) == 0) {
            const char* overrideSteps = argv[i] + argLen;
            if (overrideSteps[0] != '\0') {
                mOverrideStepsToRun = strtoul(overrideSteps, nullptr, 0);
            }
            continue;
        }

        constexpr const char kTraceFileArg[] = "--trace-file=";
        argLen = sizeof(kTraceFileArg) - 1;
        if (strncmp(argv[i], kTraceFileArg, argLen) == 0) {
            const char* traceFile = argv[i] + argLen;
            if (traceFile[0] != '\0') {
                mTraceFile = traceFile;
            }
            continue;
        }

        if (strcmp("-h", argv[i]) == 0 || strcmp("--help", argv[i]) == 0) {
            dawn::InfoLog()
                << "Additional flags:"
                << " [--calibration] [--override-steps=x] [--trace-file=file]\n"
                << "  --calibration: Only run calibration. Calibration allows the perf test"
                   " runner script to save some time.\n"
                << " --override-steps: Set a fixed number of steps to run for each test\n"
                << " --trace-file: The file to dump trace results.\n";
            continue;
        }
    }
}

DawnPerfTestEnvironment::~DawnPerfTestEnvironment() = default;

void DawnPerfTestEnvironment::SetUp() {
    DawnTestEnvironment::SetUp();

    mPlatform = std::make_unique<DawnPerfTestPlatform>();
    mInstance->SetPlatform(mPlatform.get());

    // Begin writing the trace event array.
    if (mTraceFile != nullptr) {
        std::ofstream outFile;
        outFile.open(mTraceFile);
        outFile << "{ \"traceEvents\": [";
        outFile << "{}";  // Dummy object so trace events can always prepend a comma
        outFile.flush();
        outFile.close();
    }
}

void DawnPerfTestEnvironment::TearDown() {
    // End writing the trace event array.
    if (mTraceFile != nullptr) {
        std::vector<DawnPerfTestPlatform::TraceEvent> traceEventBuffer =
            mPlatform->AcquireTraceEventBuffer();

        // Write remaining trace events.
        DumpTraceEventsToJSONFile(traceEventBuffer, mTraceFile);

        std::ofstream outFile;
        outFile.open(mTraceFile, std::ios_base::app);
        outFile << "]}";
        outFile << std::endl;
        outFile.close();
    }

    DawnTestEnvironment::TearDown();
}

bool DawnPerfTestEnvironment::IsCalibrating() const {
    return mIsCalibrating;
}

unsigned int DawnPerfTestEnvironment::OverrideStepsToRun() const {
    return mOverrideStepsToRun;
}

const char* DawnPerfTestEnvironment::GetTraceFile() const {
    return mTraceFile;
}

DawnPerfTestPlatform* DawnPerfTestEnvironment::GetPlatform() const {
    return mPlatform.get();
}

DawnPerfTestBase::DawnPerfTestBase(DawnTestBase* test,
                                   unsigned int iterationsPerStep,
                                   unsigned int maxStepsInFlight)
    : mTest(test),
      mIterationsPerStep(iterationsPerStep),
      mMaxStepsInFlight(maxStepsInFlight),
      mTimer(utils::CreateTimer()) {
}

DawnPerfTestBase::~DawnPerfTestBase() = default;

void DawnPerfTestBase::AbortTest() {
    mRunning = false;
}

void DawnPerfTestBase::RunTest() {
    if (gTestEnv->OverrideStepsToRun() == 0) {
        // Run to compute the approximate number of steps to perform.
        mStepsToRun = std::numeric_limits<unsigned int>::max();

        // Do a warmup run for calibration.
        DoRunLoop(kCalibrationRunTimeSeconds);
        DoRunLoop(kCalibrationRunTimeSeconds);

        // Scale steps down according to the time that exceeded one second.
        double scale = kCalibrationRunTimeSeconds / mTimer->GetElapsedTime();
        mStepsToRun = static_cast<unsigned int>(static_cast<double>(mNumStepsPerformed) * scale);

        // Calibration allows the perf test runner script to save some time.
        if (gTestEnv->IsCalibrating()) {
            PrintResult("steps", mStepsToRun, "count", false);
            return;
        }
    } else {
        mStepsToRun = gTestEnv->OverrideStepsToRun();
    }

    // Do another warmup run. Seems to consistently improve results.
    DoRunLoop(kMaximumRunTimeSeconds);

    DawnPerfTestPlatform* platform =
        reinterpret_cast<DawnPerfTestPlatform*>(gTestEnv->GetPlatform());
    const char* testName = ::testing::UnitTest::GetInstance()->current_test_info()->name();

    // Only enable trace event recording in this section.
    // We don't care about trace events during warmup and calibration.
    platform->EnableTraceEventRecording(true);
    {
        TRACE_EVENT0(platform, General, testName);
        for (unsigned int trial = 0; trial < kNumTrials; ++trial) {
            TRACE_EVENT0(platform, General, "Trial");
            DoRunLoop(kMaximumRunTimeSeconds);
            OutputResults();
        }
    }
    platform->EnableTraceEventRecording(false);
}

void DawnPerfTestBase::DoRunLoop(double maxRunTime) {
    dawn_platform::Platform* platform = gTestEnv->GetPlatform();

    mNumStepsPerformed = 0;
    cpuTime = 0;
    mRunning = true;

    wgpu::FenceDescriptor desc = {};
    uint64_t signaledFenceValue = 0;
    wgpu::Fence fence = mTest->queue.CreateFence(&desc);

    mTimer->Start();

    // This loop can be canceled by calling AbortTest().
    while (mRunning) {
        // Wait if there are too many steps in flight on the GPU.
        while (signaledFenceValue - fence.GetCompletedValue() >= mMaxStepsInFlight) {
            mTest->WaitABit();
        }
        TRACE_EVENT0(platform, General, "Step");
        double stepStart = mTimer->GetElapsedTime();
        Step();
        cpuTime += mTimer->GetElapsedTime() - stepStart;

        mTest->queue.Signal(fence, ++signaledFenceValue);

        if (mRunning) {
            ++mNumStepsPerformed;
            if (mTimer->GetElapsedTime() > maxRunTime) {
                mRunning = false;
            } else if (mNumStepsPerformed >= mStepsToRun) {
                mRunning = false;
            }
        }
    }

    // Wait for all GPU commands to complete.
    // TODO(enga): When Dawn has multiple backgrounds threads, add a Device::WaitForIdleForTesting()
    // which waits for all threads to stop doing work. When we output results, there should
    // be no additional incoming trace events.
    while (signaledFenceValue != fence.GetCompletedValue()) {
        mTest->WaitABit();
    }

    mTimer->Stop();
}

void DawnPerfTestBase::OutputResults() {
    // TODO(enga): When Dawn has multiple backgrounds threads, add a Device::WaitForIdleForTesting()
    // which waits for all threads to stop doing work. When we output results, there should
    // be no additional incoming trace events.
    DawnPerfTestPlatform* platform =
        reinterpret_cast<DawnPerfTestPlatform*>(gTestEnv->GetPlatform());

    std::vector<DawnPerfTestPlatform::TraceEvent> traceEventBuffer =
        platform->AcquireTraceEventBuffer();

    struct EventTracker {
        double start = std::numeric_limits<double>::max();
        double end = 0;
        uint32_t count = 0;
    };

    EventTracker validationTracker = {};
    EventTracker recordingTracker = {};

    double totalValidationTime = 0;
    double totalRecordingTime = 0;

    // Note: We assume END timestamps always come after their corresponding BEGIN timestamps.
    // TODO(enga): When Dawn has multiple threads, stratify by thread id.
    for (const DawnPerfTestPlatform::TraceEvent& traceEvent : traceEventBuffer) {
        EventTracker* tracker = nullptr;
        double* totalTime = nullptr;

        switch (traceEvent.category) {
            case dawn_platform::TraceCategory::Validation:
                tracker = &validationTracker;
                totalTime = &totalValidationTime;
                break;
            case dawn_platform::TraceCategory::Recording:
                tracker = &recordingTracker;
                totalTime = &totalRecordingTime;
                break;
            default:
                break;
        }

        if (tracker == nullptr) {
            continue;
        }

        if (traceEvent.phase == TRACE_EVENT_PHASE_BEGIN) {
            tracker->start = std::min(tracker->start, traceEvent.timestamp);
            tracker->count++;
        }

        if (traceEvent.phase == TRACE_EVENT_PHASE_END) {
            tracker->end = std::max(tracker->end, traceEvent.timestamp);
            ASSERT(tracker->count > 0);
            tracker->count--;

            if (tracker->count == 0) {
                *totalTime += (tracker->end - tracker->start);
                *tracker = {};
            }
        }
    }

    PrintPerIterationResultFromSeconds("wall_time", mTimer->GetElapsedTime(), true);
    PrintPerIterationResultFromSeconds("cpu_time", cpuTime, true);
    PrintPerIterationResultFromSeconds("validation_time", totalValidationTime, true);
    PrintPerIterationResultFromSeconds("recording_time", totalRecordingTime, true);

    const char* traceFile = gTestEnv->GetTraceFile();
    if (traceFile != nullptr) {
        DumpTraceEventsToJSONFile(traceEventBuffer, traceFile);
    }
}

void DawnPerfTestBase::PrintPerIterationResultFromSeconds(const std::string& trace,
                                                          double valueInSeconds,
                                                          bool important) const {
    if (valueInSeconds == 0) {
        return;
    }

    double secondsPerIteration =
        valueInSeconds / static_cast<double>(mNumStepsPerformed * mIterationsPerStep);

    // Give the result a different name to ensure separate graphs if we transition.
    if (secondsPerIteration > 1) {
        PrintResult(trace, secondsPerIteration * 1e3, "ms", important);
    } else if (secondsPerIteration > 1e-3) {
        PrintResult(trace, secondsPerIteration * 1e6, "us", important);
    } else {
        PrintResult(trace, secondsPerIteration * 1e9, "ns", important);
    }
}

void DawnPerfTestBase::PrintResult(const std::string& trace,
                                   double value,
                                   const std::string& units,
                                   bool important) const {
    PrintResultImpl(trace, std::to_string(value), units, important);
}

void DawnPerfTestBase::PrintResult(const std::string& trace,
                                   unsigned int value,
                                   const std::string& units,
                                   bool important) const {
    PrintResultImpl(trace, std::to_string(value), units, important);
}

void DawnPerfTestBase::PrintResultImpl(const std::string& trace,
                                       const std::string& value,
                                       const std::string& units,
                                       bool important) const {
    const ::testing::TestInfo* const testInfo =
        ::testing::UnitTest::GetInstance()->current_test_info();

    std::string metric = std::string(testInfo->test_suite_name()) + "." + trace;

    std::string story = testInfo->name();
    std::replace(story.begin(), story.end(), '/', '_');

    // The results are printed according to the format specified at
    // [chromium]//src/tools/perf/generate_legacy_perf_dashboard_json.py
    dawn::InfoLog() << (important ? "*" : "") << "RESULT " << metric << ": " << story << "= "
                    << value << " " << units;
}
