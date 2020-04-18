// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/sandbox_parameters_mac.h"

#include <unistd.h>

#include "base/command_line.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/mac/bundle_locations.h"
#include "base/mac/mac_util.h"
#include "base/numerics/checked_math.h"
#include "base/strings/stringprintf.h"
#include "base/strings/sys_string_conversions.h"
#include "base/sys_info.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/browser/plugin_service.h"
#include "content/public/common/content_client.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/pepper_plugin_info.h"
#include "sandbox/mac/seatbelt_exec.h"
#include "services/service_manager/sandbox/mac/sandbox_mac.h"
#include "services/service_manager/sandbox/switches.h"

namespace content {

namespace {

// Produce the OS version as an integer "1010", etc. and pass that to the
// profile. The profile converts the string back to a number and can do
// comparison operations on OS version.
std::string GetOSVersion() {
  int32_t major_version, minor_version, bugfix_version;
  base::SysInfo::OperatingSystemVersionNumbers(&major_version, &minor_version,
                                               &bugfix_version);
  base::CheckedNumeric<int32_t> os_version(major_version);
  os_version *= 100;
  os_version += minor_version;

  int32_t final_os_version = os_version.ValueOrDie();
  return std::to_string(final_os_version);
}

}  // namespace

void SetupCommonSandboxParameters(sandbox::SeatbeltExecClient* client) {
  const base::CommandLine* command_line =
      base::CommandLine::ForCurrentProcess();
  bool enable_logging =
      command_line->HasSwitch(service_manager::switches::kEnableSandboxLogging);

  CHECK(client->SetBooleanParameter(
      service_manager::SandboxMac::kSandboxEnableLogging, enable_logging));
  CHECK(client->SetBooleanParameter(
      service_manager::SandboxMac::kSandboxDisableDenialLogging,
      !enable_logging));

  std::string bundle_path =
      service_manager::SandboxMac::GetCanonicalPath(base::mac::MainBundlePath())
          .value();
  CHECK(client->SetParameter(service_manager::SandboxMac::kSandboxBundlePath,
                             bundle_path));

  NSBundle* bundle = base::mac::OuterBundle();
  std::string bundle_id = base::SysNSStringToUTF8([bundle bundleIdentifier]);
  CHECK(client->SetParameter(
      service_manager::SandboxMac::kSandboxChromeBundleId, bundle_id));

  CHECK(client->SetParameter(service_manager::SandboxMac::kSandboxBrowserPID,
                             std::to_string(getpid())));

  std::string logging_path =
      GetContentClient()->browser()->GetLoggingFileName(*command_line).value();
  CHECK(client->SetParameter(
      service_manager::SandboxMac::kSandboxLoggingPathAsLiteral, logging_path));

#if defined(COMPONENT_BUILD)
  // For component builds, allow access to one directory level higher, where
  // the dylibs live.
  base::FilePath component_path = base::mac::MainBundlePath().Append("..");
  std::string component_path_canonical =
      service_manager::SandboxMac::GetCanonicalPath(component_path).value();
  CHECK(client->SetParameter(service_manager::SandboxMac::kSandboxComponentPath,
                             component_path_canonical));
#endif

  CHECK(client->SetParameter(service_manager::SandboxMac::kSandboxOSVersion,
                             GetOSVersion()));

  std::string homedir =
      service_manager::SandboxMac::GetCanonicalPath(base::GetHomeDir()).value();
  CHECK(client->SetParameter(
      service_manager::SandboxMac::kSandboxHomedirAsLiteral, homedir));
}

void SetupPPAPISandboxParameters(sandbox::SeatbeltExecClient* client) {
  SetupCommonSandboxParameters(client);

  std::vector<content::WebPluginInfo> plugins;
  PluginService::GetInstance()->GetInternalPlugins(&plugins);

  base::FilePath bundle_path = service_manager::SandboxMac::GetCanonicalPath(
      base::mac::MainBundlePath());

  const std::string param_base_name = "PPAPI_PATH_";
  int index = 0;
  for (const auto& plugin : plugins) {
    // Only add plugins which are external to Chrome's bundle to the profile.
    if (!bundle_path.IsParent(plugin.path) && plugin.path.IsAbsolute()) {
      std::string param_name =
          param_base_name + base::StringPrintf("%d", index++);
      CHECK(client->SetParameter(param_name, plugin.path.value()));
    }
  }

  // The profile does not support more than 4 PPAPI plugins, but it will be set
  // to n+1 more than the plugins added.
  CHECK(index <= 5);
}

void SetupCDMSandboxParameters(sandbox::SeatbeltExecClient* client) {
  SetupCommonSandboxParameters(client);

  base::FilePath bundle_path = service_manager::SandboxMac::GetCanonicalPath(
      base::mac::FrameworkBundlePath().DirName());
  CHECK(!bundle_path.empty());

  CHECK(client->SetParameter(
      service_manager::SandboxMac::kSandboxBundleVersionPath,
      bundle_path.value()));
}

void SetupUtilitySandboxParameters(sandbox::SeatbeltExecClient* client,
                                   const base::CommandLine& command_line) {
  SetupCommonSandboxParameters(client);
}

}  // namespace content
