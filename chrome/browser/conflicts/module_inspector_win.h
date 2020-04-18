// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CONFLICTS_MODULE_INSPECTOR_WIN_H_
#define CHROME_BROWSER_CONFLICTS_MODULE_INSPECTOR_WIN_H_

#include <memory>

#include "base/callback.h"
#include "base/containers/queue.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/task_scheduler/task_traits.h"
#include "base/threading/thread_checker.h"
#include "chrome/browser/conflicts/module_info_win.h"

namespace base {
class SequencedTaskRunner;
}

// This class takes care of inspecting several modules (identified by their
// ModuleInfoKey) and returning the result via the OnModuleInspectedCallback on
// the SequencedTaskRunner where it was created.
//
// The inspection of all modules is quite expensive in terms of resources, so it
// is done one by one, in a task with a background priority level. If needed, it
// is possible to increase the priority level of these tasks by calling
// IncreaseInspectionPriority().
//
// This class is not thread safe and it enforces safety via a ThreadChecker.
class ModuleInspector {
 public:
  using OnModuleInspectedCallback =
      base::Callback<void(const ModuleInfoKey& module_key,
                          std::unique_ptr<ModuleInspectionResult>)>;

  explicit ModuleInspector(
      const OnModuleInspectedCallback& on_module_inspected_callback);
  ~ModuleInspector();

  // Adds the module to the queue of modules to inspect. Starts the inspection
  // process if the |queue_| is empty.
  void AddModule(const ModuleInfoKey& module_key);

  // Removes the throttling.
  void IncreaseInspectionPriority();

  // Returns true if ModuleInspector is not doing anything right now.
  bool IsIdle();

 private:
  // Starts inspecting the module at the front of the queue.
  void StartInspectingModule();

  // Called back on the execution context on which the ModuleInspector was
  // created when a module has finished being inspected. The callback will be
  // executed and, if the |queue_| is not empty, the next module will be sent
  // for inspection.
  void OnInspectionFinished(
      const ModuleInfoKey& module_key,
      std::unique_ptr<ModuleInspectionResult> inspection_result);

  OnModuleInspectedCallback on_module_inspected_callback_;

  // The modules are put in queue until they are sent for inspection.
  base::queue<ModuleInfoKey> queue_;

  // The task runner where module inspections takes place. It originally starts
  // at BACKGROUND priority, but is changed to USER_VISIBLE when
  // IncreaseInspectionPriority() is called.
  scoped_refptr<base::SequencedTaskRunner> task_runner_;

  // The vector of paths to %env_var%, used to account for differences in
  // localization and where people keep their files.
  // e.g. c:\windows vs d:\windows
  scoped_refptr<base::RefCountedData<StringMapping>> path_mapping_;

  base::ThreadChecker thread_checker_;

  // Weak pointers are used to safely post the inspection result back to the
  // ModuleInspector from the task scheduler.
  base::WeakPtrFactory<ModuleInspector> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(ModuleInspector);
};

#endif  // CHROME_BROWSER_CONFLICTS_MODULE_INSPECTOR_WIN_H_
