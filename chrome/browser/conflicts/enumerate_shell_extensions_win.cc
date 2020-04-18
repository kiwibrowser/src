// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/conflicts/enumerate_shell_extensions_win.h"

#include <utility>

#include "base/files/file_path.h"
#include "base/memory/ref_counted.h"
#include "base/metrics/histogram_functions.h"
#include "base/sequenced_task_runner.h"
#include "base/strings/string16.h"
#include "base/strings/stringprintf.h"
#include "base/task_scheduler/post_task.h"
#include "base/task_scheduler/task_traits.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "base/threading/thread_restrictions.h"
#include "base/win/registry.h"
#include "chrome/browser/conflicts/module_info_util_win.h"

namespace {

void ReadShellExtensions(
    HKEY parent,
    const base::RepeatingCallback<void(const base::FilePath&)>& callback,
    int* nb_shell_extensions) {
  for (base::win::RegistryValueIterator iter(parent,
                                             kShellExtensionRegistryKey);
       iter.Valid(); ++iter) {
    base::string16 key =
        base::StringPrintf(kClassIdRegistryKeyFormat, iter.Name());

    base::win::RegKey clsid;
    if (clsid.Open(HKEY_CLASSES_ROOT, key.c_str(), KEY_READ) != ERROR_SUCCESS)
      continue;

    base::string16 dll;
    if (clsid.ReadValue(L"", &dll) != ERROR_SUCCESS)
      continue;

    nb_shell_extensions++;
    callback.Run(base::FilePath(dll));
  }
}

void OnShellExtensionPathEnumerated(
    scoped_refptr<base::SequencedTaskRunner> task_runner,
    OnShellExtensionEnumeratedCallback on_shell_extension_enumerated,
    const base::FilePath& path) {
  uint32_t size_of_image = 0;
  uint32_t time_date_stamp = 0;
  if (!GetModuleImageSizeAndTimeDateStamp(path, &size_of_image,
                                          &time_date_stamp)) {
    return;
  }

  task_runner->PostTask(
      FROM_HERE, base::BindRepeating(std::move(on_shell_extension_enumerated),
                                     path, size_of_image, time_date_stamp));
}

void EnumerateShellExtensionsOnBlockingSequence(
    scoped_refptr<base::SequencedTaskRunner> task_runner,
    OnShellExtensionEnumeratedCallback on_shell_extension_enumerated,
    base::OnceClosure on_enumeration_finished) {
  EnumerateShellExtensionPaths(
      base::BindRepeating(&OnShellExtensionPathEnumerated, task_runner,
                          std::move(on_shell_extension_enumerated)));

  task_runner->PostTask(FROM_HERE, std::move(on_enumeration_finished));
}

}  // namespace

const wchar_t kShellExtensionRegistryKey[] =
    L"Software\\Microsoft\\Windows\\CurrentVersion\\Shell Extensions\\Approved";

void EnumerateShellExtensionPaths(
    const base::RepeatingCallback<void(const base::FilePath&)>& callback) {
  base::AssertBlockingAllowed();

  int nb_shell_extensions = 0;
  ReadShellExtensions(HKEY_LOCAL_MACHINE, callback, &nb_shell_extensions);
  ReadShellExtensions(HKEY_CURRENT_USER, callback, &nb_shell_extensions);

  base::UmaHistogramCounts100("ThirdPartyModules.ShellExtensionsCount",
                              nb_shell_extensions);
}

void EnumerateShellExtensions(
    OnShellExtensionEnumeratedCallback on_shell_extension_enumerated,
    base::OnceClosure on_enumeration_finished) {
  base::PostTaskWithTraits(
      FROM_HERE,
      {base::MayBlock(), base::TaskPriority::BACKGROUND,
       base::TaskShutdownBehavior::CONTINUE_ON_SHUTDOWN},
      base::BindOnce(&EnumerateShellExtensionsOnBlockingSequence,
                     base::SequencedTaskRunnerHandle::Get(),
                     std::move(on_shell_extension_enumerated),
                     std::move(on_enumeration_finished)));
}
