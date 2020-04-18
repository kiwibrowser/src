// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/shell_integration_linux.h"

#include <fcntl.h>
#include <stddef.h>

#include <memory>

#if defined(USE_GLIB)
#include <glib.h>
#endif

#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <string>
#include <vector>

#include "base/base_paths.h"
#include "base/command_line.h"
#include "base/environment.h"
#include "base/files/file_enumerator.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/i18n/file_util_icu.h"
#include "base/memory/ref_counted_memory.h"
#include "base/nix/xdg_util.h"
#include "base/path_service.h"
#include "base/posix/eintr_wrapper.h"
#include "base/process/kill.h"
#include "base/process/launch.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_tokenizer.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread.h"
#include "base/threading/thread_restrictions.h"
#include "build/build_config.h"
#include "chrome/browser/shell_integration.h"
#include "chrome/common/buildflags.h"
#include "chrome/common/channel_info.h"
#include "chrome/common/chrome_constants.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/grit/chrome_unscaled_resources.h"
#include "components/version_info/version_info.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/image/image_family.h"
#include "url/gurl.h"

namespace shell_integration {

// Allows LaunchXdgUtility to join a process.
class LaunchXdgUtilityScopedAllowBaseSyncPrimitives
    : public base::ScopedAllowBaseSyncPrimitives {};

namespace {

// Helper to launch xdg scripts. We don't want them to ask any questions on the
// terminal etc. The function returns true if the utility launches and exits
// cleanly, in which case |exit_code| returns the utility's exit code.
bool LaunchXdgUtility(const std::vector<std::string>& argv, int* exit_code) {
  // xdg-settings internally runs xdg-mime, which uses mv to move newly-created
  // files on top of originals after making changes to them. In the event that
  // the original files are owned by another user (e.g. root, which can happen
  // if they are updated within sudo), mv will prompt the user to confirm if
  // standard input is a terminal (otherwise it just does it). So make sure it's
  // not, to avoid locking everything up waiting for mv.
  *exit_code = EXIT_FAILURE;
  int devnull = open("/dev/null", O_RDONLY);
  if (devnull < 0)
    return false;

  base::LaunchOptions options;
  options.fds_to_remap.push_back(std::make_pair(devnull, STDIN_FILENO));
  base::Process process = base::LaunchProcess(argv, options);
  close(devnull);
  if (!process.IsValid())
    return false;
  LaunchXdgUtilityScopedAllowBaseSyncPrimitives allow_base_sync_primitives;
  return process.WaitForExit(exit_code);
}

const char kXdgSettings[] = "xdg-settings";
const char kXdgSettingsDefaultBrowser[] = "default-web-browser";
const char kXdgSettingsDefaultSchemeHandler[] = "default-url-scheme-handler";

// Utility function to get the path to the version of a script shipped with
// Chrome. |script| gives the name of the script. |chrome_version| returns the
// path to the Chrome version of the script, and the return value of the
// function is true if the function is successful and the Chrome version is
// not the script found on the PATH.
bool GetChromeVersionOfScript(const std::string& script,
                              std::string* chrome_version) {
  // Get the path to the Chrome version.
  base::FilePath chrome_dir;
  if (!base::PathService::Get(base::DIR_EXE, &chrome_dir))
    return false;

  base::FilePath chrome_version_path = chrome_dir.Append(script);
  *chrome_version = chrome_version_path.value();

  // Check if this is different to the one on path.
  std::vector<std::string> argv;
  argv.push_back("which");
  argv.push_back(script);
  std::string path_version;
  if (base::GetAppOutput(base::CommandLine(argv), &path_version)) {
    // Remove trailing newline
    path_version.erase(path_version.length() - 1, 1);
    base::FilePath path_version_path(path_version);
    return (chrome_version_path != path_version_path);
  }
  return false;
}

// Value returned by xdg-settings if it can't understand our request.
const int EXIT_XDG_SETTINGS_SYNTAX_ERROR = 1;

// We delegate the difficulty of setting the default browser and default url
// scheme handler in Linux desktop environments to an xdg utility, xdg-settings.

// When calling this script we first try to use the script on PATH. If that
// fails we then try to use the script that we have included. This gives
// scripts on the system priority over ours, as distribution vendors may have
// tweaked the script, but still allows our copy to be used if the script on the
// system fails, as the system copy may be missing capabilities of the Chrome
// copy.

// If |protocol| is empty this function sets Chrome as the default browser,
// otherwise it sets Chrome as the default handler application for |protocol|.
bool SetDefaultWebClient(const std::string& protocol) {
#if defined(OS_CHROMEOS)
  return true;
#else
  std::unique_ptr<base::Environment> env(base::Environment::Create());

  std::vector<std::string> argv;
  argv.push_back(kXdgSettings);
  argv.push_back("set");
  if (protocol.empty()) {
    argv.push_back(kXdgSettingsDefaultBrowser);
  } else {
    argv.push_back(kXdgSettingsDefaultSchemeHandler);
    argv.push_back(protocol);
  }
  argv.push_back(shell_integration_linux::GetDesktopName(env.get()));

  int exit_code;
  bool ran_ok = LaunchXdgUtility(argv, &exit_code);
  if (ran_ok && exit_code == EXIT_XDG_SETTINGS_SYNTAX_ERROR) {
    if (GetChromeVersionOfScript(kXdgSettings, &argv[0])) {
      ran_ok = LaunchXdgUtility(argv, &exit_code);
    }
  }

  return ran_ok && exit_code == EXIT_SUCCESS;
#endif
}

// If |protocol| is empty this function checks if Chrome is the default browser,
// otherwise it checks if Chrome is the default handler application for
// |protocol|.
DefaultWebClientState GetIsDefaultWebClient(const std::string& protocol) {
#if defined(OS_CHROMEOS)
  return UNKNOWN_DEFAULT;
#else
  base::AssertBlockingAllowed();

  std::unique_ptr<base::Environment> env(base::Environment::Create());

  std::vector<std::string> argv;
  argv.push_back(kXdgSettings);
  argv.push_back("check");
  if (protocol.empty()) {
    argv.push_back(kXdgSettingsDefaultBrowser);
  } else {
    argv.push_back(kXdgSettingsDefaultSchemeHandler);
    argv.push_back(protocol);
  }
  argv.push_back(shell_integration_linux::GetDesktopName(env.get()));

  std::string reply;
  int success_code;
  bool ran_ok = base::GetAppOutputWithExitCode(base::CommandLine(argv), &reply,
                                               &success_code);
  if (ran_ok && success_code == EXIT_XDG_SETTINGS_SYNTAX_ERROR) {
    if (GetChromeVersionOfScript(kXdgSettings, &argv[0])) {
      ran_ok = base::GetAppOutputWithExitCode(base::CommandLine(argv), &reply,
                                              &success_code);
    }
  }

  if (!ran_ok || success_code != EXIT_SUCCESS) {
    // xdg-settings failed: we can't determine or set the default browser.
    return UNKNOWN_DEFAULT;
  }

  // Allow any reply that starts with "yes".
  return base::StartsWith(reply, "yes", base::CompareCase::SENSITIVE)
             ? IS_DEFAULT
             : NOT_DEFAULT;
#endif
}

// https://wiki.gnome.org/Projects/GnomeShell/ApplicationBased
// The WM_CLASS property should be set to the same as the *.desktop file without
// the .desktop extension.  We cannot simply use argv[0] in this case, because
// on the stable channel, the executable name is google-chrome-stable, but the
// desktop file is google-chrome.desktop.
std::string GetDesktopBaseName(const std::string& desktop_file_name) {
  static const char kDesktopExtension[] = ".desktop";
  if (base::EndsWith(desktop_file_name, kDesktopExtension,
                     base::CompareCase::SENSITIVE)) {
    return desktop_file_name.substr(
        0, desktop_file_name.length() - strlen(kDesktopExtension));
  }
  return desktop_file_name;
}

}  // namespace

bool SetAsDefaultBrowser() {
  return SetDefaultWebClient(std::string());
}

bool SetAsDefaultProtocolClient(const std::string& protocol) {
  return SetDefaultWebClient(protocol);
}

DefaultWebClientSetPermission GetDefaultWebClientSetPermission() {
  return SET_DEFAULT_UNATTENDED;
}

base::string16 GetApplicationNameForProtocol(const GURL& url) {
  return base::ASCIIToUTF16("xdg-open");
}

DefaultWebClientState GetDefaultBrowser() {
  return GetIsDefaultWebClient(std::string());
}

bool IsFirefoxDefaultBrowser() {
  std::vector<std::string> argv;
  argv.push_back(kXdgSettings);
  argv.push_back("get");
  argv.push_back(kXdgSettingsDefaultBrowser);

  std::string browser;
  // We don't care about the return value here.
  base::GetAppOutput(base::CommandLine(argv), &browser);
  return browser.find("irefox") != std::string::npos;
}

DefaultWebClientState IsDefaultProtocolClient(const std::string& protocol) {
  return GetIsDefaultWebClient(protocol);
}

}  // namespace shell_integration

