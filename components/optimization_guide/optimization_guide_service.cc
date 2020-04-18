// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/optimization_guide/optimization_guide_service.h"

#include <string>

#include "base/bind.h"
#include "base/files/file.h"
#include "base/files/file_util.h"
#include "base/metrics/histogram_macros.h"
#include "base/task_scheduler/post_task.h"

namespace optimization_guide {

namespace {

// Version "0" corresponds to no processed version. By service conventions,
// we represent it as a dotted triple.
const char kNullVersion[] = "0.0.0";

void RecordProcessHintsResult(
    OptimizationGuideService::ProcessHintsResult result) {
  UMA_HISTOGRAM_ENUMERATION(
      "OptimizationGuide.ProcessHintsResult", static_cast<int>(result),
      static_cast<int>(OptimizationGuideService::ProcessHintsResult::MAX));
}

}  // namespace

ComponentInfo::ComponentInfo(const base::Version& hints_version,
                             const base::FilePath& hints_path)
    : hints_version(hints_version), hints_path(hints_path) {}

ComponentInfo::~ComponentInfo() {}

OptimizationGuideService::OptimizationGuideService(
    const scoped_refptr<base::SingleThreadTaskRunner>& io_thread_task_runner)
    : background_task_runner_(base::CreateSequencedTaskRunnerWithTraits(
          {base::MayBlock(), base::TaskPriority::BACKGROUND})),
      io_thread_task_runner_(io_thread_task_runner),
      latest_processed_version_(kNullVersion) {
  DETACH_FROM_SEQUENCE(sequence_checker_);
}

OptimizationGuideService::~OptimizationGuideService() {}

void OptimizationGuideService::SetLatestProcessedVersionForTesting(
    const base::Version& version) {
  latest_processed_version_ = version;
}

void OptimizationGuideService::AddObserver(
    OptimizationGuideServiceObserver* observer) {
  if (io_thread_task_runner_->BelongsToCurrentThread()) {
    AddObserverOnIOThread(observer);
  } else {
    io_thread_task_runner_->PostTask(
        FROM_HERE, base::Bind(&OptimizationGuideService::AddObserverOnIOThread,
                              base::Unretained(this), observer));
  }
}

void OptimizationGuideService::AddObserverOnIOThread(
    OptimizationGuideServiceObserver* observer) {
  DCHECK(io_thread_task_runner_->BelongsToCurrentThread());
  observers_.AddObserver(observer);
}

void OptimizationGuideService::RemoveObserver(
    OptimizationGuideServiceObserver* observer) {
  if (io_thread_task_runner_->BelongsToCurrentThread()) {
    RemoveObserverOnIOThread(observer);
  } else {
    io_thread_task_runner_->PostTask(
        FROM_HERE,
        base::Bind(&OptimizationGuideService::RemoveObserverOnIOThread,
                   base::Unretained(this), observer));
  }
}

void OptimizationGuideService::RemoveObserverOnIOThread(
    OptimizationGuideServiceObserver* observer) {
  DCHECK(io_thread_task_runner_->BelongsToCurrentThread());
  observers_.RemoveObserver(observer);
}

void OptimizationGuideService::ProcessHints(
    const ComponentInfo& component_info) {
  background_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&OptimizationGuideService::ProcessHintsInBackground,
                     base::Unretained(this), component_info));
}

void OptimizationGuideService::ProcessHintsInBackground(
    const ComponentInfo& component_info) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  // TODO(crbug.com/783246): Add crash loop detection to ensure bad component
  // updates do not crash Chrome.

  if (!component_info.hints_version.IsValid()) {
    RecordProcessHintsResult(ProcessHintsResult::FAILED_INVALID_PARAMETERS);
    return;
  }
  if (latest_processed_version_.CompareTo(component_info.hints_version) >= 0)
    return;
  if (component_info.hints_path.empty()) {
    RecordProcessHintsResult(ProcessHintsResult::FAILED_INVALID_PARAMETERS);
    return;
  }
  std::string binary_pb;
  if (!base::ReadFileToString(component_info.hints_path, &binary_pb)) {
    RecordProcessHintsResult(ProcessHintsResult::FAILED_READING_FILE);
    return;
  }

  proto::Configuration new_config;
  if (!new_config.ParseFromString(binary_pb)) {
    RecordProcessHintsResult(ProcessHintsResult::FAILED_INVALID_CONFIGURATION);
    return;
  }
  latest_processed_version_ = component_info.hints_version;

  RecordProcessHintsResult(ProcessHintsResult::SUCCESS);
  io_thread_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&OptimizationGuideService::DispatchHintsOnIOThread,
                     base::Unretained(this), new_config, component_info));
}

void OptimizationGuideService::DispatchHintsOnIOThread(
    const proto::Configuration& config,
    const ComponentInfo& component_info) {
  DCHECK(io_thread_task_runner_->BelongsToCurrentThread());

  for (auto& observer : observers_)
    observer.OnHintsProcessed(config, component_info);
}

}  // namespace optimization_guide
