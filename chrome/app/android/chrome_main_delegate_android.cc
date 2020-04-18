// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/app/android/chrome_main_delegate_android.h"

#include "base/android/jni_android.h"
#include "base/base_paths_android.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "base/trace_event/trace_event.h"
#include "chrome/browser/android/chrome_startup_flags.h"
#include "chrome/browser/android/metrics/uma_utils.h"
#include "chrome/browser/media/android/remote/remote_media_player_manager.h"
#include "components/policy/core/browser/android/android_combined_policy_provider.h"
#include "components/safe_browsing/android/safe_browsing_api_handler.h"
#include "components/safe_browsing/android/safe_browsing_api_handler_bridge.h"
#include "components/startup_metric_utils/browser/startup_metric_utils.h"
#include "content/browser/media/android/browser_media_player_manager.h"
#include "content/public/browser/browser_main_runner.h"

using safe_browsing::SafeBrowsingApiHandler;

namespace {

content::BrowserMediaPlayerManager* CreateRemoteMediaPlayerManager(
    content::RenderFrameHost* render_frame_host) {
  return new remote_media::RemoteMediaPlayerManager(render_frame_host);
}

} // namespace

// ChromeMainDelegateAndroid is created when the library is loaded. It is always
// done in the process's main Java thread. But for non browser process, e.g.
// renderer process, it is not the native Chrome's main thread.
ChromeMainDelegateAndroid::ChromeMainDelegateAndroid() {
}

ChromeMainDelegateAndroid::~ChromeMainDelegateAndroid() {
}

bool ChromeMainDelegateAndroid::BasicStartupComplete(int* exit_code) {
  safe_browsing_api_handler_.reset(
      new safe_browsing::SafeBrowsingApiHandlerBridge());
  SafeBrowsingApiHandler::SetInstance(safe_browsing_api_handler_.get());

  policy::android::AndroidCombinedPolicyProvider::SetShouldWaitForPolicy(true);
  SetChromeSpecificCommandLineFlags();
  content::BrowserMediaPlayerManager::RegisterFactory(
      &CreateRemoteMediaPlayerManager);

  return ChromeMainDelegate::BasicStartupComplete(exit_code);
}

void ChromeMainDelegateAndroid::SandboxInitialized(
    const std::string& process_type) {
  ChromeMainDelegate::SandboxInitialized(process_type);
}

void ChromeMainDelegateAndroid::SecureDataDirectory() {
  // By default, Android creates the directory accessible by others.
  // We'd like to tighten security and make it accessible only by
  // the browser process.
  // TODO(crbug.com/832388): Remove this once minsdk >= 21,
  // at which point this will be handled by PathUtils.java.
  base::FilePath data_path;
  bool ok = base::PathService::Get(base::DIR_ANDROID_APP_DATA, &data_path);
  if (ok) {
    ok = base::SetPosixFilePermissions(data_path,
                                       base::FILE_PERMISSION_USER_MASK);
  }
  if (!ok) {
    LOG(ERROR) << "Failed to set data directory permissions";
  }
}

int ChromeMainDelegateAndroid::RunProcess(
    const std::string& process_type,
    const content::MainFunctionParams& main_function_params) {
  TRACE_EVENT0("startup", "ChromeMainDelegateAndroid::RunProcess")
  if (process_type.empty()) {
    SecureDataDirectory();

    // Because the browser process can be started asynchronously as a series of
    // UI thread tasks a second request to start it can come in while the
    // first request is still being processed. Chrome must keep the same
    // browser runner for the second request.
    // Also only record the start time the first time round, since this is the
    // start time of the application, and will be same for all requests.
    if (!browser_runner_.get()) {
      startup_metric_utils::RecordMainEntryPointTime(
          chrome::android::GetMainEntryPointTimeWallClock(),
          chrome::android::GetMainEntryPointTimeTicks());
      browser_runner_.reset(content::BrowserMainRunner::Create());
    }
    return browser_runner_->Initialize(main_function_params);
  }

  return ChromeMainDelegate::RunProcess(process_type, main_function_params);
}

void ChromeMainDelegateAndroid::ProcessExiting(
    const std::string& process_type) {
  SafeBrowsingApiHandler::SetInstance(nullptr);
}