namespace shell_integration_linux {

namespace {

#if BUILDFLAG(ENABLE_APP_LIST)
// The Categories for the App Launcher desktop shortcut. Should be the same as
// the Chrome desktop shortcut, so they are in the same sub-menu.
const char kAppListCategories[] = "Network;WebBrowser;";
#endif

std::string CreateShortcutIcon(const gfx::ImageFamily& icon_images,
                               const base::FilePath& shortcut_filename) {
  if (icon_images.empty())
    return std::string();

  // TODO(phajdan.jr): Report errors from this function, possibly as infobars.
  base::ScopedTempDir temp_dir;
  if (!temp_dir.CreateUniqueTempDir())
    return std::string();

  base::FilePath temp_file_path =
      temp_dir.GetPath().Append(shortcut_filename.ReplaceExtension("png"));
  std::string icon_name = temp_file_path.BaseName().RemoveExtension().value();

  for (gfx::ImageFamily::const_iterator it = icon_images.begin();
       it != icon_images.end(); ++it) {
    int width = it->Width();
    scoped_refptr<base::RefCountedMemory> png_data = it->As1xPNGBytes();
    if (png_data->size() == 0) {
      // If the bitmap could not be encoded to PNG format, skip it.
      LOG(WARNING) << "Could not encode icon " << icon_name << ".png at size "
                   << width << ".";
      continue;
    }
    int bytes_written = base::WriteFile(temp_file_path,
                                        png_data->front_as<char>(),
                                        png_data->size());

    if (bytes_written != static_cast<int>(png_data->size()))
      return std::string();

    std::vector<std::string> argv;
    argv.push_back("xdg-icon-resource");
    argv.push_back("install");

    // Always install in user mode, even if someone runs the browser as root
    // (people do that).
    argv.push_back("--mode");
    argv.push_back("user");

    argv.push_back("--size");
    argv.push_back(base::IntToString(width));

    argv.push_back(temp_file_path.value());
    argv.push_back(icon_name);
    int exit_code;
    if (!shell_integration::LaunchXdgUtility(argv, &exit_code) || exit_code) {
      LOG(WARNING) << "Could not install icon " << icon_name << ".png at size "
                   << width << ".";
    }
  }
  return icon_name;
}

bool CreateShortcutOnDesktop(const base::FilePath& shortcut_filename,
                             const std::string& contents) {
  // Make sure that we will later call openat in a secure way.
  DCHECK_EQ(shortcut_filename.BaseName().value(), shortcut_filename.value());

  base::FilePath desktop_path;
  if (!base::PathService::Get(base::DIR_USER_DESKTOP, &desktop_path))
    return false;

  int desktop_fd = open(desktop_path.value().c_str(), O_RDONLY | O_DIRECTORY);
  if (desktop_fd < 0)
    return false;

  int fd = openat(desktop_fd, shortcut_filename.value().c_str(),
                  O_CREAT | O_EXCL | O_WRONLY,
                  S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
  if (fd < 0) {
    if (IGNORE_EINTR(close(desktop_fd)) < 0)
      PLOG(ERROR) << "close";
    return false;
  }

  if (!base::WriteFileDescriptor(fd, contents.c_str(), contents.size())) {
    // Delete the file. No shortuct is better than corrupted one. Use unlinkat
    // to make sure we're deleting the file in the directory we think we are.
    // Even if an attacker manager to put something other at
    // |shortcut_filename| we'll just undo their action.
    unlinkat(desktop_fd, shortcut_filename.value().c_str(), 0);
  }

  if (IGNORE_EINTR(close(fd)) < 0)
    PLOG(ERROR) << "close";

  if (IGNORE_EINTR(close(desktop_fd)) < 0)
    PLOG(ERROR) << "close";

  return true;
}

void DeleteShortcutOnDesktop(const base::FilePath& shortcut_filename) {
  base::FilePath desktop_path;
  if (base::PathService::Get(base::DIR_USER_DESKTOP, &desktop_path))
    base::DeleteFile(desktop_path.Append(shortcut_filename), false);
}

// Creates a shortcut with |shortcut_filename| and |contents| in the system
// applications menu. If |directory_filename| is non-empty, creates a sub-menu
// with |directory_filename| and |directory_contents|, and stores the shortcut
// under the sub-menu.
bool CreateShortcutInApplicationsMenu(const base::FilePath& shortcut_filename,
                                      const std::string& contents,
                                      const base::FilePath& directory_filename,
                                      const std::string& directory_contents) {
  base::ScopedTempDir temp_dir;
  if (!temp_dir.CreateUniqueTempDir())
    return false;

  base::FilePath temp_directory_path;
  if (!directory_filename.empty()) {
    temp_directory_path = temp_dir.GetPath().Append(directory_filename);

    int bytes_written = base::WriteFile(temp_directory_path,
                                        directory_contents.data(),
                                        directory_contents.length());

    if (bytes_written != static_cast<int>(directory_contents.length()))
      return false;
  }

  base::FilePath temp_file_path = temp_dir.GetPath().Append(shortcut_filename);

  int bytes_written = base::WriteFile(temp_file_path, contents.data(),
                                      contents.length());

  if (bytes_written != static_cast<int>(contents.length()))
    return false;

  std::vector<std::string> argv;
  argv.push_back("xdg-desktop-menu");
  argv.push_back("install");

  // Always install in user mode, even if someone runs the browser as root
  // (people do that).
  argv.push_back("--mode");
  argv.push_back("user");

  // If provided, install the shortcut file inside the given directory.
  if (!directory_filename.empty())
    argv.push_back(temp_directory_path.value());
  argv.push_back(temp_file_path.value());
  int exit_code;
  shell_integration::LaunchXdgUtility(argv, &exit_code);
  return exit_code == 0;
}

void DeleteShortcutInApplicationsMenu(
    const base::FilePath& shortcut_filename,
    const base::FilePath& directory_filename) {
  std::vector<std::string> argv;
  argv.push_back("xdg-desktop-menu");
  argv.push_back("uninstall");

  // Uninstall in user mode, to match the install.
  argv.push_back("--mode");
  argv.push_back("user");

  // The file does not need to exist anywhere - xdg-desktop-menu will uninstall
  // items from the menu with a matching name.
  // If |directory_filename| is supplied, this will also remove the item from
  // the directory, and remove the directory if it is empty.
  if (!directory_filename.empty())
    argv.push_back(directory_filename.value());
  argv.push_back(shortcut_filename.value());
  int exit_code;
  shell_integration::LaunchXdgUtility(argv, &exit_code);
}

#if defined(USE_GLIB)
// Quote a string such that it appears as one verbatim argument for the Exec
// key in a desktop file.
std::string QuoteArgForDesktopFileExec(const std::string& arg) {
  // http://standards.freedesktop.org/desktop-entry-spec/latest/ar01s06.html

  // Quoting is only necessary if the argument has a reserved character.
  if (arg.find_first_of(" \t\n\"'\\><~|&;$*?#()`") == std::string::npos)
    return arg;  // No quoting necessary.

  std::string quoted = "\"";
  for (size_t i = 0; i < arg.size(); ++i) {
    // Note that the set of backslashed characters is smaller than the
    // set of reserved characters.
    switch (arg[i]) {
      case '"':
      case '`':
      case '$':
      case '\\':
        quoted += '\\';
        break;
    }
    quoted += arg[i];
  }
  quoted += '"';

  return quoted;
}

// Quote a command line so it is suitable for use as the Exec key in a desktop
// file. Note: This should be used instead of GetCommandLineString, which does
// not properly quote the string; this function is designed for the Exec key.
std::string QuoteCommandLineForDesktopFileExec(
    const base::CommandLine& command_line) {
  // http://standards.freedesktop.org/desktop-entry-spec/latest/ar01s06.html

  std::string quoted_path = "";
  const base::CommandLine::StringVector& argv = command_line.argv();
  for (base::CommandLine::StringVector::const_iterator i = argv.begin();
       i != argv.end(); ++i) {
    if (i != argv.begin())
      quoted_path += " ";
    quoted_path += QuoteArgForDesktopFileExec(*i);
  }

  return quoted_path;
}

const char kDesktopEntry[] = "Desktop Entry";

const char kXdgOpenShebang[] = "#!/usr/bin/env xdg-open";
#endif

const char kDirectoryFilename[] = "chrome-apps.directory";

#if BUILDFLAG(ENABLE_APP_LIST)
#if defined(GOOGLE_CHROME_BUILD)
const char kAppListDesktopName[] = "chrome-app-list";
#else  // CHROMIUM_BUILD
const char kAppListDesktopName[] = "chromium-app-list";
#endif
#endif

// Get the value of NoDisplay from the [Desktop Entry] section of a .desktop
// file, given in |shortcut_contents|. If the key is not found, returns false.
bool GetNoDisplayFromDesktopFile(const std::string& shortcut_contents) {
#if defined(USE_GLIB)
  // An empty file causes a crash with glib <= 2.32, so special case here.
  if (shortcut_contents.empty())
    return false;

  GKeyFile* key_file = g_key_file_new();
  GError* err = NULL;
  if (!g_key_file_load_from_data(key_file, shortcut_contents.c_str(),
                                 shortcut_contents.size(), G_KEY_FILE_NONE,
                                 &err)) {
    LOG(WARNING) << "Unable to read desktop file template: " << err->message;
    g_error_free(err);
    g_key_file_free(key_file);
    return false;
  }

  bool nodisplay = false;
  char* nodisplay_c_string = g_key_file_get_string(key_file, kDesktopEntry,
                                                   "NoDisplay", &err);
  if (nodisplay_c_string) {
    if (!g_strcmp0(nodisplay_c_string, "true"))
      nodisplay = true;
    g_free(nodisplay_c_string);
  } else {
    g_error_free(err);
  }

  g_key_file_free(key_file);
  return nodisplay;
#else
  NOTIMPLEMENTED();
  return false;
#endif
}

// Gets the path to the Chrome executable or wrapper script.
// Returns an empty path if the executable path could not be found, which should
// never happen.
base::FilePath GetChromeExePath() {
  // Try to get the name of the wrapper script that launched Chrome.
  std::unique_ptr<base::Environment> environment(base::Environment::Create());
  std::string wrapper_script;
  if (environment->GetVar("CHROME_WRAPPER", &wrapper_script))
    return base::FilePath(wrapper_script);

  // Just return the name of the executable path for Chrome.
  base::FilePath chrome_exe_path;
  base::PathService::Get(base::FILE_EXE, &chrome_exe_path);
  return chrome_exe_path;
}

}  // namespace

base::FilePath GetDataWriteLocation(base::Environment* env) {
  return base::nix::GetXDGDirectory(env, "XDG_DATA_HOME", ".local/share");
}

std::vector<base::FilePath> GetDataSearchLocations(base::Environment* env) {
  base::AssertBlockingAllowed();

  std::vector<base::FilePath> search_paths;
  base::FilePath write_location = GetDataWriteLocation(env);
  search_paths.push_back(write_location);

  std::string xdg_data_dirs;
  if (env->GetVar("XDG_DATA_DIRS", &xdg_data_dirs) && !xdg_data_dirs.empty()) {
    base::StringTokenizer tokenizer(xdg_data_dirs, ":");
    while (tokenizer.GetNext()) {
      base::FilePath data_dir(tokenizer.token());
      search_paths.push_back(data_dir);
    }
  } else {
    search_paths.push_back(base::FilePath("/usr/local/share"));
    search_paths.push_back(base::FilePath("/usr/share"));
  }

  return search_paths;
}

namespace internal {

std::string GetProgramClassName(const base::CommandLine& command_line,
                                const std::string& desktop_file_name) {
  std::string class_name =
      shell_integration::GetDesktopBaseName(desktop_file_name);
  std::string user_data_dir =
      command_line.GetSwitchValueNative(switches::kUserDataDir);
  // If the user launches with e.g. --user-data-dir=/tmp/my-user-data, set the
  // class name to "Chrome (/tmp/my-user-data)".  The class name will show up in
  // the alt-tab list in gnome-shell if you're running a binary that doesn't
  // have a matching .desktop file.
  return user_data_dir.empty()
             ? class_name
             : class_name + " (" + user_data_dir + ")";
}

std::string GetProgramClassClass(const base::CommandLine& command_line,
                                 const std::string& desktop_file_name) {
  if (command_line.HasSwitch(switches::kWmClass))
    return command_line.GetSwitchValueASCII(switches::kWmClass);
  std::string class_class =
      shell_integration::GetDesktopBaseName(desktop_file_name);
  if (!class_class.empty()) {
    // Capitalize the first character like gtk does.
    class_class[0] = base::ToUpperASCII(class_class[0]);
  }
  return class_class;
}

}  // namespace internal

std::string GetProgramClassName() {
  std::unique_ptr<base::Environment> env(base::Environment::Create());
  return internal::GetProgramClassName(*base::CommandLine::ForCurrentProcess(),
                                       GetDesktopName(env.get()));
}

std::string GetProgramClassClass() {
  std::unique_ptr<base::Environment> env(base::Environment::Create());
  return internal::GetProgramClassClass(*base::CommandLine::ForCurrentProcess(),
                                        GetDesktopName(env.get()));
}

std::string GetDesktopName(base::Environment* env) {
#if defined(GOOGLE_CHROME_BUILD)
  version_info::Channel product_channel(chrome::GetChannel());
  switch (product_channel) {
    case version_info::Channel::DEV:
      return "google-chrome-unstable.desktop";
    case version_info::Channel::BETA:
      return "google-chrome-beta.desktop";
    default:
      return "google-chrome.desktop";
  }
#else  // CHROMIUM_BUILD
  // Allow $CHROME_DESKTOP to override the built-in value, so that development
  // versions can set themselves as the default without interfering with
  // non-official, packaged versions using the built-in value.
  std::string name;
  if (env->GetVar("CHROME_DESKTOP", &name) && !name.empty())
    return name;
  return "chromium-browser.desktop";
#endif
}

std::string GetIconName() {
#if defined(GOOGLE_CHROME_BUILD)
  return "google-chrome";
#else  // CHROMIUM_BUILD
  return "chromium-browser";
#endif
}

web_app::ShortcutLocations GetExistingShortcutLocations(
    base::Environment* env,
    const base::FilePath& profile_path,
    const std::string& extension_id) {
  base::FilePath desktop_path;
  // If Get returns false, just leave desktop_path empty.
  base::PathService::Get(base::DIR_USER_DESKTOP, &desktop_path);
  return GetExistingShortcutLocations(env, profile_path, extension_id,
                                      desktop_path);
}

web_app::ShortcutLocations GetExistingShortcutLocations(
    base::Environment* env,
    const base::FilePath& profile_path,
    const std::string& extension_id,
    const base::FilePath& desktop_path) {
  base::AssertBlockingAllowed();

  base::FilePath shortcut_filename = GetExtensionShortcutFilename(
      profile_path, extension_id);
  DCHECK(!shortcut_filename.empty());
  web_app::ShortcutLocations locations;

  // Determine whether there is a shortcut on desktop.
  if (!desktop_path.empty()) {
    locations.on_desktop =
        base::PathExists(desktop_path.Append(shortcut_filename));
  }

  // Determine whether there is a shortcut in the applications directory.
  std::string shortcut_contents;
  if (GetExistingShortcutContents(env, shortcut_filename, &shortcut_contents)) {
    // If the shortcut contents contain NoDisplay=true, it should be hidden.
    // Otherwise since these shortcuts are for apps, they are always in the
    // "Chrome Apps" directory.
    locations.applications_menu_location =
        GetNoDisplayFromDesktopFile(shortcut_contents)
            ? web_app::APP_MENU_LOCATION_HIDDEN
            : web_app::APP_MENU_LOCATION_SUBDIR_CHROMEAPPS;
  }

  return locations;
}

bool GetExistingShortcutContents(base::Environment* env,
                                 const base::FilePath& desktop_filename,
                                 std::string* output) {
  base::AssertBlockingAllowed();

  std::vector<base::FilePath> search_paths = GetDataSearchLocations(env);

  for (std::vector<base::FilePath>::const_iterator i = search_paths.begin();
       i != search_paths.end(); ++i) {
    base::FilePath path = i->Append("applications").Append(desktop_filename);
    VLOG(1) << "Looking for desktop file in " << path.value();
    if (base::PathExists(path)) {
      VLOG(1) << "Found desktop file at " << path.value();
      return base::ReadFileToString(path, output);
    }
  }

  return false;
}

base::FilePath GetWebShortcutFilename(const GURL& url) {
  // Use a prefix, because xdg-desktop-menu requires it.
  std::string filename =
      std::string(chrome::kBrowserProcessExecutableName) + "-" + url.spec();
  base::i18n::ReplaceIllegalCharactersInPath(&filename, '_');

  base::FilePath desktop_path;
  if (!base::PathService::Get(base::DIR_USER_DESKTOP, &desktop_path))
    return base::FilePath();

  base::FilePath filepath = desktop_path.Append(filename);
  base::FilePath alternative_filepath(filepath.value() + ".desktop");
  for (size_t i = 1; i < 100; ++i) {
    if (base::PathExists(base::FilePath(alternative_filepath))) {
      alternative_filepath = base::FilePath(
          filepath.value() + "_" + base::NumberToString(i) + ".desktop");
    } else {
      return base::FilePath(alternative_filepath).BaseName();
    }
  }

  return base::FilePath();
}

base::FilePath GetExtensionShortcutFilename(const base::FilePath& profile_path,
                                            const std::string& extension_id) {
  DCHECK(!extension_id.empty());

  // Use a prefix, because xdg-desktop-menu requires it.
  std::string filename(chrome::kBrowserProcessExecutableName);
  filename.append("-")
      .append(extension_id)
      .append("-")
      .append(profile_path.BaseName().value());
  base::i18n::ReplaceIllegalCharactersInPath(&filename, '_');
  // Spaces in filenames break xdg-desktop-menu
  // (see https://bugs.freedesktop.org/show_bug.cgi?id=66605).
  base::ReplaceChars(filename, " ", "_", &filename);
  return base::FilePath(filename.append(".desktop"));
}

std::vector<base::FilePath> GetExistingProfileShortcutFilenames(
    const base::FilePath& profile_path,
    const base::FilePath& directory) {
  base::AssertBlockingAllowed();

  // Use a prefix, because xdg-desktop-menu requires it.
  std::string prefix(chrome::kBrowserProcessExecutableName);
  prefix.append("-");
  std::string suffix("-");
  suffix.append(profile_path.BaseName().value());
  base::i18n::ReplaceIllegalCharactersInPath(&suffix, '_');
  // Spaces in filenames break xdg-desktop-menu
  // (see https://bugs.freedesktop.org/show_bug.cgi?id=66605).
  base::ReplaceChars(suffix, " ", "_", &suffix);
  std::string glob = prefix + "*" + suffix + ".desktop";

  base::FileEnumerator files(directory, false, base::FileEnumerator::FILES,
                             glob);
  base::FilePath shortcut_file = files.Next();
  std::vector<base::FilePath> shortcut_paths;
  while (!shortcut_file.empty()) {
    shortcut_paths.push_back(shortcut_file.BaseName());
    shortcut_file = files.Next();
  }
  return shortcut_paths;
}

std::string GetDesktopFileContents(
    const base::FilePath& chrome_exe_path,
    const std::string& app_name,
    const GURL& url,
    const std::string& extension_id,
    const base::string16& title,
    const std::string& icon_name,
    const base::FilePath& profile_path,
    const std::string& categories,
    bool no_display) {
  base::CommandLine cmd_line = shell_integration::CommandLineArgsForLauncher(
      url, extension_id, profile_path);
  cmd_line.SetProgram(chrome_exe_path);
  return GetDesktopFileContentsForCommand(cmd_line, app_name, url, title,
                                          icon_name, categories, no_display);
}

std::string GetDesktopFileContentsForCommand(
    const base::CommandLine& command_line,
    const std::string& app_name,
    const GURL& url,
    const base::string16& title,
    const std::string& icon_name,
    const std::string& categories,
    bool no_display) {
#if defined(USE_GLIB)
  // Although not required by the spec, Nautilus on Ubuntu Karmic creates its
  // launchers with an xdg-open shebang. Follow that convention.
  std::string output_buffer = std::string(kXdgOpenShebang) + "\n";

  // See http://standards.freedesktop.org/desktop-entry-spec/latest/
  GKeyFile* key_file = g_key_file_new();

  // Set keys with fixed values.
  g_key_file_set_string(key_file, kDesktopEntry, "Version", "1.0");
  g_key_file_set_string(key_file, kDesktopEntry, "Terminal", "false");
  g_key_file_set_string(key_file, kDesktopEntry, "Type", "Application");

  // Set the "Name" key.
  std::string final_title = base::UTF16ToUTF8(title);
  // Make sure no endline characters can slip in and possibly introduce
  // additional lines (like Exec, which makes it a security risk). Also
  // use the URL as a default when the title is empty.
  if (final_title.empty() ||
      final_title.find("\n") != std::string::npos ||
      final_title.find("\r") != std::string::npos) {
    final_title = url.spec();
  }
  g_key_file_set_string(key_file, kDesktopEntry, "Name", final_title.c_str());

  // Set the "Exec" key.
  std::string final_path = QuoteCommandLineForDesktopFileExec(command_line);
  g_key_file_set_string(key_file, kDesktopEntry, "Exec", final_path.c_str());

  // Set the "Icon" key.
  if (!icon_name.empty()) {
    g_key_file_set_string(key_file, kDesktopEntry, "Icon", icon_name.c_str());
  } else {
    g_key_file_set_string(key_file, kDesktopEntry, "Icon",
                          GetIconName().c_str());
  }

  // Set the "Categories" key.
  if (!categories.empty()) {
    g_key_file_set_string(
        key_file, kDesktopEntry, "Categories", categories.c_str());
  }

  // Set the "NoDisplay" key.
  if (no_display)
    g_key_file_set_string(key_file, kDesktopEntry, "NoDisplay", "true");

  std::string wmclass = web_app::GetWMClassFromAppName(app_name);
  g_key_file_set_string(key_file, kDesktopEntry, "StartupWMClass",
                        wmclass.c_str());

  gsize length = 0;
  gchar* data_dump = g_key_file_to_data(key_file, &length, NULL);
  if (data_dump) {
    // If strlen(data_dump[0]) == 0, this check will fail.
    if (data_dump[0] == '\n') {
      // Older versions of glib produce a leading newline. If this is the case,
      // remove it to avoid double-newline after the shebang.
      output_buffer += (data_dump + 1);
    } else {
      output_buffer += data_dump;
    }
    g_free(data_dump);
  }

  g_key_file_free(key_file);
  return output_buffer;
#else
  NOTIMPLEMENTED();
  return std::string();
#endif
}

std::string GetDirectoryFileContents(const base::string16& title,
                                     const std::string& icon_name) {
#if defined(USE_GLIB)
  // See http://standards.freedesktop.org/desktop-entry-spec/latest/
  GKeyFile* key_file = g_key_file_new();

  g_key_file_set_string(key_file, kDesktopEntry, "Version", "1.0");
  g_key_file_set_string(key_file, kDesktopEntry, "Type", "Directory");
  std::string final_title = base::UTF16ToUTF8(title);
  g_key_file_set_string(key_file, kDesktopEntry, "Name", final_title.c_str());
  if (!icon_name.empty()) {
    g_key_file_set_string(key_file, kDesktopEntry, "Icon", icon_name.c_str());
  } else {
    g_key_file_set_string(key_file, kDesktopEntry, "Icon",
                          GetIconName().c_str());
  }

  gsize length = 0;
  gchar* data_dump = g_key_file_to_data(key_file, &length, NULL);
  std::string output_buffer;
  if (data_dump) {
    // If strlen(data_dump[0]) == 0, this check will fail.
    if (data_dump[0] == '\n') {
      // Older versions of glib produce a leading newline. If this is the case,
      // remove it to avoid double-newline after the shebang.
      output_buffer += (data_dump + 1);
    } else {
      output_buffer += data_dump;
    }
    g_free(data_dump);
  }

  g_key_file_free(key_file);
  return output_buffer;
#else
  NOTIMPLEMENTED();
  return std::string();
#endif
}

bool CreateDesktopShortcut(
    const web_app::ShortcutInfo& shortcut_info,
    const web_app::ShortcutLocations& creation_locations) {
  base::AssertBlockingAllowed();

  base::FilePath shortcut_filename;
  if (!shortcut_info.extension_id.empty()) {
    shortcut_filename = GetExtensionShortcutFilename(
        shortcut_info.profile_path, shortcut_info.extension_id);
    // For extensions we do not want duplicate shortcuts. So, delete any that
    // already exist and replace them.
    if (creation_locations.on_desktop)
      DeleteShortcutOnDesktop(shortcut_filename);

    if (creation_locations.applications_menu_location !=
            web_app::APP_MENU_LOCATION_NONE) {
      DeleteShortcutInApplicationsMenu(shortcut_filename, base::FilePath());
    }
  } else {
    shortcut_filename = GetWebShortcutFilename(shortcut_info.url);
  }
  if (shortcut_filename.empty())
    return false;

  std::string icon_name =
      CreateShortcutIcon(shortcut_info.favicon, shortcut_filename);

  std::string app_name =
      web_app::GenerateApplicationNameFromInfo(shortcut_info);

  bool success = true;

  base::FilePath chrome_exe_path = GetChromeExePath();
  if (chrome_exe_path.empty()) {
    NOTREACHED();
    return false;
  }

  if (creation_locations.on_desktop) {
    std::string contents = GetDesktopFileContents(
        chrome_exe_path,
        app_name,
        shortcut_info.url,
        shortcut_info.extension_id,
        shortcut_info.title,
        icon_name,
        shortcut_info.profile_path,
        "",
        false);
    success = CreateShortcutOnDesktop(shortcut_filename, contents);
  }

  if (creation_locations.applications_menu_location ==
          web_app::APP_MENU_LOCATION_NONE) {
    return success;
  }

  base::FilePath directory_filename;
  std::string directory_contents;
  switch (creation_locations.applications_menu_location) {
    case web_app::APP_MENU_LOCATION_HIDDEN:
      break;
    case web_app::APP_MENU_LOCATION_SUBDIR_CHROMEAPPS:
      directory_filename = base::FilePath(kDirectoryFilename);
      directory_contents = GetDirectoryFileContents(
          shell_integration::GetAppShortcutsSubdirName(), "");
      break;
    default:
      NOTREACHED();
      break;
  }

  // Set NoDisplay=true if hidden. This will hide the application from
  // user-facing menus.
  std::string contents = GetDesktopFileContents(
      chrome_exe_path,
      app_name,
      shortcut_info.url,
      shortcut_info.extension_id,
      shortcut_info.title,
      icon_name,
      shortcut_info.profile_path,
      "",
      creation_locations.applications_menu_location ==
          web_app::APP_MENU_LOCATION_HIDDEN);
  success = CreateShortcutInApplicationsMenu(
      shortcut_filename, contents, directory_filename, directory_contents) &&
      success;

  return success;
}

#if BUILDFLAG(ENABLE_APP_LIST)
bool CreateAppListDesktopShortcut(
    const std::string& wm_class,
    const std::string& title) {
  base::AssertBlockingAllowed();

  base::FilePath desktop_name(kAppListDesktopName);
  base::FilePath shortcut_filename = desktop_name.AddExtension("desktop");

  // We do not want duplicate shortcuts. Delete any that already exist and
  // replace them.
  DeleteShortcutInApplicationsMenu(shortcut_filename, base::FilePath());

  base::FilePath chrome_exe_path = GetChromeExePath();
  if (chrome_exe_path.empty()) {
    NOTREACHED();
    return false;
  }

  gfx::ImageFamily icon_images;
  ui::ResourceBundle& resource_bundle = ui::ResourceBundle::GetSharedInstance();
  icon_images.Add(*resource_bundle.GetImageSkiaNamed(IDR_APP_LIST_16));
  icon_images.Add(*resource_bundle.GetImageSkiaNamed(IDR_APP_LIST_32));
  icon_images.Add(*resource_bundle.GetImageSkiaNamed(IDR_APP_LIST_48));
  icon_images.Add(*resource_bundle.GetImageSkiaNamed(IDR_APP_LIST_256));
  std::string icon_name = CreateShortcutIcon(icon_images, desktop_name);

  base::CommandLine command_line(chrome_exe_path);
  command_line.AppendSwitch(switches::kShowAppList);
  std::string contents =
      GetDesktopFileContentsForCommand(command_line,
                                       wm_class,
                                       GURL(),
                                       base::UTF8ToUTF16(title),
                                       icon_name,
                                       kAppListCategories,
                                       false);
  return CreateShortcutInApplicationsMenu(
      shortcut_filename, contents, base::FilePath(), "");
}
#endif

void DeleteDesktopShortcuts(const base::FilePath& profile_path,
                            const std::string& extension_id) {
  base::AssertBlockingAllowed();

  base::FilePath shortcut_filename = GetExtensionShortcutFilename(
      profile_path, extension_id);
  DCHECK(!shortcut_filename.empty());

  DeleteShortcutOnDesktop(shortcut_filename);
  // Delete shortcuts from |kDirectoryFilename|.
  // Note that it is possible that shortcuts were not created in the Chrome Apps
  // directory. It doesn't matter: this will still delete the shortcut even if
  // it isn't in the directory.
  DeleteShortcutInApplicationsMenu(shortcut_filename,
                                   base::FilePath(kDirectoryFilename));
}

void DeleteAllDesktopShortcuts(const base::FilePath& profile_path) {
  base::AssertBlockingAllowed();

  std::unique_ptr<base::Environment> env(base::Environment::Create());

  // Delete shortcuts from Desktop.
  base::FilePath desktop_path;
  if (base::PathService::Get(base::DIR_USER_DESKTOP, &desktop_path)) {
    std::vector<base::FilePath> shortcut_filenames_desktop =
        GetExistingProfileShortcutFilenames(profile_path, desktop_path);
    for (const auto& shortcut : shortcut_filenames_desktop) {
      DeleteShortcutOnDesktop(shortcut);
    }
  }

  // Delete shortcuts from |kDirectoryFilename|.
  base::FilePath applications_menu = GetDataWriteLocation(env.get());
  applications_menu = applications_menu.AppendASCII("applications");
  std::vector<base::FilePath> shortcut_filenames_app_menu =
      GetExistingProfileShortcutFilenames(profile_path, applications_menu);
  for (const auto& menu : shortcut_filenames_app_menu) {
    DeleteShortcutInApplicationsMenu(menu, base::FilePath(kDirectoryFilename));
  }
}

}  // namespace shell_integration_linux
