// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "webrunner/app/webrunner_main_delegate.h"

#include "base/base_paths.h"
#include "base/command_line.h"
#include "base/path_service.h"
#include "content/public/common/content_switches.h"
#include "ui/base/resource/resource_bundle.h"
#include "webrunner/browser/webrunner_browser_main.h"
#include "webrunner/browser/webrunner_content_browser_client.h"
#include "webrunner/common/webrunner_content_client.h"

namespace webrunner {

namespace {

void InitLoggingFromCommandLine(const base::CommandLine& command_line) {
  base::FilePath log_filename;
  std::string filename = command_line.GetSwitchValueASCII(switches::kLogFile);
  if (filename.empty()) {
    base::PathService::Get(base::DIR_EXE, &log_filename);
    log_filename = log_filename.AppendASCII("webrunner.log");
  } else {
    log_filename = base::FilePath::FromUTF8Unsafe(filename);
  }

  logging::LoggingSettings settings;
  settings.logging_dest = logging::LOG_TO_ALL;
  settings.log_file = log_filename.value().c_str();
  settings.delete_old = logging::DELETE_OLD_LOG_FILE;
  logging::InitLogging(settings);
  logging::SetLogItems(true /* Process ID */, true /* Thread ID */,
                       true /* Timestamp */, false /* Tick count */);
}

void InitializeResourceBundle() {
  base::FilePath pak_file;
  bool result = base::PathService::Get(base::DIR_ASSETS, &pak_file);
  DCHECK(result);
  pak_file = pak_file.Append(FILE_PATH_LITERAL("webrunner.pak"));
  ui::ResourceBundle::InitSharedInstanceWithPakPath(pak_file);
}

}  // namespace

WebRunnerMainDelegate::WebRunnerMainDelegate() = default;
WebRunnerMainDelegate::~WebRunnerMainDelegate() = default;

bool WebRunnerMainDelegate::BasicStartupComplete(int* exit_code) {
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  InitLoggingFromCommandLine(*command_line);
  content_client_ = std::make_unique<WebRunnerContentClient>();
  SetContentClient(content_client_.get());
  return false;
}

void WebRunnerMainDelegate::PreSandboxStartup() {
  InitializeResourceBundle();
}

int WebRunnerMainDelegate::RunProcess(
    const std::string& process_type,
    const content::MainFunctionParams& main_function_params) {
  if (!process_type.empty())
    return -1;

  return WebRunnerBrowserMain(main_function_params);
}

content::ContentBrowserClient*
WebRunnerMainDelegate::CreateContentBrowserClient() {
  DCHECK(!browser_client_);
  browser_client_ = std::make_unique<WebRunnerContentBrowserClient>();
  return browser_client_.get();
}

}  // namespace webrunner
