// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/test/chromedriver/chrome_launcher.h"

#include <stddef.h>
#include <stdint.h>

#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

#include "base/base64.h"
#include "base/bind.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_file.h"
#include "base/format_macros.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/process/kill.h"
#include "base/process/launch.h"
#include "base/process/process.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/platform_thread.h"
#include "base/time/time.h"
#include "base/values.h"
#include "build/build_config.h"
#include "chrome/common/chrome_constants.h"
#include "chrome/test/chromedriver/chrome/chrome_android_impl.h"
#include "chrome/test/chromedriver/chrome/chrome_desktop_impl.h"
#include "chrome/test/chromedriver/chrome/chrome_finder.h"
#include "chrome/test/chromedriver/chrome/chrome_remote_impl.h"
#include "chrome/test/chromedriver/chrome/device_manager.h"
#include "chrome/test/chromedriver/chrome/devtools_client_impl.h"
#include "chrome/test/chromedriver/chrome/devtools_event_listener.h"
#include "chrome/test/chromedriver/chrome/devtools_http_client.h"
#include "chrome/test/chromedriver/chrome/embedded_automation_extension.h"
#include "chrome/test/chromedriver/chrome/status.h"
#include "chrome/test/chromedriver/chrome/user_data_dir.h"
#include "chrome/test/chromedriver/chrome/version.h"
#include "chrome/test/chromedriver/chrome/web_view.h"
#include "chrome/test/chromedriver/net/net_util.h"
#include "chrome/test/chromedriver/net/url_request_context_getter.h"
#include "crypto/rsa_private_key.h"
#include "crypto/sha2.h"
#include "third_party/zlib/google/zip.h"
#include "url/gurl.h"

#if defined(OS_POSIX)
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#elif defined(OS_WIN)
#include "chrome/test/chromedriver/keycode_text_conversion.h"
#endif

