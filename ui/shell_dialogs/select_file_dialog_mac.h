// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_SHELL_DIALOGS_SELECT_FILE_DIALOG_MAC_H_
#define UI_SHELL_DIALOGS_SELECT_FILE_DIALOG_MAC_H_

#import <Cocoa/Cocoa.h>

#include <map>
#include <set>

#import "base/mac/scoped_nsobject.h"
#include "base/macros.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/shell_dialogs/select_file_dialog.h"
#include "ui/shell_dialogs/shell_dialogs_export.h"

@class ExtensionDropdownHandler;
@class SelectFileDialogBridge;

namespace ui {

namespace test {
class SelectFileDialogMacTest;
}  // namespace test

// Implementation of SelectFileDialog that shows Cocoa dialogs for choosing a
// file or folder.
// Exported for unit tests.
class SHELL_DIALOGS_EXPORT SelectFileDialogImpl : public ui::SelectFileDialog {
 public:
  SelectFileDialogImpl(Listener* listener,
                       std::unique_ptr<ui::SelectFilePolicy> policy);

  // BaseShellDialog implementation.
  bool IsRunning(gfx::NativeWindow parent_window) const override;
  void ListenerDestroyed() override;

  // Callback from ObjC bridge.
  void FileWasSelected(NSSavePanel* dialog,
                       NSWindow* parent_window,
                       bool was_cancelled,
                       bool is_multi,
                       const std::vector<base::FilePath>& files,
                       int index);

 protected:
  // SelectFileDialog implementation.
  // |params| is user data we pass back via the Listener interface.
  void SelectFileImpl(Type type,
                      const base::string16& title,
                      const base::FilePath& default_path,
                      const FileTypeInfo* file_types,
                      int file_type_index,
                      const base::FilePath::StringType& default_extension,
                      gfx::NativeWindow owning_window,
                      void* params) override;

 private:
  friend class test::SelectFileDialogMacTest;

  // Struct to store data associated with a file dialog while it is showing.
  struct DialogData {
    DialogData(void* params_,
               base::scoped_nsobject<ExtensionDropdownHandler> handler);
    DialogData(const DialogData& other);

    // |params| user data associated with this file dialog.
    void* params;

    // Extension dropdown handler corresponding to this file dialog.
    base::scoped_nsobject<ExtensionDropdownHandler> extension_dropdown_handler;

    ~DialogData();
  };

  ~SelectFileDialogImpl() override;

  // Sets the accessory view for the |dialog| and returns the associated
  // ExtensionDropdownHandler.
  static base::scoped_nsobject<ExtensionDropdownHandler> SetAccessoryView(
      NSSavePanel* dialog,
      const FileTypeInfo* file_types,
      int file_type_index,
      const base::FilePath::StringType& default_extension);

  bool HasMultipleFileTypeChoicesImpl() override;

  // The bridge for results from Cocoa to return to us.
  base::scoped_nsobject<SelectFileDialogBridge> bridge_;

  // The set of all parent windows for which we are currently running dialogs.
  std::set<NSWindow*> parents_;

  // A map from file dialogs to the DialogData associated with them.
  std::map<NSSavePanel*, DialogData> dialog_data_map_;

  bool hasMultipleFileTypeChoices_;

  DISALLOW_COPY_AND_ASSIGN(SelectFileDialogImpl);
};

}  // namespace ui

#endif  //  UI_SHELL_DIALOGS_SELECT_FILE_DIALOG_MAC_H_

