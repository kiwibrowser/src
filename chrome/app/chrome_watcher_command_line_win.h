// Copyright (c) 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_APP_CHROME_WATCHER_COMMAND_LINE_WIN_H_
#define CHROME_APP_CHROME_WATCHER_COMMAND_LINE_WIN_H_

#include <windows.h>

#include <memory>
#include <vector>

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/win/scoped_handle.h"

// Class for configuring the Chrome watcher process via the command-line. This
// accepts configuration from the parent process and generates the required
// command-line.
//
// It provides functionality for the common task of sharing handles from a
// parent to a child process, automatically duplicating them with the
// appropriate permissions. These handles are closed when the generator is
// destroyed so the generator must outlive the process creation.
class ChromeWatcherCommandLineGenerator {
 public:
  explicit ChromeWatcherCommandLineGenerator(const base::FilePath& chrome_exe);
  ~ChromeWatcherCommandLineGenerator();

  // Sets a handle to be shared with the child process. This will duplicate the
  // handle with the inherit flag, with this object retaining ownership of the
  // duplicated handle. As such, this object must live at least until after the
  // watcher process has been created. Returns true on success, false on
  // failure. This can fail if the call to DuplicateHandle fails. Each of these
  // may only be called once.
  bool SetOnInitializedEventHandle(HANDLE on_initialized_event_handle);
  bool SetParentProcessHandle(HANDLE parent_process_handle_);

  // Generates a command-line representing this configuration. Must be run from
  // the main thread of the parent process. This should only be called after the
  // generator is fully configured.
  base::CommandLine GenerateCommandLine();

  // Appends all inherited handles to the provided vector. Does not clear the
  // vector first.
  void GetInheritedHandles(std::vector<HANDLE>* inherited_handles) const;

 protected:
  // Exposed for unittesting.

  // Duplicate the provided |handle|, making it inheritable, and storing it in
  // the provided |scoped_handle|.
  bool SetHandle(HANDLE handle, base::win::ScopedHandle* scoped_handle);

  base::FilePath chrome_exe_;

  // Duplicated inheritable handles that are being shared from the parent to the
  // child.
  base::win::ScopedHandle on_initialized_event_handle_;
  base::win::ScopedHandle parent_process_handle_;

 private:
  DISALLOW_COPY_AND_ASSIGN(ChromeWatcherCommandLineGenerator);
};

// Class for processing a command-line used to start a Chrome Watcher process
// and exposing the resulting configuration and any shared handles. To be used
// within the watcher process.
class ChromeWatcherCommandLine {
 public:
  // Causes the process to explode if any inherited handles weren't taken from
  // this object.
  ~ChromeWatcherCommandLine();

  // Parses the provided command-line used to launch a Chrome Watcher process.
  // If this fails any successfully opened handles will be closed prior to
  // return. Returns a ChromeWatcherCommandLine object on success, nullptr
  // otherwise.
  static std::unique_ptr<ChromeWatcherCommandLine> InterpretCommandLine(
      const base::CommandLine& command_line);

  // Accessors for handles. Any handles not taken from this object at the time
  // of its destruction will cause a terminal error.
  base::win::ScopedHandle TakeOnInitializedEventHandle();
  base::win::ScopedHandle TakeParentProcessHandle();

  // Returns the ID of the main thread in the parent process.
  DWORD main_thread_id() const { return main_thread_id_; }

 private:
  // Private constructor for use by InterpretCommandLine.
  ChromeWatcherCommandLine(HANDLE on_initialized_event_handle,
                           HANDLE parent_process_handle,
                           DWORD main_thread_id);

  base::win::ScopedHandle on_initialized_event_handle_;
  base::win::ScopedHandle parent_process_handle_;

  // The ID of the main thread in the parent process.
  DWORD main_thread_id_;

  DISALLOW_COPY_AND_ASSIGN(ChromeWatcherCommandLine);
};

// Generates a CommandLine that will launch |chrome_exe| in Chrome Watcher mode
// to observe |parent_process|, whose main thread is identified by
// |main_thread_id|. The watcher process will signal |on_initialized_event| when
// its initialization is complete.
base::CommandLine GenerateChromeWatcherCommandLine(
    const base::FilePath& chrome_exe,
    HANDLE parent_process,
    DWORD main_thread_id,
    HANDLE on_initialized_event);

// Interprets the Command Line used to launch a Chrome Watcher process and
// extracts the parent process and initialization event HANDLEs and the parent
// process main thread ID. Verifies that the handles are usable in this process
// before returning them. Returns true if all parameters are successfully parsed
// and false otherwise. In case of partial failure, any successfully parsed
// HANDLEs will be closed.
bool InterpretChromeWatcherCommandLine(
    const base::CommandLine& command_line,
    base::win::ScopedHandle* parent_process,
    DWORD* main_thread_id,
    base::win::ScopedHandle* on_initialized_event);

#endif  // CHROME_APP_CHROME_WATCHER_COMMAND_LINE_WIN_H_
