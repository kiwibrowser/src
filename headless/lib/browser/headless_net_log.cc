// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "headless/lib/browser/headless_net_log.h"

#include <memory>
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

namespace headless {
namespace {

std::unique_ptr<base::DictionaryValue> GetHeadlessConstants() {
  std::unique_ptr<base::DictionaryValue> constants_dict =
      net::GetNetConstants();

  // Add a dictionary with client information
  auto dict = std::make_unique<base::DictionaryValue>();

  dict->SetString("name", "headless");
  dict->SetString(
      "command_line",
      base::CommandLine::ForCurrentProcess()->GetCommandLineString());

  constants_dict->Set("clientInfo", std::move(dict));

  return constants_dict;
}

}  // namespace

HeadlessNetLog::HeadlessNetLog(const base::FilePath& log_path) {
  if (!log_path.empty()) {
    net::NetLogCaptureMode capture_mode = net::NetLogCaptureMode::Default();
    file_net_log_observer_ = net::FileNetLogObserver::CreateUnbounded(
        log_path, GetHeadlessConstants());
    file_net_log_observer_->StartObserving(this, capture_mode);
  }
}

HeadlessNetLog::~HeadlessNetLog() {
  // Remove the observer we own before we're destroyed.
  if (file_net_log_observer_)
    file_net_log_observer_->StopObserving(nullptr, base::OnceClosure());
}

}  // namespace headless
