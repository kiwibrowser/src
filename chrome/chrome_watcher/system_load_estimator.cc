// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/chrome_watcher/system_load_estimator.h"

#include <vector>

#include "base/logging.h"
#include "base/macros.h"
#include "base/threading/platform_thread.h"

namespace chrome_watcher {
namespace {

using CounterHandle = SystemLoadEstimator::CounterHandle;

uint64_t FileTimeToUInt64(FILETIME time) {
  ULARGE_INTEGER output = {};
  output.LowPart = time.dwLowDateTime;
  output.HighPart = time.dwHighDateTime;
  return static_cast<uint64_t>(output.QuadPart);
}

CounterHandle AddPdhCounter(PDH_HQUERY query,
                            const wchar_t* object_name,
                            const wchar_t* counter_name) {
  DCHECK(counter_name);

  // Get the counter's path.
  PDH_COUNTER_PATH_ELEMENTS path_elements = {
      nullptr, const_cast<LPTSTR>(object_name), nullptr, nullptr,
      0,       const_cast<LPTSTR>(counter_name)};
  std::unique_ptr<wchar_t[]> counter_path(new wchar_t[PDH_MAX_COUNTER_PATH]);
  DWORD path_len = PDH_MAX_COUNTER_PATH;

  PDH_STATUS status =
      ::PdhMakeCounterPath(&path_elements, counter_path.get(), &path_len, 0);
  if (status != ERROR_SUCCESS)
    return CounterHandle();

  // Add the counter.
  HCOUNTER counter_raw = nullptr;
  status = ::PdhAddCounter(query, counter_path.get(), 0, &counter_raw);
  if (status != ERROR_SUCCESS)
    return CounterHandle();
  return CounterHandle(counter_raw);
}

bool GetDoubleCounterValue(HCOUNTER counter, int* value) {
  DCHECK(value);

  PDH_FMT_COUNTERVALUE counter_value = {};
  PDH_STATUS status = ::PdhGetFormattedCounterValue(counter, PDH_FMT_DOUBLE,
                                                    nullptr, &counter_value);
  if (status != ERROR_SUCCESS)
    return false;

  *value = static_cast<int>(counter_value.doubleValue);
  return true;
}

}  // namespace

void QueryDeleter::operator()(PDH_HQUERY query) const {
  if (query)
    ::PdhCloseQuery(query);
}

void CounterDeleter::operator()(PDH_HCOUNTER counter) const {
  if (counter)
    ::PdhRemoveCounter(counter);
}

bool SystemLoadEstimator::Measure(Estimate* estimate) {
  DCHECK(estimate);

  SystemLoadEstimator estimator;
  if (!estimator.Initialize())
    return false;

  return estimator.PerformMeasurements(estimate);
}

SystemLoadEstimator::SystemLoadEstimator() = default;

SystemLoadEstimator::~SystemLoadEstimator() = default;

bool SystemLoadEstimator::Initialize() {
  // Open the PDH query.
  PDH_HQUERY query_raw;
  if (::PdhOpenQuery(nullptr, 0, &query_raw) != ERROR_SUCCESS)
    return false;
  pdh_query_.reset(query_raw);

  // Add counters to the query.
  pdh_disk_idle_counter_ =
      AddPdhCounter(pdh_query_.get(), L"LogicalDisk(C:)", L"% Idle Time");
  if (!pdh_disk_idle_counter_)
    return false;

  pdh_disk_queue_counter_ = AddPdhCounter(pdh_query_.get(), L"LogicalDisk(C:)",
                                          L"Avg. Disk Queue Length");
  if (!pdh_disk_queue_counter_)
    return false;

  return true;
}

bool SystemLoadEstimator::PerformMeasurements(Estimate* estimate) {
  return PerformMeasurements(base::TimeDelta::FromSeconds(1), estimate);
}

bool SystemLoadEstimator::PerformMeasurements(base::TimeDelta duration,
                                              Estimate* estimate) {
  DCHECK(estimate);
  DCHECK(pdh_query_.get());
  DCHECK(pdh_disk_idle_counter_.get());
  DCHECK(pdh_disk_queue_counter_.get());

  // Initial cpu measurement.
  uint64_t cpu_total_start = 0ULL;
  uint64_t cpu_idle_start = 0ULL;
  if (!GetTotalAndIdleTimes(&cpu_total_start, &cpu_idle_start))
    return false;

  // Initial disk measurement.
  if (::PdhCollectQueryData(pdh_query_.get()) != ERROR_SUCCESS)
    return false;

  // Wait a bit.
  base::PlatformThread::Sleep(duration);

  // Final cpu measurement.
  uint64_t cpu_total_end = 0ULL;
  uint64_t cpu_idle_end = 0ULL;
  if (!GetTotalAndIdleTimes(&cpu_total_end, &cpu_idle_end))
    return false;
  uint64_t cpu_total = 0ULL;
  cpu_total = cpu_total_end - cpu_total_start;
  uint64_t cpu_idle = 0ULL;
  cpu_idle = cpu_idle_end - cpu_idle_start;

  if (cpu_total == 0)
    return false;  // The measurement is deemed invalid.
  estimate->cpu_load_pct = static_cast<int>(
      100. * static_cast<double>(cpu_total - cpu_idle) / cpu_total);

  // Final disk measurement.
  if (::PdhCollectQueryData(pdh_query_.get()) != ERROR_SUCCESS)
    return false;
  if (!GetDoubleCounterValue(pdh_disk_idle_counter_.get(),
                             &estimate->disk_idle_pct)) {
    return false;
  }
  return GetDoubleCounterValue(pdh_disk_queue_counter_.get(),
                               &estimate->avg_disk_queue_len);
}

bool SystemLoadEstimator::GetTotalAndIdleTimes(uint64_t* total,
                                               uint64_t* idle) {
  DCHECK(total);
  DCHECK(idle);

  FILETIME time_idle = {};
  FILETIME time_kernel = {};
  FILETIME time_user = {};
  if (!::GetSystemTimes(&time_idle, &time_kernel, &time_user))
    return false;

  // Note: kernel time includes idle time.
  *idle = FileTimeToUInt64(time_idle);
  *total = FileTimeToUInt64(time_kernel) + FileTimeToUInt64(time_user);
  return true;
}

}  // namespace chrome_watcher
