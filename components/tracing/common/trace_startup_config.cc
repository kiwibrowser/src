// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/tracing/common/trace_startup_config.h"

#include <stddef.h>

#include <memory>
#include <string>

#include "base/command_line.h"
#include "base/files/file_util.h"
#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/memory/singleton.h"
#include "base/strings/string_number_conversions.h"
#include "base/values.h"
#include "build/build_config.h"
#include "components/tracing/common/tracing_switches.h"

namespace tracing {

namespace {

// Maximum trace config file size that will be loaded, in bytes.
const size_t kTraceConfigFileSizeLimit = 64 * 1024;

// Trace config file path:
// - Android: /data/local/chrome-trace-config.json
// - Others: specified by --trace-config-file flag.
#if defined(OS_ANDROID)
const base::FilePath::CharType kAndroidTraceConfigFile[] =
    FILE_PATH_LITERAL("/data/local/chrome-trace-config.json");
#endif

// String parameters that can be used to parse the trace config file content.
const char kTraceConfigParam[] = "trace_config";
const char kStartupDurationParam[] = "startup_duration";
const char kResultFileParam[] = "result_file";

}  // namespace

TraceStartupConfig* TraceStartupConfig::GetInstance() {
  return base::Singleton<TraceStartupConfig, base::DefaultSingletonTraits<
                                                 TraceStartupConfig>>::get();
}

TraceStartupConfig::TraceStartupConfig()
    : is_enabled_(false),
      trace_config_(base::trace_event::TraceConfig()),
      startup_duration_(0),
      result_file_() {
  auto* command_line = base::CommandLine::ForCurrentProcess();

  if (command_line->HasSwitch(switches::kTraceStartup)) {
    std::string startup_duration_str =
        command_line->GetSwitchValueASCII(switches::kTraceStartupDuration);
    startup_duration_ = 5;
    if (!startup_duration_str.empty() &&
        !base::StringToInt(startup_duration_str, &startup_duration_)) {
      DLOG(WARNING) << "Could not parse --" << switches::kTraceStartupDuration
                    << "=" << startup_duration_str << " defaulting to 5 (secs)";
      startup_duration_ = 5;
    }

    trace_config_ = base::trace_event::TraceConfig(
        command_line->GetSwitchValueASCII(switches::kTraceStartup),
        command_line->GetSwitchValueASCII(switches::kTraceStartupRecordMode));

    result_file_ =
        command_line->GetSwitchValuePath(switches::kTraceStartupFile);

    is_enabled_ = true;
    return;
  }

#if defined(OS_ANDROID)
  base::FilePath trace_config_file(kAndroidTraceConfigFile);
#else
  if (!command_line->HasSwitch(switches::kTraceConfigFile))
    return;
  base::FilePath trace_config_file =
      command_line->GetSwitchValuePath(switches::kTraceConfigFile);
#endif

  if (trace_config_file.empty()) {
    // If the trace config file path is not specified, trace Chrome with the
    // default configuration for 5 sec.
    startup_duration_ = 5;
    is_enabled_ = true;
    DLOG(WARNING) << "Use default trace config.";
    return;
  }

  if (!base::PathExists(trace_config_file)) {
    DLOG(WARNING) << "The trace config file does not exist.";
    return;
  }

  std::string trace_config_file_content;
  if (!base::ReadFileToStringWithMaxSize(trace_config_file,
                                         &trace_config_file_content,
                                         kTraceConfigFileSizeLimit)) {
    DLOG(WARNING) << "Cannot read the trace config file correctly.";
    return;
  }
  is_enabled_ = ParseTraceConfigFileContent(trace_config_file_content);
  if (!is_enabled_)
    DLOG(WARNING) << "Cannot parse the trace config file correctly.";
}

TraceStartupConfig::~TraceStartupConfig() = default;

bool TraceStartupConfig::ParseTraceConfigFileContent(
    const std::string& content) {
  std::unique_ptr<base::Value> value(base::JSONReader::Read(content));
  if (!value || !value->is_dict())
    return false;

  std::unique_ptr<base::DictionaryValue> dict(
      static_cast<base::DictionaryValue*>(value.release()));

  base::DictionaryValue* trace_config_dict = nullptr;
  if (!dict->GetDictionary(kTraceConfigParam, &trace_config_dict))
    return false;

  trace_config_ = base::trace_event::TraceConfig(*trace_config_dict);

  if (!dict->GetInteger(kStartupDurationParam, &startup_duration_))
    startup_duration_ = 0;

  if (startup_duration_ < 0)
    startup_duration_ = 0;

  base::FilePath::StringType result_file_str;
  if (dict->GetString(kResultFileParam, &result_file_str))
    result_file_ = base::FilePath(result_file_str);

  return true;
}

bool TraceStartupConfig::IsEnabled() const {
  return is_enabled_;
}

void TraceStartupConfig::SetDisabled() {
  is_enabled_ = false;
}

bool TraceStartupConfig::IsTracingStartupForDuration() const {
  return is_enabled_ && startup_duration_ > 0;
}

base::trace_event::TraceConfig TraceStartupConfig::GetTraceConfig() const {
  DCHECK(IsEnabled());
  return trace_config_;
}

int TraceStartupConfig::GetStartupDuration() const {
  DCHECK(IsEnabled());
  return startup_duration_;
}

#if !defined(OS_ANDROID)
base::FilePath TraceStartupConfig::GetResultFile() const {
  DCHECK(IsEnabled());
  return result_file_;
}
#endif

}  // namespace tracing
