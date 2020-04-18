// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_INSTALLER_MINI_INSTALLER_CONFIGURATION_H_
#define CHROME_INSTALLER_MINI_INSTALLER_CONFIGURATION_H_

#include <windows.h>

namespace mini_installer {

// A simple container of the mini_installer's configuration, as dictated by the
// command line used to invoke it.
class Configuration {
 public:
  enum Operation {
    INSTALL_PRODUCT,
    CLEANUP,
  };

  Configuration();
  ~Configuration();

  // Initializes this instance on the basis of the process's command line.
  bool Initialize(HMODULE module);

  // Returns the desired operation dictated by the command line options.
  Operation operation() const { return operation_; }

  // Returns the program portion of the command line, or NULL if it cannot be
  // determined (e.g., by misuse).
  const wchar_t* program() const;

  // Returns the number of arguments specified on the command line, including
  // the program itself.
  int argument_count() const { return argument_count_; }

  // Returns the original command line.
  const wchar_t* command_line() const { return command_line_; }

  // Returns the app guid to be used for Chrome. --chrome-sxs on the command
  // line makes this the canary's app guid (Google Chrome only).
  const wchar_t* chrome_app_guid() const { return chrome_app_guid_; }

  // Returns true if --system-level is on the command line or if
  // GoogleUpdateIsMachine=1 is set in the process's environment.
  bool is_system_level() const { return is_system_level_; }

  // Returns true if an existing multi-install Chrome is being updated (Google
  // Chrome only).
  bool is_updating_multi_chrome() const { return is_updating_multi_chrome_; }

  // Returns true if any invalid switch is found on the command line.
  bool has_invalid_switch() const { return has_invalid_switch_; }

  // Returns the previous version contained in the image's resource.
  const wchar_t* previous_version() const { return previous_version_; }

 protected:
  void Clear();
  bool ParseCommandLine(const wchar_t* command_line);
  void ReadResources(HMODULE module);

  wchar_t** args_;
  const wchar_t* chrome_app_guid_;
  const wchar_t* command_line_;
  int argument_count_;
  Operation operation_;
  bool is_system_level_;
  bool is_updating_multi_chrome_;
  bool has_invalid_switch_;
  const wchar_t* previous_version_;

 private:
  Configuration(const Configuration&) = delete;
  Configuration& operator=(const Configuration&) = delete;

  // Returns true if multi-install Chrome is already present on the machine.
  bool IsUpdatingMultiChrome() const;
};

}  // namespace mini_installer

#endif  // CHROME_INSTALLER_MINI_INSTALLER_CONFIGURATION_H_
