// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/shell/browser/shell_net_log.h"

#include <stdio.h>

#include <utility>

#include "base/bind.h"
#include "base/callback.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/values.h"
#include "build/build_config.h"
#include "content/public/common/content_switches.h"
#include "net/log/file_net_log_observer.h"
#include "net/log/net_log_util.h"
#include "services/network/public/cpp/network_switches.h"

namespace content {

namespace {

std::unique_ptr<base::DictionaryValue> GetShellConstants(
    const std::string& app_name) {
  std::unique_ptr<base::DictionaryValue> constants_dict =
      net::GetNetConstants();

  // Add a dictionary with client information
  auto dict = std::make_unique<base::DictionaryValue>();

  dict->SetString("name", app_name);
  dict->SetString(
      "command_line",
      base::CommandLine::ForCurrentProcess()->GetCommandLineString());

  constants_dict->Set("clientInfo", std::move(dict));

  return constants_dict;
}

}  // namespace

ShellNetLog::ShellNetLog(const std::string& app_name) {
  const base::CommandLine* command_line =
      base::CommandLine::ForCurrentProcess();

  if (command_line->HasSwitch(network::switches::kLogNetLog)) {
    base::FilePath log_path =
        command_line->GetSwitchValuePath(network::switches::kLogNetLog);
    net::NetLogCaptureMode capture_mode = net::NetLogCaptureMode::Default();

    file_net_log_observer_ = net::FileNetLogObserver::CreateUnbounded(
        log_path, GetShellConstants(app_name));
    file_net_log_observer_->StartObserving(this, capture_mode);
  }
}

ShellNetLog::~ShellNetLog() {
  // Remove the observer we own before we're destroyed.
  if (file_net_log_observer_)
    file_net_log_observer_->StopObserving(nullptr, base::OnceClosure());
}

}  // namespace content
