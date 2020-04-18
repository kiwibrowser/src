// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/loader/temporary_file_stream.h"

#include <utility>

#include "base/bind.h"
#include "base/callback.h"
#include "base/files/file_proxy.h"
#include "base/memory/ref_counted.h"
#include "base/sequenced_task_runner.h"
#include "base/task_scheduler/post_task.h"
#include "content/public/browser/browser_thread.h"
#include "net/base/file_stream.h"
#include "storage/browser/blob/shareable_file_reference.h"

using storage::ShareableFileReference;

namespace content {

namespace {

void DidCreateTemporaryFile(
    const CreateTemporaryFileStreamCallback& callback,
    std::unique_ptr<base::FileProxy> file_proxy,
    scoped_refptr<base::SequencedTaskRunner> task_runner,
    base::File::Error error_code,
    const base::FilePath& file_path) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  if (!file_proxy->IsValid()) {
    callback.Run(error_code, std::unique_ptr<net::FileStream>(), NULL);
    return;
  }

  // Cancelled or not, create the deletable_file so the temporary is cleaned up.
  scoped_refptr<ShareableFileReference> deletable_file =
      ShareableFileReference::GetOrCreate(
          file_path,
          ShareableFileReference::DELETE_ON_FINAL_RELEASE,
          task_runner.get());

  std::unique_ptr<net::FileStream> file_stream(
      new net::FileStream(file_proxy->TakeFile(), task_runner));

  callback.Run(error_code, std::move(file_stream), deletable_file.get());
}

}  // namespace

void CreateTemporaryFileStream(
    const CreateTemporaryFileStreamCallback& callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  scoped_refptr<base::SequencedTaskRunner> task_runner =
      base::CreateSequencedTaskRunnerWithTraits(
          {base::MayBlock(), base::TaskPriority::USER_VISIBLE,
           base::TaskShutdownBehavior::BLOCK_SHUTDOWN});

  std::unique_ptr<base::FileProxy> file_proxy(
      new base::FileProxy(task_runner.get()));
  base::FileProxy* proxy = file_proxy.get();
  proxy->CreateTemporary(
      base::File::FLAG_ASYNC,
      base::BindOnce(&DidCreateTemporaryFile, callback, std::move(file_proxy),
                     std::move(task_runner)));
}

}  // namespace content
