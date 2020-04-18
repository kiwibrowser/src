// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_SHELL_DIALOGS_BASE_SHELL_DIALOG_WIN_H_
#define UI_SHELL_DIALOGS_BASE_SHELL_DIALOG_WIN_H_

#include <shlobj.h>
#include <set>

#include "base/macros.h"
#include "ui/shell_dialogs/base_shell_dialog.h"
#include "ui/shell_dialogs/shell_dialogs_export.h"

namespace base {
class Thread;
}

namespace ui {

///////////////////////////////////////////////////////////////////////////////
// A base class for all shell dialog implementations that handles showing a
// shell dialog modally on its own thread.
class SHELL_DIALOGS_EXPORT BaseShellDialogImpl {
 public:
  BaseShellDialogImpl();
  virtual ~BaseShellDialogImpl();

 protected:
  // Represents a run of a dialog.
  struct RunState {
    // Owning HWND, may be null.
    HWND owner;

    // Thread dialog is run on.
    base::Thread* dialog_thread;
  };

  // Called at the beginning of a modal dialog run. Disables the owner window
  // and tracks it. Returns the message loop of the thread that the dialog will
  // be run on.
  RunState BeginRun(HWND owner);

  // Cleans up after a dialog run. If the run_state has a valid HWND this makes
  // sure that the window is enabled. This is essential because BeginRun
  // aggressively guards against multiple modal dialogs per HWND. Must be called
  // on the UI thread after the result of the dialog has been determined.
  //
  // In addition this deletes the Thread in RunState.
  void EndRun(RunState run_state);

  // Returns true if a modal shell dialog is currently active for the specified
  // owner. Must be called on the UI thread.
  bool IsRunningDialogForOwner(HWND owner) const;

  // Disables the window |owner|. Can be run from either the ui or the dialog
  // thread. Can be called on either the UI or the dialog thread. This function
  // is called on the dialog thread after the modal Windows Common dialog
  // functions return because Windows automatically re-enables the owning
  // window when those functions return, but we don't actually want them to be
  // re-enabled until the response of the dialog propagates back to the UI
  // thread, so we disable the owner manually after the Common dialog function
  // returns.
  void DisableOwner(HWND owner);

 private:
  typedef std::set<HWND> Owners;

  // Creates a thread to run a shell dialog on. Each dialog requires its own
  // thread otherwise in some situations where a singleton owns a single
  // instance of this object we can have a situation where a modal dialog in
  // one window blocks the appearance of a modal dialog in another.
  static base::Thread* CreateDialogThread();

  // Enables the window |owner_|. Can only be run from the ui thread.
  void EnableOwner(HWND owner);

  // A list of windows that currently own active shell dialogs for this
  // instance. For example, if the DownloadManager owns an instance of this
  // object and there are two browser windows open both with Save As dialog
  // boxes active, this list will consist of the two browser windows' HWNDs.
  // The derived class must call EndRun once the dialog is done showing to
  // remove the owning HWND from this list.
  // This object is static since it is maintained for all instances of this
  // object - i.e. you can't have two file pickers open for the
  // same owner, even though they might be represented by different instances
  // of this object.
  // This set only contains non-null HWNDs. NULL hwnds are not added to this
  // list.
  static Owners owners_;
  static int instance_count_;

  DISALLOW_COPY_AND_ASSIGN(BaseShellDialogImpl);
};

}  // namespace ui

#endif  // UI_SHELL_DIALOGS_BASE_SHELL_DIALOG_WIN_H_

