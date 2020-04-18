// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/controller/oom_intervention_impl.h"

#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>

#include "mojo/public/cpp/bindings/strong_binding.h"
#include "third_party/blink/common/oom_intervention/oom_intervention_types.h"
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/renderer/platform/bindings/v8_per_isolate_data.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/web_task_runner.h"
#include "third_party/blink/renderer/platform/wtf/allocator/partitions.h"

namespace blink {

namespace {

constexpr uint32_t kMaxLineSize = 4096;
bool ReadFileContents(int fd, char contents[kMaxLineSize]) {
  lseek(fd, 0, SEEK_SET);
  int res = read(fd, contents, kMaxLineSize - 1);
  if (res <= 0)
    return false;
  contents[res] = '\0';
  return true;
}

// Since the measurement is done every second in background, optimizations are
// in place to get just the metrics we need from the proc files. So, this
// calculation exists here instead of using the cross-process memory-infra code.
bool CalculateProcessMemoryFootprint(int statm_fd,
                                     int status_fd,
                                     uint64_t* private_footprint,
                                     uint64_t* swap_footprint) {
  // Get total resident and shared sizes from statm file.
  static size_t page_size = getpagesize();
  uint64_t resident_pages;
  uint64_t shared_pages;
  char line[kMaxLineSize];
  if (!ReadFileContents(statm_fd, line))
    return false;
  int num_scanned =
      sscanf(line, "%*s %" SCNu64 " %" SCNu64, &resident_pages, &shared_pages);
  if (num_scanned != 2)
    return false;

  // Get swap size from status file. The format is: VmSwap :  10 kB.
  if (!ReadFileContents(status_fd, line))
    return false;
  char* swap_line = strstr(line, "VmSwap");
  if (!swap_line)
    return false;
  num_scanned = sscanf(swap_line, "VmSwap: %" SCNu64 " kB", swap_footprint);
  if (num_scanned != 1)
    return false;

  *swap_footprint *= 1024;
  *private_footprint =
      (resident_pages - shared_pages) * page_size + *swap_footprint;
  return true;
}

// Roughly caclculates amount of memory which is used to execute pages.
uint64_t BlinkMemoryWorkloadCaculator() {
  v8::Isolate* isolate = V8PerIsolateData::MainThreadIsolate();
  DCHECK(isolate);
  v8::HeapStatistics heap_statistics;
  isolate->GetHeapStatistics(&heap_statistics);
  // TODO: Add memory usage for worker threads.
  size_t v8_size =
      heap_statistics.total_heap_size() + heap_statistics.malloced_memory();
  size_t blink_gc_size = ProcessHeap::TotalAllocatedObjectSize() +
                         ProcessHeap::TotalMarkedObjectSize();
  size_t partition_alloc_size = WTF::Partitions::TotalSizeOfCommittedPages();
  return v8_size + blink_gc_size + partition_alloc_size;
}

}  // namespace

// static
void OomInterventionImpl::Create(mojom::blink::OomInterventionRequest request) {
  mojo::MakeStrongBinding(
      std::make_unique<OomInterventionImpl>(
          WTF::BindRepeating(&BlinkMemoryWorkloadCaculator)),
      std::move(request));
}

OomInterventionImpl::OomInterventionImpl(
    MemoryWorkloadCaculator workload_calculator)
    : workload_calculator_(std::move(workload_calculator)),
      timer_(Platform::Current()->MainThread()->GetTaskRunner(),
             this,
             &OomInterventionImpl::Check) {
  DCHECK(workload_calculator_);
}

OomInterventionImpl::~OomInterventionImpl() {}

void OomInterventionImpl::StartDetection(
    mojom::blink::OomInterventionHostPtr host,
    base::UnsafeSharedMemoryRegion shared_metrics_buffer,
    uint64_t memory_workload_threshold,
    bool trigger_intervention) {
  host_ = std::move(host);
  shared_metrics_buffer_ = shared_metrics_buffer.Map();

  // See https://goo.gl/KjWnZP For details about why we read these files from
  // sandboxed renderer. Keep these files open when detection is enabled.
  if (!statm_fd_.is_valid())
    statm_fd_.reset(open("/proc/self/statm", O_RDONLY));
  if (!status_fd_.is_valid())
    status_fd_.reset(open("/proc/self/status", O_RDONLY));
  // Disable intervention if we cannot get memory details of current process.
  // TODO(ssid): Add UMA here to make sure we don't stop disable intervention
  // more often than expected.
  if (!statm_fd_.is_valid() || !status_fd_.is_valid())
    return;

  memory_workload_threshold_ = memory_workload_threshold;
  trigger_intervention_ = trigger_intervention;

  timer_.Start(TimeDelta(), TimeDelta::FromSeconds(1), FROM_HERE);
}

void OomInterventionImpl::Check(TimerBase*) {
  DCHECK(host_);
  DCHECK_GT(memory_workload_threshold_, 0UL);

  uint64_t workload = workload_calculator_.Run();
  if (workload > memory_workload_threshold_) {
    host_->OnHighMemoryUsage(trigger_intervention_);

    if (trigger_intervention_) {
      // The ScopedPagePauser is destroyed when the intervention is declined and
      // mojo strong binding is disconnected.
      pauser_.reset(new ScopedPagePauser);
    }
  }

  // Write memory metrics to shared buffer accesible by browser.
  OomInterventionMetrics* metrics_shared =
      static_cast<OomInterventionMetrics*>(shared_metrics_buffer_.memory());
  metrics_shared->current_blink_usage_kb = workload / 1024;
  DCHECK(statm_fd_.is_valid());
  DCHECK(status_fd_.is_valid());
  uint64_t private_footprint, swap;
  if (CalculateProcessMemoryFootprint(statm_fd_.get(), status_fd_.get(),
                                      &private_footprint, &swap)) {
    metrics_shared->current_private_footprint_kb = private_footprint / 1024;
    metrics_shared->current_swap_kb = swap / 1024;
  }
}

}  // namespace blink