namespace {

// TODO(eseckler): Remove --ignore-certificate-errors for newer Chrome versions
// that support the Security DevTools domain on the browser target.
const char* const kCommonSwitches[] = {
    "disable-popup-blocking", "enable-automation", "ignore-certificate-errors",
    "metrics-recording-only",
};

const char* const kDesktopSwitches[] = {
    "disable-hang-monitor",
    "disable-prompt-on-repost",
    "disable-sync",
    "no-first-run",
    "disable-background-networking",
    "disable-web-resources",
    "disable-client-side-phishing-detection",
    "disable-default-apps",
    "enable-logging",
    "log-level=0",
    "password-store=basic",
    "use-mock-keychain",
    "test-type=webdriver",
    "force-fieldtrials=SiteIsolationExtensions/Control",
};

const char* const kAndroidSwitches[] = {
    "disable-fre", "enable-remote-debugging",
};

#if defined(OS_LINUX)
const char kEnableCrashReport[] = "enable-crash-reporter-for-testing";
#endif
const base::FilePath::CharType kDevToolsActivePort[] =
    FILE_PATH_LITERAL("DevToolsActivePort");

Status UnpackAutomationExtension(const base::FilePath& temp_dir,
                                 base::FilePath* automation_extension) {
  std::string decoded_extension;
  if (!base::Base64Decode(kAutomationExtension, &decoded_extension))
    return Status(kUnknownError, "failed to base64decode automation extension");

  base::FilePath extension_zip = temp_dir.AppendASCII("internal.zip");
  int size = static_cast<int>(decoded_extension.length());
  if (base::WriteFile(extension_zip, decoded_extension.c_str(), size)
      != size) {
    return Status(kUnknownError, "failed to write automation extension zip");
  }

  base::FilePath extension_dir = temp_dir.AppendASCII("internal");
  if (!zip::Unzip(extension_zip, extension_dir))
    return Status(kUnknownError, "failed to unzip automation extension");

  *automation_extension = extension_dir;
  return Status(kOk);
}

Status PrepareCommandLine(const Capabilities& capabilities,
                          base::CommandLine* prepared_command,
                          base::ScopedTempDir* user_data_dir_temp_dir,
                          base::ScopedTempDir* extension_dir,
                          std::vector<std::string>* extension_bg_pages,
                          base::FilePath* user_data_dir) {
  base::FilePath program = capabilities.binary;
  if (program.empty()) {
    if (!FindChrome(&program))
      return Status(kUnknownError, "cannot find Chrome binary");
  } else if (!base::PathExists(program)) {
    return Status(kUnknownError,
                  base::StringPrintf("no chrome binary at %" PRFilePath,
                                     program.value().c_str()));
  }
  base::CommandLine command(program);
  Switches switches;

  for (auto* common_switch : kCommonSwitches)
    switches.SetUnparsedSwitch(common_switch);
  for (auto* desktop_switch : kDesktopSwitches)
    switches.SetUnparsedSwitch(desktop_switch);
  switches.SetSwitch("remote-debugging-port", "0");
  for (const auto& excluded_switch : capabilities.exclude_switches) {
    switches.RemoveSwitch(excluded_switch);
  }
  switches.SetFromSwitches(capabilities.switches);

  if (capabilities.exclude_switches.count("user-data-dir") > 0)
    LOG(WARNING) << "excluding user-data-dir switch is not supported";
  if (switches.HasSwitch("user-data-dir")) {
    *user_data_dir =
        base::FilePath(switches.GetSwitchValueNative("user-data-dir"));
  } else {
    command.AppendArg("data:,");
    if (!user_data_dir_temp_dir->CreateUniqueTempDir())
      return Status(kUnknownError, "cannot create temp dir for user data dir");
    switches.SetSwitch("user-data-dir",
                       user_data_dir_temp_dir->GetPath().value());
    *user_data_dir = user_data_dir_temp_dir->GetPath();
  }

  Status status = internal::PrepareUserDataDir(
      *user_data_dir, capabilities.prefs.get(), capabilities.local_state.get());
  if (status.IsError())
    return status;

  if (capabilities.exclude_switches.count("load-extension") > 0) {
    if (capabilities.extensions.size() > 0)
      return Status(
          kUnknownError,
          "cannot exclude load-extension switch when extensions are specified");
  } else {
    if (!extension_dir->CreateUniqueTempDir()) {
      return Status(kUnknownError,
                    "cannot create temp dir for unpacking extensions");
    }
    status = internal::ProcessExtensions(
        capabilities.extensions, extension_dir->GetPath(),
        capabilities.use_automation_extension, &switches, extension_bg_pages);
    if (status.IsError())
      return status;
  }
  switches.AppendToCommandLine(&command);
  *prepared_command = command;
  return Status(kOk);
}

Status WaitForDevToolsAndCheckVersion(
    const NetAddress& address,
    URLRequestContextGetter* context_getter,
    const SyncWebSocketFactory& socket_factory,
    const Capabilities* capabilities,
    std::unique_ptr<DevToolsHttpClient>* user_client) {
  std::unique_ptr<DeviceMetrics> device_metrics;
  if (capabilities && capabilities->device_metrics)
    device_metrics.reset(new DeviceMetrics(*capabilities->device_metrics));

  std::unique_ptr<std::set<WebViewInfo::Type>> window_types;
  if (capabilities && !capabilities->window_types.empty()) {
    window_types.reset(
        new std::set<WebViewInfo::Type>(capabilities->window_types));
  } else {
    window_types.reset(new std::set<WebViewInfo::Type>());
  }

  std::unique_ptr<DevToolsHttpClient> client(new DevToolsHttpClient(
      address, context_getter, socket_factory, std::move(device_metrics),
      std::move(window_types), capabilities->page_load_strategy));
  base::TimeTicks deadline =
      base::TimeTicks::Now() + base::TimeDelta::FromSeconds(60);
  Status status = client->Init(deadline - base::TimeTicks::Now());
  if (status.IsError())
    return status;

  const BrowserInfo* browser_info = client->browser_info();
  if (browser_info->is_android &&
      browser_info->android_package != capabilities->android_package) {
    // DevTools from Chrome 30 and earlier did not provide an Android-Package
    // key, so skip the package check for WebView on KitKat and older.
    // TODO(samuong): Make this unconditional once we stop supporting Android
    // KitKat WebView apps.
    if (!(browser_info->browser_name == "webview" &&
          browser_info->major_version <= 30 &&
          browser_info->android_package.empty())) {
      return Status(
          kSessionNotCreatedException,
          base::StringPrintf("please close '%s' and try again",
                             browser_info->android_package.c_str()));
    }
  }

  base::CommandLine* cmd_line = base::CommandLine::ForCurrentProcess();
  if (cmd_line->HasSwitch("disable-build-check")) {
    LOG(WARNING) << "You are using an unsupported command-line switch: "
                    "--disable-build-check. Please don't report bugs that "
                    "cannot be reproduced with this switch removed.";
  } else if (browser_info->build_no < kMinimumSupportedChromeBuildNo) {
    return Status(
        kSessionNotCreatedException,
        "Chrome version must be >= " + GetMinimumSupportedChromeVersion());
  }

  while (base::TimeTicks::Now() < deadline) {
    WebViewsInfo views_info;
    client->GetWebViewsInfo(&views_info);
    for (size_t i = 0; i < views_info.GetSize(); ++i) {
      if (views_info.Get(i).type == WebViewInfo::kPage) {
        *user_client = std::move(client);
        return Status(kOk);
      }
    }
    base::PlatformThread::Sleep(base::TimeDelta::FromMilliseconds(50));
  }
  return Status(kUnknownError, "unable to discover open pages");
}

Status CreateBrowserwideDevToolsClientAndConnect(
    const NetAddress& address,
    const PerfLoggingPrefs& perf_logging_prefs,
    const SyncWebSocketFactory& socket_factory,
    const std::vector<std::unique_ptr<DevToolsEventListener>>&
        devtools_event_listeners,
    const std::string& web_socket_url,
    std::unique_ptr<DevToolsClient>* browser_client) {
  std::string url(web_socket_url);
  if (url.length() == 0) {
    url = base::StringPrintf("ws://%s/devtools/browser/",
                             address.ToString().c_str());
  }
  std::unique_ptr<DevToolsClient> client(new DevToolsClientImpl(
      socket_factory, url, DevToolsClientImpl::kBrowserwideDevToolsClientId));
  for (const auto& listener : devtools_event_listeners) {
    // Only add listeners that subscribe to the browser-wide |DevToolsClient|.
    // Otherwise, listeners will think this client is associated with a webview,
    // and will send unrecognized commands to it.
    if (listener->subscribes_to_browser())
      client->AddListener(listener.get());
  }
  // Provide the client regardless of whether it connects, so that Chrome always
  // has a valid |devtools_websocket_client_|. If not connected, no listeners
  // will be notified, and client will just return kDisconnected errors if used.
  *browser_client = std::move(client);
  // To avoid unnecessary overhead, only connect if tracing is enabled, since
  // the browser-wide client is currently only used for tracing.
  if (!perf_logging_prefs.trace_categories.empty()) {
    Status status = (*browser_client)->ConnectIfNecessary();
    if (status.IsError())
      return status;
  }
  return Status(kOk);
}

Status LaunchRemoteChromeSession(
    URLRequestContextGetter* context_getter,
    const SyncWebSocketFactory& socket_factory,
    const Capabilities& capabilities,
    std::vector<std::unique_ptr<DevToolsEventListener>>
        devtools_event_listeners,
    std::unique_ptr<Chrome>* chrome) {
  Status status(kOk);
  std::unique_ptr<DevToolsHttpClient> devtools_http_client;
  status = WaitForDevToolsAndCheckVersion(
      capabilities.debugger_address, context_getter, socket_factory,
      &capabilities, &devtools_http_client);
  if (status.IsError()) {
    return Status(kUnknownError, "cannot connect to chrome at " +
                      capabilities.debugger_address.ToString(),
                  status);
  }

  std::unique_ptr<DevToolsClient> devtools_websocket_client;
  status = CreateBrowserwideDevToolsClientAndConnect(
      capabilities.debugger_address, capabilities.perf_logging_prefs,
      socket_factory, devtools_event_listeners,
      devtools_http_client->browser_info()->web_socket_url,
      &devtools_websocket_client);
  if (status.IsError()) {
    LOG(WARNING) << "Browser-wide DevTools client failed to connect: "
                 << status.message();
  }

  chrome->reset(new ChromeRemoteImpl(
      std::move(devtools_http_client), std::move(devtools_websocket_client),
      std::move(devtools_event_listeners), capabilities.page_load_strategy));
  return Status(kOk);
}

Status LaunchDesktopChrome(URLRequestContextGetter* context_getter,
                           const SyncWebSocketFactory& socket_factory,
                           const Capabilities& capabilities,
                           std::vector<std::unique_ptr<DevToolsEventListener>>
                               devtools_event_listeners,
                           std::unique_ptr<Chrome>* chrome,
                           bool w3c_compliant) {
  base::CommandLine command(base::CommandLine::NO_PROGRAM);
  base::ScopedTempDir user_data_dir_temp_dir;
  base::FilePath user_data_dir;
  base::ScopedTempDir extension_dir;
  Status status = Status(kOk);
  std::vector<std::string> extension_bg_pages;

  if (capabilities.switches.HasSwitch("user-data-dir")) {
    status = internal::RemoveOldDevToolsActivePortFile(base::FilePath(
        capabilities.switches.GetSwitchValueNative("user-data-dir")));
    if (status.IsError()) {
      return status;
    }
  }
  status =
      PrepareCommandLine(capabilities, &command, &user_data_dir_temp_dir,
                         &extension_dir, &extension_bg_pages, &user_data_dir);
  if (status.IsError())
    return status;

  base::LaunchOptions options;

#if defined(OS_LINUX)
  // If minidump path is set in the capability, enable minidump for crashes.
  if (!capabilities.minidump_path.empty()) {
    VLOG(0) << "Minidump generation specified. Will save dumps to: "
            << capabilities.minidump_path;

    options.environ["CHROME_HEADLESS"] = 1;
    options.environ["BREAKPAD_DUMP_LOCATION"] = capabilities.minidump_path;

    if (!command.HasSwitch(kEnableCrashReport))
      command.AppendSwitch(kEnableCrashReport);
  }

  // We need to allow new privileges so that chrome's setuid sandbox can run.
  options.allow_new_privs = true;
#endif

#if !defined(OS_WIN)
  if (!capabilities.log_path.empty())
    options.environ["CHROME_LOG_FILE"] = capabilities.log_path;
  if (capabilities.detach)
    options.new_process_group = true;
#endif

#if defined(OS_POSIX)
  base::ScopedFD devnull;
  const base::CommandLine* cmd_line = base::CommandLine::ForCurrentProcess();
  if (!cmd_line->HasSwitch("verbose") &&
      cmd_line->GetSwitchValueASCII("log-level") != "ALL") {
    // Redirect stderr to /dev/null, so that Chrome log spew doesn't confuse
    // users.
    devnull.reset(HANDLE_EINTR(open("/dev/null", O_WRONLY)));
    if (!devnull.is_valid())
      return Status(kUnknownError, "couldn't open /dev/null");
    options.fds_to_remap.push_back(
        std::make_pair(devnull.get(), STDERR_FILENO));
  }
#elif defined(OS_WIN)
  if (!SwitchToUSKeyboardLayout())
    VLOG(0) << "Cannot switch to US keyboard layout - some keys may be "
        "interpreted incorrectly";
#endif

#if defined(OS_WIN)
  std::string command_string = base::WideToUTF8(command.GetCommandLineString());
#else
  std::string command_string = command.GetCommandLineString();
#endif
  VLOG(0) << "Launching chrome: " << command_string;
  base::Process process = base::LaunchProcess(command, options);
  if (!process.IsValid())
    return Status(kUnknownError, "chrome failed to start");

  std::unique_ptr<DevToolsHttpClient> devtools_http_client;
  int devtools_port;
  status = internal::ParseDevToolsActivePortFile(user_data_dir, &devtools_port);
  if (status.IsError()) {
    return status;
  }
  status = WaitForDevToolsAndCheckVersion(NetAddress(devtools_port),
                                          context_getter, socket_factory,
                                          &capabilities, &devtools_http_client);
  if (status.IsError()) {
    int exit_code;
    base::TerminationStatus chrome_status =
        base::GetTerminationStatus(process.Handle(), &exit_code);
    if (chrome_status != base::TERMINATION_STATUS_STILL_RUNNING) {
      std::string termination_reason;
      switch (chrome_status) {
        case base::TERMINATION_STATUS_NORMAL_TERMINATION:
          termination_reason = "exited normally";
          break;
        case base::TERMINATION_STATUS_ABNORMAL_TERMINATION:
          termination_reason = "exited abnormally";
          break;
        case base::TERMINATION_STATUS_PROCESS_WAS_KILLED:
#if defined(OS_CHROMEOS)
        case base::TERMINATION_STATUS_PROCESS_WAS_KILLED_BY_OOM:
#endif
          termination_reason = "was killed";
          break;
        case base::TERMINATION_STATUS_PROCESS_CRASHED:
          termination_reason = "crashed";
          break;
        case base::TERMINATION_STATUS_LAUNCH_FAILED:
          termination_reason = "failed to launch";
          break;
        default:
          termination_reason = "unknown";
          break;
      }
      return Status(kUnknownError,
                    "Chrome failed to start: " + termination_reason);
    }
    if (!process.Terminate(0, true)) {
      int exit_code;
      if (base::GetTerminationStatus(process.Handle(), &exit_code) ==
          base::TERMINATION_STATUS_STILL_RUNNING)
        return Status(kUnknownError, "cannot kill Chrome", status);
    }
    return status;
  }

  std::unique_ptr<DevToolsClient> devtools_websocket_client;
  status = CreateBrowserwideDevToolsClientAndConnect(
      NetAddress(devtools_port), capabilities.perf_logging_prefs,
      socket_factory, devtools_event_listeners,
      devtools_http_client->browser_info()->web_socket_url,
      &devtools_websocket_client);
  if (status.IsError()) {
    LOG(WARNING) << "Browser-wide DevTools client failed to connect: "
                 << status.message();
  }

  std::unique_ptr<ChromeDesktopImpl> chrome_desktop(new ChromeDesktopImpl(
      std::move(devtools_http_client), std::move(devtools_websocket_client),
      std::move(devtools_event_listeners), capabilities.page_load_strategy,
      std::move(process), command, &user_data_dir_temp_dir, &extension_dir,
      capabilities.network_emulation_enabled));
  if (!capabilities.extension_load_timeout.is_zero()) {
    for (size_t i = 0; i < extension_bg_pages.size(); ++i) {
      VLOG(0) << "Waiting for extension bg page load: "
              << extension_bg_pages[i];
      std::unique_ptr<WebView> web_view;
      Status status = chrome_desktop->WaitForPageToLoad(
          extension_bg_pages[i], capabilities.extension_load_timeout, &web_view,
          w3c_compliant);
      if (status.IsError()) {
        return Status(kUnknownError,
                      "failed to wait for extension background page to load: " +
                          extension_bg_pages[i],
                      status);
      }
    }
  }
  *chrome = std::move(chrome_desktop);
  return Status(kOk);
}

Status LaunchAndroidChrome(URLRequestContextGetter* context_getter,
                           const SyncWebSocketFactory& socket_factory,
                           const Capabilities& capabilities,
                           std::vector<std::unique_ptr<DevToolsEventListener>>
                               devtools_event_listeners,
                           DeviceManager* device_manager,
                           std::unique_ptr<Chrome>* chrome) {
  Status status(kOk);
  std::unique_ptr<Device> device;
  int devtools_port;
  if (capabilities.android_device_serial.empty()) {
    status = device_manager->AcquireDevice(&device);
  } else {
    status = device_manager->AcquireSpecificDevice(
        capabilities.android_device_serial, &device);
  }
  if (status.IsError())
    return status;

  Switches switches(capabilities.switches);
  for (auto* common_switch : kCommonSwitches)
    switches.SetUnparsedSwitch(common_switch);
  for (auto* android_switch : kAndroidSwitches)
    switches.SetUnparsedSwitch(android_switch);
  for (auto excluded_switch : capabilities.exclude_switches)
    switches.RemoveSwitch(excluded_switch);
  status = device->SetUp(
      capabilities.android_package, capabilities.android_activity,
      capabilities.android_process, capabilities.android_device_socket,
      capabilities.android_exec_name, switches.ToString(),
      capabilities.android_use_running_app, &devtools_port);
  if (status.IsError()) {
    device->TearDown();
    return status;
  }

  std::unique_ptr<DevToolsHttpClient> devtools_http_client;
  status = WaitForDevToolsAndCheckVersion(NetAddress(devtools_port),
                                          context_getter, socket_factory,
                                          &capabilities, &devtools_http_client);
  if (status.IsError()) {
    device->TearDown();
    return status;
  }

  std::unique_ptr<DevToolsClient> devtools_websocket_client;
  status = CreateBrowserwideDevToolsClientAndConnect(
      NetAddress(devtools_port), capabilities.perf_logging_prefs,
      socket_factory, devtools_event_listeners,
      devtools_http_client->browser_info()->web_socket_url,
      &devtools_websocket_client);
  if (status.IsError()) {
    LOG(WARNING) << "Browser-wide DevTools client failed to connect: "
                 << status.message();
  }

  chrome->reset(new ChromeAndroidImpl(
      std::move(devtools_http_client), std::move(devtools_websocket_client),
      std::move(devtools_event_listeners), capabilities.page_load_strategy,
      std::move(device)));
  return Status(kOk);
}

}  // namespace

