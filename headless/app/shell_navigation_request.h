// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef HEADLESS_APP_SHELL_NAVIGATION_REQUEST_H_
#define HEADLESS_APP_SHELL_NAVIGATION_REQUEST_H_

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "headless/public/devtools/domains/network.h"
#include "headless/public/util/navigation_request.h"

namespace headless {
class HeadlessShell;

// Used in deterministic mode to make sure navigations and resource requests
// complete in the order requested.
class ShellNavigationRequest : public NavigationRequest {
 public:
  ShellNavigationRequest(base::WeakPtr<HeadlessShell> headless_shell,
                         const std::string& interception_id);

  ~ShellNavigationRequest() override;

  void StartProcessing(base::Closure done_callback) override;

 private:
  static void StartProcessingOnUiThread(
      std::unique_ptr<base::WeakPtr<HeadlessShell>> headless_shell,
      std::string interception_id,
      base::Closure done_callback);

  // Note the navigation likely isn't done when this is called, however we
  // expect it will have been committed and the initial resource load requested.
  static void ContinueInterceptedRequestResult(
      base::Closure done_callback,
      std::unique_ptr<network::ContinueInterceptedRequestResult>);

  static void ContinueInterceptedRequestResultOnIoThread(
      base::Closure done_callback);

  // Yuck we need to post a weak pointer from the IO -> UI threads but WeakPtr
  // is super finicky about which threads it's touched on. By boxing this up in
  // a unique_ptr we can pass it about and only touch it on the UI thread.
  std::unique_ptr<base::WeakPtr<HeadlessShell>> headless_shell_;
  std::string interception_id_;
};

}  // namespace headless

#endif  // HEADLESS_APP_SHELL_NAVIGATION_REQUEST_H_
