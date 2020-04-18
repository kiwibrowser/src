// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/conflicts/module_inspector_win.h"

#include <utility>

#include "base/bind.h"
#include "base/sequenced_task_runner.h"
#include "base/task_scheduler/post_task.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "chrome/browser/after_startup_task_utils.h"

namespace {

StringMapping GetPathMapping() {
  return GetEnvironmentVariablesMapping({
      L"LOCALAPPDATA", L"ProgramFiles", L"ProgramData", L"USERPROFILE",
      L"SystemRoot", L"TEMP", L"TMP", L"CommonProgramFiles",
  });
}

// Does the inspection of the module and replies with the result by calling
// |on_inspection_finished_callback| on |reply_task_runner|.
// The StringMapping is wrapped in a RefCountedData to save a copy per
// invocation.
//
// TODO(pmonette): When the Task Scheduler starts supporting after-startup
// background sequences, change this to use base::PostTaskAndReplyWithResult().
void InspectModuleOnBlockingSequenceAndReply(
    scoped_refptr<base::RefCountedData<StringMapping>> env_variable_mapping,
    const ModuleInfoKey& module_key,
    scoped_refptr<base::SequencedTaskRunner> reply_task_runner,
    base::OnceCallback<void(std::unique_ptr<ModuleInspectionResult>)>
        on_inspection_finished_callback) {
  reply_task_runner->PostTask(
      FROM_HERE,
      base::BindOnce(std::move(on_inspection_finished_callback),
                     InspectModule(env_variable_mapping->data, module_key)));
}

}  // namespace

ModuleInspector::ModuleInspector(
    const OnModuleInspectedCallback& on_module_inspected_callback)
    : on_module_inspected_callback_(on_module_inspected_callback),
      task_runner_(base::CreateSequencedTaskRunnerWithTraits(
          {base::MayBlock(), base::TaskPriority::BACKGROUND,
           base::TaskShutdownBehavior::CONTINUE_ON_SHUTDOWN})),
      path_mapping_(base::MakeRefCounted<base::RefCountedData<StringMapping>>(
          GetPathMapping())),
      weak_ptr_factory_(this) {}

ModuleInspector::~ModuleInspector() = default;

void ModuleInspector::AddModule(const ModuleInfoKey& module_key) {
  DCHECK(thread_checker_.CalledOnValidThread());
  queue_.push(module_key);
  if (queue_.size() == 1)
    StartInspectingModule();
}

void ModuleInspector::IncreaseInspectionPriority() {
  DCHECK(thread_checker_.CalledOnValidThread());
  // Create a task runner with higher priority so that future inspections are
  // done faster.
  task_runner_ = base::CreateSequencedTaskRunnerWithTraits(
      {base::MayBlock(), base::TaskPriority::USER_VISIBLE,
       base::TaskShutdownBehavior::CONTINUE_ON_SHUTDOWN});
}

bool ModuleInspector::IsIdle() {
  return queue_.empty();
}

void ModuleInspector::StartInspectingModule() {
  ModuleInfoKey module_key = queue_.front();

  // There is a small priority inversion that happens when
  // IncreaseInspectionPriority() is called while a module is currently being
  // inspected.
  //
  // This is because all the subsequent tasks will be posted at a higher
  // priority, but they are waiting on the current task that is currently
  // running at a lower priority.
  //
  // In practice, this is not an issue because the only caller of
  // IncreaseInspectionPriority() (chrome://conflicts) does not depend on the
  // inspection to finish synchronously and is not blocking anything else.
  AfterStartupTaskUtils::PostTask(
      FROM_HERE, task_runner_,
      base::BindOnce(
          &InspectModuleOnBlockingSequenceAndReply, path_mapping_, module_key,
          base::SequencedTaskRunnerHandle::Get(),
          base::BindOnce(&ModuleInspector::OnInspectionFinished,
                         weak_ptr_factory_.GetWeakPtr(), module_key)));
}

void ModuleInspector::OnInspectionFinished(
    const ModuleInfoKey& module_key,
    std::unique_ptr<ModuleInspectionResult> inspection_result) {
  DCHECK(thread_checker_.CalledOnValidThread());

  // Pop first, because the callback may want to know if there is any work left
  // to be done, which is caracterized by a non-empty queue.
  queue_.pop();

  on_module_inspected_callback_.Run(module_key, std::move(inspection_result));

  // Continue the work.
  if (!queue_.empty())
    StartInspectingModule();
}
