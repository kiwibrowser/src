// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/services/file_util/public/cpp/zip_file_creator.h"

#include <utility>

#include "base/bind.h"
#include "base/callback_helpers.h"
#include "base/task_scheduler/post_task.h"
#include "chrome/services/file_util/public/mojom/constants.mojom.h"
#include "components/services/filesystem/directory_impl.h"
#include "components/services/filesystem/lock_table.h"
#include "content/public/browser/browser_thread.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "services/service_manager/public/cpp/connector.h"

namespace {

// Creates the destination zip file only if it does not already exist.
base::File OpenFileHandleAsync(const base::FilePath& zip_path) {
  return base::File(zip_path, base::File::FLAG_CREATE | base::File::FLAG_WRITE);
}

void BindDirectoryInBackground(
    const base::FilePath& src_dir,
    mojo::InterfaceRequest<filesystem::mojom::Directory> request) {
  auto directory_impl = std::make_unique<filesystem::DirectoryImpl>(
      src_dir, /*temp_dir=*/nullptr, /*lock_table=*/nullptr);
  mojo::MakeStrongBinding(std::move(directory_impl), std::move(request));
}

}  // namespace

ZipFileCreator::ZipFileCreator(
    const ResultCallback& callback,
    const base::FilePath& src_dir,
    const std::vector<base::FilePath>& src_relative_paths,
    const base::FilePath& dest_file)
    : callback_(callback),
      src_dir_(src_dir),
      src_relative_paths_(src_relative_paths),
      dest_file_(dest_file) {
  DCHECK(!callback_.is_null());
}

void ZipFileCreator::Start(service_manager::Connector* connector) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  // Note this class owns itself (it self-deletes when finished in ReportDone),
  // so it is safe to use base::Unretained(this).
  base::PostTaskWithTraitsAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::Bind(&OpenFileHandleAsync, dest_file_),
      base::Bind(&ZipFileCreator::CreateZipFile, base::Unretained(this),
                 base::Unretained(connector)));
}

ZipFileCreator::~ZipFileCreator() = default;

void ZipFileCreator::CreateZipFile(service_manager::Connector* connector,
                                   base::File file) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DCHECK(!zip_file_creator_ptr_);

  if (!file.IsValid()) {
    LOG(ERROR) << "Failed to create dest zip file " << dest_file_.value();
    ReportDone(false);
    return;
  }

  if (!directory_task_runner_) {
    directory_task_runner_ = base::CreateSequencedTaskRunnerWithTraits(
        {base::MayBlock(), base::TaskPriority::USER_BLOCKING,
         base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN});
  }

  filesystem::mojom::DirectoryPtr directory_ptr;
  mojo::InterfaceRequest<filesystem::mojom::Directory> request =
      mojo::MakeRequest(&directory_ptr);
  directory_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&BindDirectoryInBackground, src_dir_,
                                base::Passed(&request)));

  connector->BindInterface(chrome::mojom::kFileUtilServiceName,
                           mojo::MakeRequest(&zip_file_creator_ptr_));
  // See comment in Start() on why using base::Unretained(this) is safe.
  zip_file_creator_ptr_.set_connection_error_handler(
      base::Bind(&ZipFileCreator::ReportDone, base::Unretained(this), false));
  zip_file_creator_ptr_->CreateZipFile(
      std::move(directory_ptr), src_dir_, src_relative_paths_, std::move(file),
      base::Bind(&ZipFileCreator::ReportDone, base::Unretained(this)));
}

void ZipFileCreator::ReportDone(bool success) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  zip_file_creator_ptr_.reset();
  base::ResetAndReturn(&callback_).Run(success);

  delete this;
}
