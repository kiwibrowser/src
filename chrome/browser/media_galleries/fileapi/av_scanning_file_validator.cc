// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media_galleries/fileapi/av_scanning_file_validator.h"

#if defined(OS_WIN)
#include <windows.h>
#include <objbase.h>
#include <shlobj.h>
#include <wrl/client.h>
#endif

#include "base/bind.h"
#include "base/callback.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task_runner_util.h"
#include "base/task_scheduler/post_task.h"
#include "base/threading/thread_restrictions.h"
#include "build/build_config.h"
#include "chrome/common/chrome_constants.h"
#include "content/public/browser/browser_thread.h"

using content::BrowserThread;

namespace {

#if defined(OS_WIN)
base::File::Error ScanFile(const base::FilePath& dest_platform_path) {
  base::AssertBlockingAllowed();

  Microsoft::WRL::ComPtr<IAttachmentExecute> attachment_services;
  HRESULT hr = ::CoCreateInstance(CLSID_AttachmentServices, nullptr, CLSCTX_ALL,
                                  IID_PPV_ARGS(&attachment_services));

  if (FAILED(hr)) {
    // The thread must have COM initialized.
    DCHECK_NE(CO_E_NOTINITIALIZED, hr);
    return base::File::FILE_ERROR_SECURITY;
  }

  hr = attachment_services->SetLocalPath(dest_platform_path.value().c_str());
  if (FAILED(hr))
    return base::File::FILE_ERROR_SECURITY;

  // A failure in the Save() call below could result in the downloaded file
  // being deleted.
  HRESULT scan_result = attachment_services->Save();
  if (scan_result == S_OK)
    return base::File::FILE_OK;

  return base::File::FILE_ERROR_SECURITY;
}
#endif

}  // namespace

AVScanningFileValidator::~AVScanningFileValidator() {}

void AVScanningFileValidator::StartPostWriteValidation(
    const base::FilePath& dest_platform_path,
    const ResultCallback& result_callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

#if defined(OS_WIN)
  base::PostTaskAndReplyWithResult(
      base::CreateCOMSTATaskRunnerWithTraits(
          {base::MayBlock(), base::TaskPriority::USER_VISIBLE})
          .get(),
      FROM_HERE, base::Bind(&ScanFile, dest_platform_path), result_callback);
#else
  result_callback.Run(base::File::FILE_OK);
#endif
}

AVScanningFileValidator::AVScanningFileValidator() {
}
