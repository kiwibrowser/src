// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/memory/swap_thrashing_monitor_delegate_win.h"

#include <windows.h>
#include <winternl.h>

#include "base/files/file_path.h"
#include "base/win/win_util.h"
#include "chrome/common/chrome_constants.h"

namespace memory {

namespace {

// The threshold that we use to consider that an hard page-fault delta sample is
// high, in hard page-fault / [sampling frequency].
//
// TODO(sebmarchand): Confirm that this value is adequate, crbug.com/779332.
const size_t kHighSwappingThreshold = 20;

// The minimum number of samples that need to be above the threshold to be in
// the suspected state.
const size_t kSampleCountToBecomeSuspectedState = 5;

// The minimum number of samples that need to be above the threshold to be in
// the confirmed state.
const size_t kSampleCountToBecomeConfirmedState = 10;

// ntstatus.h conflicts with windows.h so define this locally.
#define STATUS_SUCCESS ((NTSTATUS)0x00000000L)
#define STATUS_BUFFER_TOO_SMALL ((NTSTATUS)0xC0000023L)
#define STATUS_INFO_LENGTH_MISMATCH ((NTSTATUS)0xC0000004L)

// The struct used to return system process information via the NT internal
// QuerySystemInformation call. This is partially documented at
// http://goo.gl/Ja9MrH and fully documented at http://goo.gl/QJ70rn
// This structure is laid out in the same format on both 32-bit and 64-bit
// systems, but has a different size due to the various pointer-sized fields.
struct SYSTEM_PROCESS_INFORMATION_EX {
  ULONG NextEntryOffset;
  ULONG NumberOfThreads;
  LARGE_INTEGER WorkingSetPrivateSize;
  ULONG HardFaultCount;
  ULONG NumberOfThreadsHighWatermark;
  ULONGLONG CycleTime;
  LARGE_INTEGER CreateTime;
  LARGE_INTEGER UserTime;
  LARGE_INTEGER KernelTime;
  UNICODE_STRING ImageName;
  LONG KPriority;
  // This is labeled a handle so that it expands to the correct size for 32-bit
  // and 64-bit operating systems. However, under the hood it's a 32-bit DWORD
  // containing the process ID.
  HANDLE UniqueProcessId;
  PVOID Reserved3;
  ULONG HandleCount;
  BYTE Reserved4[4];
  PVOID Reserved5[11];
  SIZE_T PeakPagefileUsage;
  SIZE_T PrivatePageCount;
  LARGE_INTEGER Reserved6[6];
  // Array of SYSTEM_THREAD_INFORMATION structs follows.
};

// Gets the hard fault count of the current process through |hard_fault_count|.
// Returns true on success.
//
// TODO(sebmarchand): Use TaskManager to get this information, crbug.com/779329.
base::Optional<int64_t> GetHardFaultCountForChromeProcesses() {
  base::Optional<int64_t> hard_fault_count = 0;

  static const auto query_system_information_ptr =
      reinterpret_cast<decltype(&::NtQuerySystemInformation)>(GetProcAddress(
          GetModuleHandle(L"ntdll.dll"), "NtQuerySystemInformation"));
  if (!query_system_information_ptr)
    return base::nullopt;

  // The output of this system call depends on the number of threads and
  // processes on the entire system, and this can change between calls. Retry
  // a small handful of times growing the buffer along the way.
  // NOTE: The actual required size depends entirely on the number of processes
  //       and threads running on the system. The initial guess suffices for
  //       ~100s of processes and ~1000s of threads.
  std::vector<uint8_t> buffer(32 * 1024);
  for (size_t tries = 0; tries < 3; ++tries) {
    ULONG return_length = 0;
    const NTSTATUS status = query_system_information_ptr(
        SystemProcessInformation, buffer.data(),
        static_cast<ULONG>(buffer.size()), &return_length);

    if (status == STATUS_SUCCESS)
      break;

    if (status == STATUS_INFO_LENGTH_MISMATCH ||
        status == STATUS_BUFFER_TOO_SMALL) {
      // Insufficient buffer. Grow to the returned |return_length| plus 10%
      // extra to avoid frequent reallocations and try again.
      DCHECK_GT(return_length, buffer.size());
      buffer.resize(static_cast<ULONG>(return_length * 1.1));
    } else {
      // An error other than the two above.
      return base::nullopt;
    }
  }

  // Look for the struct housing information for the Chrome processes.
  size_t index = 0;
  while (index < buffer.size()) {
    DCHECK_LE(index + sizeof(SYSTEM_PROCESS_INFORMATION_EX), buffer.size());
    SYSTEM_PROCESS_INFORMATION_EX* proc_info =
        reinterpret_cast<SYSTEM_PROCESS_INFORMATION_EX*>(buffer.data() + index);
    if (base::FilePath::CompareEqualIgnoreCase(
            proc_info->ImageName.Buffer,
            chrome::kBrowserProcessExecutableName)) {
      hard_fault_count = hard_fault_count.value() + proc_info->HardFaultCount;
    }
    // The list ends when NextEntryOffset is zero. This also prevents busy
    // looping if the data is in fact invalid.
    if (proc_info->NextEntryOffset <= 0)
      break;
    index += proc_info->NextEntryOffset;
  }
  return hard_fault_count;
}

}  // namespace

// The number of sample that we'll keep track of.
const size_t SwapThrashingMonitorDelegateWin::HardFaultDeltasWindow::
    kHardFaultDeltasWindowSize = 12;

SwapThrashingMonitorDelegateWin::SwapThrashingMonitorDelegateWin()
    : hard_fault_deltas_window_(std::make_unique<HardFaultDeltasWindow>()) {}

SwapThrashingMonitorDelegateWin::~SwapThrashingMonitorDelegateWin() {}

SwapThrashingMonitorDelegateWin::HardFaultDeltasWindow::HardFaultDeltasWindow()
    : latest_hard_fault_count_(), observation_above_threshold_count_(0U) {}

SwapThrashingMonitorDelegateWin::HardFaultDeltasWindow::
    ~HardFaultDeltasWindow() {}

void SwapThrashingMonitorDelegateWin::HardFaultDeltasWindow::OnObservation(
    uint64_t hard_fault_count) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (!latest_hard_fault_count_) {
    latest_hard_fault_count_ = hard_fault_count;
    return;
  }

