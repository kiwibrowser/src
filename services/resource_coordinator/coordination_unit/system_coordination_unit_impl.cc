// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/resource_coordinator/coordination_unit/system_coordination_unit_impl.h"

#include "services/resource_coordinator/coordination_unit/frame_coordination_unit_impl.h"
#include "services/resource_coordinator/coordination_unit/page_coordination_unit_impl.h"
#include "services/resource_coordinator/coordination_unit/process_coordination_unit_impl.h"

#include "base/macros.h"
#include "base/process/process_handle.h"

namespace resource_coordinator {

SystemCoordinationUnitImpl::SystemCoordinationUnitImpl(
    const CoordinationUnitID& id,
    std::unique_ptr<service_manager::ServiceContextRef> service_ref)
    : CoordinationUnitInterface(id, std::move(service_ref)) {}

SystemCoordinationUnitImpl::~SystemCoordinationUnitImpl() = default;

void SystemCoordinationUnitImpl::OnProcessCPUUsageReady() {
  SendEvent(mojom::Event::kProcessCPUUsageReady);
}

void SystemCoordinationUnitImpl::DistributeMeasurementBatch(
    mojom::ProcessResourceMeasurementBatchPtr measurement_batch) {
  // Grab all the processes.
  std::vector<ProcessCoordinationUnitImpl*> processes =
      ProcessCoordinationUnitImpl::GetAllProcessCoordinationUnits();

  base::TimeDelta time_since_last_measurement;
  if (!last_measurement_batch_time_.is_null()) {
    // Use the end of the measurement batch as a proxy for when every
    // measurement was acquired. For the purpose of estimating CPU usage
    // over the duration from last measurement, it'll be near enough. The error
    // will average out, and there's an inherent race in knowing when a
    // measurement was actually acquired in any case.
    time_since_last_measurement =
        measurement_batch->batch_ended_time - last_measurement_batch_time_;
    DCHECK_LE(base::TimeDelta(), time_since_last_measurement);
  }
  last_measurement_batch_time_ = measurement_batch->batch_ended_time;

  // Keep track of the pages updated with CPU cost for the second pass,
  // where their memory usage is updated.
  std::set<PageCoordinationUnitImpl*> pages;
  for (const auto& measurement : measurement_batch->measurements) {
    for (auto it = processes.begin(); it != processes.end(); ++it) {
      ProcessCoordinationUnitImpl* process = *it;
      int64_t process_pid;
      // TODO(siggi): This seems pretty silly - we're going O(N^2) in processes
      //     here, and going through a relatively expensive accessor for the
      //     PID.
      if (process->GetProperty(mojom::PropertyType::kPID, &process_pid) &&
          static_cast<base::ProcessId>(process_pid) == measurement->pid) {
        base::TimeDelta cumulative_cpu_delta =
            measurement->cpu_usage - process->cumulative_cpu_usage();
        DCHECK_LE(base::TimeDelta(), cumulative_cpu_delta);

        // Distribute the CPU delta to the pages that own the frames in this
        // process.
        std::set<FrameCoordinationUnitImpl*> frames =
            process->GetFrameCoordinationUnits();
        if (!frames.empty()) {
          // To make sure we don't systemically truncate the remainder of the
          // delta, simply subtract the remainder and "hold it back" from the
          // measurement. Since our measurement is cumulative, we'll see that
          // CPU time again in the next measurement.
          cumulative_cpu_delta -=
              cumulative_cpu_delta %
              base::TimeDelta::FromMicroseconds(frames.size());

          for (FrameCoordinationUnitImpl* frame : frames) {
            PageCoordinationUnitImpl* page = frame->GetPageCoordinationUnit();
            if (page) {
              page->set_usage_estimate_time(last_measurement_batch_time_);
              page->set_cumulative_cpu_usage_estimate(
                  page->cumulative_cpu_usage_estimate() +
                  cumulative_cpu_delta / frames.size());

              pages.insert(page);
            }
          }
        } else {
          // TODO(siggi): The process has zero frames, maybe this is a newly
          //    started renderer and if so, this might be a good place to
          //    estimate the process overhead. Alternatively perhaps the first
          //    measurement for each process, or a lower bound thereof will
          //    converge to a decent estimate.
        }

        if (process->cumulative_cpu_usage().is_zero() ||
            time_since_last_measurement.is_zero()) {
          // Imitate the behavior of GetPlatformIndependentCPUUsage, which
          // yields zero for the initial measurement of each process.
          process->SetCPUUsage(0.0);
        } else {
          double cpu_usage = 100.0 * cumulative_cpu_delta.InMicrosecondsF() /
                             time_since_last_measurement.InMicrosecondsF();
          process->SetCPUUsage(cpu_usage);
        }
        process->set_cumulative_cpu_usage(process->cumulative_cpu_usage() +
                                          cumulative_cpu_delta);
        process->set_private_footprint_kb(measurement->private_footprint_kb);

        // Remove found processes.
        processes.erase(it);
        break;
      }
    }
  }

  // Clear processes we didn't get data for.
  for (ProcessCoordinationUnitImpl* process : processes) {
    process->SetCPUUsage(0.0);
    process->set_private_footprint_kb(0);
  }

  // Iterate through the pages involved to distribute the memory to them.
  for (PageCoordinationUnitImpl* page : pages) {
    uint64_t private_footprint_kb_sum = 0;
    const auto& frames = page->GetFrameCoordinationUnits();
    for (FrameCoordinationUnitImpl* frame : frames) {
      ProcessCoordinationUnitImpl* process =
          frame->GetProcessCoordinationUnit();
      if (process) {
        private_footprint_kb_sum += process->private_footprint_kb() /
                                    process->GetFrameCoordinationUnits().size();
      }
    }

    page->set_private_footprint_kb_estimate(private_footprint_kb_sum);

    DCHECK_EQ(last_measurement_batch_time_, page->usage_estimate_time());
  }

  // Fire the end update signal.
  OnProcessCPUUsageReady();
}

void SystemCoordinationUnitImpl::OnEventReceived(mojom::Event event) {
  for (auto& observer : observers())
    observer.OnSystemEventReceived(this, event);
}

void SystemCoordinationUnitImpl::OnPropertyChanged(
    mojom::PropertyType property_type,
    int64_t value) {
  for (auto& observer : observers())
    observer.OnSystemPropertyChanged(this, property_type, value);
}

}  // namespace resource_coordinator
