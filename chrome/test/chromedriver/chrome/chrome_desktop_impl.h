// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_TEST_CHROMEDRIVER_CHROME_CHROME_DESKTOP_IMPL_H_
#define CHROME_TEST_CHROMEDRIVER_CHROME_CHROME_DESKTOP_IMPL_H_

#include <memory>
#include <string>
#include <vector>

#include "base/command_line.h"
#include "base/compiler_specific.h"
#include "base/process/process.h"
#include "base/values.h"
#include "chrome/test/chromedriver/chrome/chrome_impl.h"
#include "chrome/test/chromedriver/chrome/scoped_temp_dir_with_retry.h"

namespace base {
class TimeDelta;
}

class AutomationExtension;
class DevToolsClient;
class DevToolsHttpClient;
class Status;
class WebView;

class ChromeDesktopImpl : public ChromeImpl {
 public:
  ChromeDesktopImpl(std::unique_ptr<DevToolsHttpClient> http_client,
                    std::unique_ptr<DevToolsClient> websocket_client,
                    std::vector<std::unique_ptr<DevToolsEventListener>>
                        devtools_event_listeners,
                    std::string page_load_strategy,
                    base::Process process,
                    const base::CommandLine& command,
                    base::ScopedTempDir* user_data_dir,
                    base::ScopedTempDir* extension_dir,
                    bool network_emulation_enabled);
  ~ChromeDesktopImpl() override;

  // Waits for a page with the given URL to appear and finish loading.
  // Returns an error if the timeout is exceeded.
  Status WaitForPageToLoad(const std::string& url,
                           const base::TimeDelta& timeout,
                           std::unique_ptr<WebView>* web_view,
                           bool w3c_compliant);

  // Gets the installed automation extension.
  Status GetAutomationExtension(AutomationExtension** extension,
                                bool w3c_compliant);

  // Overridden from Chrome:
  Status GetAsDesktop(ChromeDesktopImpl** desktop) override;
  std::string GetOperatingSystemName() override;

  // Overridden from ChromeImpl:
  bool IsMobileEmulationEnabled() const override;
  bool HasTouchScreen() const override;
  Status QuitImpl() override;

  const base::CommandLine& command() const;
  bool IsNetworkConnectionEnabled() const;

  int GetNetworkConnection() const;
  void SetNetworkConnection(int network_connection);

  Status GetWindowPosition(const std::string& target_id, int* x, int* y);
  Status GetWindowSize(const std::string& target_id, int* width, int* height);
  Status SetWindowRect(const std::string& target_id,
                       const base::DictionaryValue& params);
  Status SetWindowPosition(const std::string& target_id, int x, int y);
  Status SetWindowSize(const std::string& target_id, int width, int height);
  Status MaximizeWindow(const std::string& target_id);
  Status MinimizeWindow(const std::string& target_id);
  Status FullScreenWindow(const std::string& target_id);

 private:
  struct Window {
    int id;
    std::string state;
    int left;
    int top;
    int width;
    int height;
  };
  Status ParseWindowBounds(std::unique_ptr<base::DictionaryValue> params,
                           Window* window);
  Status ParseWindow(std::unique_ptr<base::DictionaryValue> params,
                     Window* window);

  Status GetWindow(const std::string& target_id, Window* window);
  Status GetWindowBounds(int window_id, Window* window);
  Status SetWindowBounds(int window_id,
                         std::unique_ptr<base::DictionaryValue> bounds);

  base::Process process_;
  base::CommandLine command_;
  ScopedTempDirWithRetry user_data_dir_;
  ScopedTempDirWithRetry extension_dir_;
  bool network_connection_enabled_;
  int network_connection_;

  // Lazily initialized, may be null.
  std::unique_ptr<AutomationExtension> automation_extension_;
};

#endif  // CHROME_TEST_CHROMEDRIVER_CHROME_CHROME_DESKTOP_IMPL_H_