  // Start by removing the sample that have expired.
  if (observation_deltas_.size() == kHardFaultDeltasWindowSize) {
    if (observation_deltas_.front() >= kHighSwappingThreshold) {
      DCHECK_GT(observation_above_threshold_count_, 0U);
      --observation_above_threshold_count_;
    }
    observation_deltas_.pop_front();
  }

  size_t delta = 0;
  if (hard_fault_count > latest_hard_fault_count_.value()) {
    delta = hard_fault_count - latest_hard_fault_count_.value();
  } else {
    delta = latest_hard_fault_count_.value() - hard_fault_count;
  }
  observation_deltas_.push_back(delta);
  latest_hard_fault_count_ = hard_fault_count;
  if (delta >= kHighSwappingThreshold)
    ++observation_above_threshold_count_;
}

SwapThrashingLevel
SwapThrashingMonitorDelegateWin::SampleAndCalculateSwapThrashingLevel() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!RecordHardFaultCountForChromeProcesses())
    return SwapThrashingLevel::SWAP_THRASHING_LEVEL_NONE;

  SwapThrashingLevel level = SwapThrashingLevel::SWAP_THRASHING_LEVEL_NONE;

  // This delegate looks at a fixed number of past samples to determine if the
  // system is now in a thrashing state. To do this it counts how many samples
  // exceed a certain threshold.
  size_t sample_above_threshold =
      hard_fault_deltas_window_->observation_above_threshold_count();
  if (sample_above_threshold >= kSampleCountToBecomeConfirmedState) {
    level = SwapThrashingLevel::SWAP_THRASHING_LEVEL_CONFIRMED;
  } else if (sample_above_threshold >= kSampleCountToBecomeSuspectedState) {
    level = SwapThrashingLevel::SWAP_THRASHING_LEVEL_SUSPECTED;
  }

  return level;
}

bool SwapThrashingMonitorDelegateWin::RecordHardFaultCountForChromeProcesses() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  base::Optional<int64_t> hard_fault_count =
      GetHardFaultCountForChromeProcesses();
  if (!hard_fault_count) {
    LOG(ERROR) << "Unable to retrieve the hard fault counts for Chrome "
               << "processes.";
    return false;
  }
  hard_fault_deltas_window_->OnObservation(hard_fault_count.value());

  return true;
}

}  // namespace memory
