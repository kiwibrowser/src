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

#ifndef TESTS_PERFTESTS_DAWNPERFTEST_H_
#define TESTS_PERFTESTS_DAWNPERFTEST_H_

#include "tests/DawnTest.h"

namespace utils {
    class Timer;
}

class DawnPerfTestPlatform;

void InitDawnPerfTestEnvironment(int argc, char** argv);

class DawnPerfTestEnvironment : public DawnTestEnvironment {
  public:
    DawnPerfTestEnvironment(int argc, char** argv);
    ~DawnPerfTestEnvironment() override;

    void SetUp() override;
    void TearDown() override;

    bool IsCalibrating() const;
    unsigned int OverrideStepsToRun() const;

    // Returns the path to the trace file, or nullptr if traces should
    // not be written to a json file.
    const char* GetTraceFile() const;

    DawnPerfTestPlatform* GetPlatform() const;

  private:
    // Only run calibration which allows the perf test runner to save time.
    bool mIsCalibrating = false;

    // If non-zero, overrides the number of steps.
    unsigned int mOverrideStepsToRun = 0;

    const char* mTraceFile = nullptr;

    std::unique_ptr<DawnPerfTestPlatform> mPlatform;
};

class DawnPerfTestBase {
    static constexpr double kCalibrationRunTimeSeconds = 1.0;
    static constexpr double kMaximumRunTimeSeconds = 10.0;
    static constexpr unsigned int kNumTrials = 3;

  public:
    // Perf test results are reported as the amortized time of |mStepsToRun| * |mIterationsPerStep|.
    // A test deriving from |DawnPerfTestBase| must call the base contructor with
    // |iterationsPerStep| appropriately to reflect the amount of work performed.
    // |maxStepsInFlight| may be used to mimic having multiple frames or workloads in flight which
    // is common with double or triple buffered applications.
    DawnPerfTestBase(DawnTestBase* test,
                     unsigned int iterationsPerStep,
                     unsigned int maxStepsInFlight);
    virtual ~DawnPerfTestBase();

  protected:
    // Call if the test step was aborted and the test should stop running.
    void AbortTest();

    void RunTest();
    void PrintPerIterationResultFromSeconds(const std::string& trace,
                                            double valueInSeconds,
                                            bool important) const;
    void PrintResult(const std::string& trace,
                     double value,
                     const std::string& units,
                     bool important) const;
    void PrintResult(const std::string& trace,
                     unsigned int value,
                     const std::string& units,
                     bool important) const;

  private:
    void DoRunLoop(double maxRunTime);
    void OutputResults();

    void PrintResultImpl(const std::string& trace,
                         const std::string& value,
                         const std::string& units,
                         bool important) const;

    virtual void Step() = 0;

    DawnTestBase* mTest;
    bool mRunning = false;
    const unsigned int mIterationsPerStep;
    const unsigned int mMaxStepsInFlight;
    unsigned int mStepsToRun = 0;
    unsigned int mNumStepsPerformed = 0;
    double cpuTime;
    std::unique_ptr<utils::Timer> mTimer;
};

template <typename Params = AdapterTestParam>
class DawnPerfTestWithParams : public DawnTestWithParams<Params>, public DawnPerfTestBase {
  protected:
    DawnPerfTestWithParams(unsigned int iterationsPerStep, unsigned int maxStepsInFlight)
        : DawnTestWithParams<Params>(),
          DawnPerfTestBase(this, iterationsPerStep, maxStepsInFlight) {
    }
    ~DawnPerfTestWithParams() override = default;
};

using DawnPerfTest = DawnPerfTestWithParams<>;

#define DAWN_INSTANTIATE_PERF_TEST_SUITE_P(testName, ...)                                      \
    INSTANTIATE_TEST_SUITE_P(                                                                  \
        , testName, ::testing::ValuesIn(MakeParamGenerator<testName::ParamType>(__VA_ARGS__)), \
        testing::PrintToStringParamName())

#endif  // TESTS_PERFTESTS_DAWNPERFTEST_H_
