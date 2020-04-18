// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "headless/app/shell_navigation_request.h"

#include <memory>

#include "content/public/browser/browser_thread.h"
#include "headless/app/headless_shell.h"

namespace headless {

ShellNavigationRequest::ShellNavigationRequest(
    base::WeakPtr<HeadlessShell> headless_shell,
    const std::string& interception_id)
    : headless_shell_(
          std::make_unique<base::WeakPtr<HeadlessShell>>(headless_shell)),
      interception_id_(interception_id) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
}

ShellNavigationRequest::~ShellNavigationRequest() = default;

void ShellNavigationRequest::StartProcessing(base::Closure done_callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);

  // The devtools bindings can only be called on the UI thread.
  content::BrowserThread::PostTask(
      content::BrowserThread::UI, FROM_HERE,
      base::BindOnce(&ShellNavigationRequest::StartProcessingOnUiThread,
                     std::move(headless_shell_), interception_id_,
                     std::move(done_callback)));
}

// static
void ShellNavigationRequest::StartProcessingOnUiThread(
    std::unique_ptr<base::WeakPtr<HeadlessShell>> headless_shell,
    std::string interception_id,
    base::Closure done_callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (!headless_shell)
    return;

  // Allow the navigation to proceed.
  (*headless_shell)
      ->devtools_client()
      ->GetNetwork()
      ->GetExperimental()
      ->ContinueInterceptedRequest(
          network::ContinueInterceptedRequestParams::Builder()
              .SetInterceptionId(interception_id)
              .Build(),
          base::BindOnce(
              &ShellNavigationRequest::ContinueInterceptedRequestResult,
              std::move(done_callback)));
}

// static
void ShellNavigationRequest::ContinueInterceptedRequestResult(
    base::Closure done_callback,
    std::unique_ptr<network::ContinueInterceptedRequestResult>) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  // The |done_callback| must be fired on the IO thread.
  content::BrowserThread::PostTask(
      content::BrowserThread::IO, FROM_HERE,
      base::BindOnce(
          &ShellNavigationRequest::ContinueInterceptedRequestResultOnIoThread,
          std::move(done_callback)));
}

// static
void ShellNavigationRequest::ContinueInterceptedRequestResultOnIoThread(
    base::Closure done_callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  done_callback.Run();
}

}  // namespace headless
