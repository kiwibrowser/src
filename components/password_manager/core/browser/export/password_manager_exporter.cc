// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/password_manager/core/browser/export/password_manager_exporter.h"

#include <utility>

#include "base/files/file_util.h"
#include "base/location.h"
#include "base/metrics/histogram_macros.h"
#include "base/task_runner_util.h"
#include "base/task_scheduler/lazy_task_runner.h"
#include "base/task_scheduler/post_task.h"
#include "components/autofill/core/common/password_form.h"
#include "components/password_manager/core/browser/export/password_csv_writer.h"
#include "components/password_manager/core/browser/password_manager_metrics_util.h"
#include "components/password_manager/core/browser/ui/credential_provider_interface.h"

namespace {

// Multiple exports should be queued in sequence. This helps avoid race
// conditions where there are multiple simultaneous exports to the same
// destination and one of them was cancelled and will delete the file. We use
// TaskPriority::USER_VISIBLE, because a busy UI is displayed while the
// passwords are being exported.
base::LazySingleThreadTaskRunner g_task_runner =
    LAZY_SINGLE_THREAD_TASK_RUNNER_INITIALIZER(
        base::TaskTraits(base::MayBlock(), base::TaskPriority::USER_VISIBLE),
        base::SingleThreadTaskRunnerThreadMode::SHARED);

// A wrapper for |write_function|, which can be bound and keep a copy of its
// data on the closure.
bool Write(
    password_manager::PasswordManagerExporter::WriteCallback write_function,
    const base::FilePath& destination,
    const std::string& serialised) {
  int written =
      write_function.Run(destination, serialised.c_str(), serialised.size());
  return written == static_cast<int>(serialised.size());
}

}  // namespace

namespace password_manager {

using metrics_util::ExportPasswordsResult;

PasswordManagerExporter::PasswordManagerExporter(
    password_manager::CredentialProviderInterface*
        credential_provider_interface,
    ProgressCallback on_progress)
    : credential_provider_interface_(credential_provider_interface),
      on_progress_(std::move(on_progress)),
      last_progress_status_(ExportProgressStatus::NOT_STARTED),
      write_function_(base::BindRepeating(&base::WriteFile)),
      delete_function_(base::BindRepeating(&base::DeleteFile)),
      task_runner_(g_task_runner.Get()),
      weak_factory_(this) {}

PasswordManagerExporter::~PasswordManagerExporter() {}

void PasswordManagerExporter::PreparePasswordsForExport() {
  DCHECK_EQ(GetProgressStatus(), ExportProgressStatus::NOT_STARTED);
  export_preparation_started_ = base::Time::Now();

  std::vector<std::unique_ptr<autofill::PasswordForm>> password_list =
      credential_provider_interface_->GetAllPasswords();
  size_t password_list_size = password_list.size();

  base::PostTaskAndReplyWithResult(
      task_runner_.get(), FROM_HERE,
      base::BindOnce(&password_manager::PasswordCSVWriter::SerializePasswords,
                     std::move(password_list)),
      base::BindOnce(&PasswordManagerExporter::SetSerialisedPasswordList,
                     weak_factory_.GetWeakPtr(), password_list_size));
}

void PasswordManagerExporter::SetDestination(
    const base::FilePath& destination) {
  DCHECK_EQ(GetProgressStatus(), ExportProgressStatus::NOT_STARTED);

  destination_ = destination;

  if (IsReadyForExport())
    Export();

  OnProgress(ExportProgressStatus::IN_PROGRESS, std::string());
}

void PasswordManagerExporter::SetSerialisedPasswordList(
    size_t count,
    const std::string& serialised) {
  serialised_password_list_ = serialised;
  password_count_ = count;

  UMA_HISTOGRAM_MEDIUM_TIMES("PasswordManager.TimeReadingExportedPasswords",
                             base::Time::Now() - export_preparation_started_);

  if (IsReadyForExport())
    Export();
}

void PasswordManagerExporter::Cancel() {
  // Tasks which had their pointers invalidated won't run.
  weak_factory_.InvalidateWeakPtrs();

  // If we are currently still serialising, Export() will see the cancellation
  // status and won't schedule writing.
  OnProgress(ExportProgressStatus::FAILED_CANCELLED, std::string());

  // If we are currently writing to the disk, we will have to cleanup the file
  // once writing stops.
  Cleanup();

  // TODO(crbug.com/789561) If the passwords have already been written to the
  // disk, then we've already recorded ExportPasswordsResult::SUCCESS. Ideally,
  // we should make different results mutually exclusive.
  UMA_HISTOGRAM_ENUMERATION("PasswordManager.ExportPasswordsToCSVResult",
                            ExportPasswordsResult::USER_ABORTED,
                            ExportPasswordsResult::COUNT);
}

password_manager::ExportProgressStatus
PasswordManagerExporter::GetProgressStatus() {
  return last_progress_status_;
}

void PasswordManagerExporter::SetWriteForTesting(WriteCallback write_function) {
  write_function_ = std::move(write_function);
}

void PasswordManagerExporter::SetDeleteForTesting(
    DeleteCallback delete_callback) {
  delete_function_ = std::move(delete_callback);
}

bool PasswordManagerExporter::IsReadyForExport() {
  return !destination_.empty() && !serialised_password_list_.empty();
}

void PasswordManagerExporter::Export() {
  // If cancelling was requested while we were serialising the passwords, don't
  // write anything to the disk.
  if (GetProgressStatus() == ExportProgressStatus::FAILED_CANCELLED) {
    serialised_password_list_.clear();
    return;
  }

  base::PostTaskAndReplyWithResult(
      task_runner_.get(), FROM_HERE,
      base::BindOnce(::Write, write_function_, destination_,
                     std::move(serialised_password_list_)),
      base::BindOnce(&PasswordManagerExporter::OnPasswordsExported,
                     weak_factory_.GetWeakPtr()));
}

void PasswordManagerExporter::OnPasswordsExported(
    bool success) {
  if (success) {
    OnProgress(ExportProgressStatus::SUCCEEDED, std::string());

    UMA_HISTOGRAM_ENUMERATION("PasswordManager.ExportPasswordsToCSVResult",
                              ExportPasswordsResult::SUCCESS,
                              ExportPasswordsResult::COUNT);
    UMA_HISTOGRAM_COUNTS("PasswordManager.ExportedPasswordsPerUserInCSV",
                         password_count_);
  } else {
    OnProgress(ExportProgressStatus::FAILED_WRITE_FAILED,
               destination_.DirName().BaseName().AsUTF8Unsafe());
    // Don't leave partial password files, if we tell the user we couldn't write
    Cleanup();

    UMA_HISTOGRAM_ENUMERATION("PasswordManager.ExportPasswordsToCSVResult",
                              ExportPasswordsResult::WRITE_FAILED,
                              ExportPasswordsResult::COUNT);
  }
}

void PasswordManagerExporter::OnProgress(
    password_manager::ExportProgressStatus status,
    const std::string& folder) {
  last_progress_status_ = status;
  on_progress_.Run(status, folder);
}

void PasswordManagerExporter::Cleanup() {
  // The PasswordManagerExporter instance may be destroyed before the cleanup is
  // executed, e.g. because a new export was initiated. The cleanup should be
  // carried out regardless, so we only schedule tasks which own their
  // arguments.
  // TODO(crbug.com/811779) When Chrome is overwriting an existing file, cancel
  // should restore the file rather than delete it.
  if (!destination_.empty()) {
    task_runner_->PostTask(FROM_HERE,
                           base::BindOnce(base::IgnoreResult(delete_function_),
                                          destination_, false));
  }
}

}  // namespace password_manager