Status LaunchChrome(URLRequestContextGetter* context_getter,
                    const SyncWebSocketFactory& socket_factory,
                    DeviceManager* device_manager,
                    const Capabilities& capabilities,
                    std::vector<std::unique_ptr<DevToolsEventListener>>
                        devtools_event_listeners,
                    std::unique_ptr<Chrome>* chrome,
                    bool w3c_compliant) {
  if (capabilities.IsRemoteBrowser()) {
    return LaunchRemoteChromeSession(
        context_getter, socket_factory, capabilities,
        std::move(devtools_event_listeners), chrome);
  }
  if (capabilities.IsAndroid()) {
    return LaunchAndroidChrome(context_getter, socket_factory, capabilities,
                               std::move(devtools_event_listeners),
                               device_manager, chrome);
  } else {
    return LaunchDesktopChrome(context_getter, socket_factory, capabilities,
                               std::move(devtools_event_listeners), chrome,
                               w3c_compliant);
  }
}

namespace internal {

void ConvertHexadecimalToIDAlphabet(std::string* id) {
  for (size_t i = 0; i < id->size(); ++i) {
    int val;
    if (base::HexStringToInt(base::StringPiece(id->begin() + i,
                                               id->begin() + i + 1),
                             &val)) {
      (*id)[i] = val + 'a';
    } else {
      (*id)[i] = 'a';
    }
  }
}

std::string GenerateExtensionId(const std::string& input) {
  uint8_t hash[16];
  crypto::SHA256HashString(input, hash, sizeof(hash));
  std::string output = base::ToLowerASCII(base::HexEncode(hash, sizeof(hash)));
  ConvertHexadecimalToIDAlphabet(&output);
  return output;
}

Status GetExtensionBackgroundPage(const base::DictionaryValue* manifest,
                                  const std::string& id,
                                  std::string* bg_page) {
  std::string bg_page_name;
  bool persistent = true;
  manifest->GetBoolean("background.persistent", &persistent);
  const base::Value* unused_value;
  if (manifest->Get("background.scripts", &unused_value))
    bg_page_name = "_generated_background_page.html";
  manifest->GetString("background.page", &bg_page_name);
  if (bg_page_name.empty() || !persistent)
    return Status(kOk);
  GURL baseUrl("chrome-extension://" + id + "/");
  *bg_page = baseUrl.Resolve(bg_page_name).spec();
  return Status(kOk);
}

Status ProcessExtension(const std::string& extension,
                        const base::FilePath& temp_dir,
                        base::FilePath* path,
                        std::string* bg_page) {
  // Decodes extension string.
  // Some WebDriver client base64 encoders follow RFC 1521, which require that
  // 'encoded lines be no more than 76 characters long'. Just remove any
  // newlines.
  std::string extension_base64;
  base::RemoveChars(extension, "\n", &extension_base64);
  std::string decoded_extension;
  if (!base::Base64Decode(extension_base64, &decoded_extension))
    return Status(kUnknownError, "cannot base64 decode");

  // If the file is a crx file, extract the extension's ID from its public key.
  // Otherwise generate a random public key and use its derived extension ID.
  std::string public_key;
  std::string magic_header = decoded_extension.substr(0, 4);
  if (magic_header.size() != 4)
    return Status(kUnknownError, "cannot extract magic number");

  const bool is_crx_file = magic_header == "Cr24";

  if (is_crx_file) {
    // Assume a CRX v2 file - see https://developer.chrome.com/extensions/crx.
    std::string key_len_str = decoded_extension.substr(8, 4);
    if (key_len_str.size() != 4)
      return Status(kUnknownError, "cannot extract public key length");
    uint32_t key_len = *reinterpret_cast<const uint32_t*>(key_len_str.c_str());
    public_key = decoded_extension.substr(16, key_len);
    if (key_len != public_key.size())
      return Status(kUnknownError, "invalid public key length");
  } else {
    // Not a CRX file. Generate RSA keypair to get a valid extension id.
    std::unique_ptr<crypto::RSAPrivateKey> key_pair(
        crypto::RSAPrivateKey::Create(2048));
    if (!key_pair)
      return Status(kUnknownError, "cannot generate RSA key pair");
    std::vector<uint8_t> public_key_vector;
    if (!key_pair->ExportPublicKey(&public_key_vector))
      return Status(kUnknownError, "cannot extract public key");
    public_key =
        std::string(reinterpret_cast<char*>(&public_key_vector.front()),
                    public_key_vector.size());
  }
  std::string public_key_base64;
  base::Base64Encode(public_key, &public_key_base64);
  std::string id = GenerateExtensionId(public_key);

  // Unzip the crx file.
  base::ScopedTempDir temp_crx_dir;
  if (!temp_crx_dir.CreateUniqueTempDir())
    return Status(kUnknownError, "cannot create temp dir");
  base::FilePath extension_crx = temp_crx_dir.GetPath().AppendASCII("temp.crx");
  int size = static_cast<int>(decoded_extension.length());
  if (base::WriteFile(extension_crx, decoded_extension.c_str(), size) !=
      size) {
    return Status(kUnknownError, "cannot write file");
  }
  base::FilePath extension_dir = temp_dir.AppendASCII("extension_" + id);
  if (!zip::Unzip(extension_crx, extension_dir))
    return Status(kUnknownError, "cannot unzip");

  // Parse the manifest and set the 'key' if not already present.
  base::FilePath manifest_path(extension_dir.AppendASCII("manifest.json"));
  std::string manifest_data;
  if (!base::ReadFileToString(manifest_path, &manifest_data))
    return Status(kUnknownError, "cannot read manifest");
  std::unique_ptr<base::Value> manifest_value =
      base::JSONReader::Read(manifest_data);
  base::DictionaryValue* manifest;
  if (!manifest_value || !manifest_value->GetAsDictionary(&manifest))
    return Status(kUnknownError, "invalid manifest");

  std::string manifest_key_base64;
  if (manifest->GetString("key", &manifest_key_base64)) {
    // If there is a key in both the header and the manifest, use the key in the
    // manifest. This allows chromedriver users users who generate dummy crxs
    // to set the manifest key and have a consistent ID.
    std::string manifest_key;
    if (!base::Base64Decode(manifest_key_base64, &manifest_key))
      return Status(kUnknownError, "'key' in manifest is not base64 encoded");
    std::string manifest_id = GenerateExtensionId(manifest_key);
    if (id != manifest_id) {
      if (is_crx_file) {
        LOG(WARNING)
            << "Public key in crx header is different from key in manifest"
            << std::endl << "key from header:   " << public_key_base64
            << std::endl << "key from manifest: " << manifest_key_base64
            << std::endl << "generated extension id from header key:   " << id
            << std::endl << "generated extension id from manifest key: "
            << manifest_id;
      }
      id = manifest_id;
    }
  } else {
    manifest->SetString("key", public_key_base64);
    base::JSONWriter::Write(*manifest, &manifest_data);
    if (base::WriteFile(
            manifest_path, manifest_data.c_str(), manifest_data.size()) !=
        static_cast<int>(manifest_data.size())) {
      return Status(kUnknownError, "cannot add 'key' to manifest");
    }
  }

  // Get extension's background page URL, if there is one.
  std::string bg_page_tmp;
  Status status = GetExtensionBackgroundPage(manifest, id, &bg_page_tmp);
  if (status.IsError())
    return status;

  *path = extension_dir;
  if (bg_page_tmp.size())
    *bg_page = bg_page_tmp;
  return Status(kOk);
}

void UpdateExtensionSwitch(Switches* switches,
                           const char name[],
                           const base::FilePath::StringType& extension) {
  base::FilePath::StringType value = switches->GetSwitchValueNative(name);
  if (value.length())
    value += FILE_PATH_LITERAL(",");
  value += extension;
  switches->SetSwitch(name, value);
}

Status ProcessExtensions(const std::vector<std::string>& extensions,
                         const base::FilePath& temp_dir,
                         bool include_automation_extension,
                         Switches* switches,
                         std::vector<std::string>* bg_pages) {
  std::vector<std::string> bg_pages_tmp;
  std::vector<base::FilePath::StringType> extension_paths;
  for (size_t i = 0; i < extensions.size(); ++i) {
    base::FilePath path;
    std::string bg_page;
    Status status = ProcessExtension(extensions[i], temp_dir, &path, &bg_page);
    if (status.IsError()) {
      return Status(
          kUnknownError,
          base::StringPrintf("cannot process extension #%" PRIuS, i + 1),
          status);
    }
    extension_paths.push_back(path.value());
    if (bg_page.length())
      bg_pages_tmp.push_back(bg_page);
  }

  if (include_automation_extension) {
    base::FilePath automation_extension;
    Status status = UnpackAutomationExtension(temp_dir, &automation_extension);
    if (status.IsError())
      return status;
    if (switches->HasSwitch("disable-extensions")) {
      UpdateExtensionSwitch(switches, "disable-extensions-except",
                            automation_extension.value());
    } else {
      extension_paths.push_back(automation_extension.value());
    }
  }

  if (extension_paths.size()) {
    base::FilePath::StringType extension_paths_value = base::JoinString(
        extension_paths, base::FilePath::StringType(1, ','));
    UpdateExtensionSwitch(switches, "load-extension", extension_paths_value);
  }
  bg_pages->swap(bg_pages_tmp);
  return Status(kOk);
}

Status WritePrefsFile(
    const std::string& template_string,
    const base::DictionaryValue* custom_prefs,
    const base::FilePath& path) {
  int code;
  std::string error_msg;
  std::unique_ptr<base::Value> template_value =
      base::JSONReader::ReadAndReturnError(template_string, 0, &code,
                                           &error_msg);
  base::DictionaryValue* prefs;
  if (!template_value || !template_value->GetAsDictionary(&prefs)) {
    return Status(kUnknownError,
                  "cannot parse internal JSON template: " + error_msg);
  }

  if (custom_prefs) {
    for (base::DictionaryValue::Iterator it(*custom_prefs); !it.IsAtEnd();
         it.Advance()) {
      prefs->Set(it.key(), std::make_unique<base::Value>(it.value().Clone()));
    }
  }

  std::string prefs_str;
  base::JSONWriter::Write(*prefs, &prefs_str);
  VLOG(0) << "Populating " << path.BaseName().value()
          << " file: " << PrettyPrintValue(*prefs);
  if (static_cast<int>(prefs_str.length()) != base::WriteFile(
          path, prefs_str.c_str(), prefs_str.length())) {
    return Status(kUnknownError, "failed to write prefs file");
  }
  return Status(kOk);
}

Status PrepareUserDataDir(
    const base::FilePath& user_data_dir,
    const base::DictionaryValue* custom_prefs,
    const base::DictionaryValue* custom_local_state) {
  base::FilePath default_dir =
      user_data_dir.AppendASCII(chrome::kInitialProfile);
  if (!base::CreateDirectory(default_dir))
    return Status(kUnknownError, "cannot create default profile directory");

  std::string preferences;
  base::FilePath preferences_path =
      default_dir.Append(chrome::kPreferencesFilename);

  if (base::PathExists(preferences_path))
    base::ReadFileToString(preferences_path, &preferences);
  else
    preferences = kPreferences;

  Status status =
      WritePrefsFile(preferences,
                     custom_prefs,
                     default_dir.Append(chrome::kPreferencesFilename));
  if (status.IsError())
    return status;

  std::string local_state;
  base::FilePath local_state_path =
      user_data_dir.Append(chrome::kLocalStateFilename);

  if (base::PathExists(local_state_path))
    base::ReadFileToString(local_state_path, &local_state);
  else
    local_state = kLocalState;

  status = WritePrefsFile(local_state,
                          custom_local_state,
                          user_data_dir.Append(chrome::kLocalStateFilename));
  if (status.IsError())
    return status;

  // Write empty "First Run" file, otherwise Chrome will wipe the default
  // profile that was written.
  if (base::WriteFile(
          user_data_dir.Append(chrome::kFirstRunSentinel), "", 0) != 0) {
    return Status(kUnknownError, "failed to write first run file");
  }
  return Status(kOk);
}

Status ReadInPort(const base::FilePath& port_filepath, int* port) {
  if (!base::PathExists(port_filepath)) {
    return Status(kUnknownError, "DevToolsActivePort file doesn't exist");
  }
  std::string buffer;
  bool result = base::ReadFileToString(port_filepath, &buffer);
  if (!result) {
    return Status(kUnknownError, "Could not read in devtools port number");
  }
  std::vector<std::string> split_port_strings = base::SplitString(
      buffer, "\n", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
  if (split_port_strings.size() < 2) {
    return Status(kUnknownError,
                  std::string("Devtools port number file contents <") + buffer +
                      std::string("> were in an unexpected format"));
  }
  if (!base::StringToInt(split_port_strings.front(), port)) {
    return Status(kUnknownError,
                  "Could not convert devtools port number to int");
  }
  return Status(kOk);
}

Status ParseDevToolsActivePortFile(const base::FilePath& user_data_dir,
                                   int* port) {
  base::FilePath port_filepath = user_data_dir.Append(kDevToolsActivePort);
  base::TimeTicks deadline =
      base::TimeTicks::Now() + base::TimeDelta::FromSeconds(60);
  Status result =
      Status(kUnknownError, "This should not happen: increase the deadline");
  while (base::TimeTicks::Now() < deadline) {
    result = ReadInPort(port_filepath, port);
    if (result.IsOk()) {
      return result;
    }
    base::PlatformThread::Sleep(base::TimeDelta::FromMilliseconds(50));
  }
  return result;
}

Status RemoveOldDevToolsActivePortFile(const base::FilePath& user_data_dir) {
  base::FilePath port_filepath = user_data_dir.Append(kDevToolsActivePort);
  // Note that calling DeleteFile on a path that doesn't exist returns True.
  if (base::DeleteFile(port_filepath, false)) {
    return Status(kOk);
  }
  return Status(
      kUnknownError,
      std::string("Could not remove old devtools port file. Perhaps "
                  "the given user-data-dir at ") +
          user_data_dir.AsUTF8Unsafe() +
          std::string(" is still attached to a running Chrome or Chromium "
                      "process."));
}

}  // namespace internal
