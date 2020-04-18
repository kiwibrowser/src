// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_INSTALLER_UTIL_SELF_REG_WORK_ITEM_H__
#define CHROME_INSTALLER_UTIL_SELF_REG_WORK_ITEM_H__

#include <windows.h>
#include <string>

#include "chrome/installer/util/work_item.h"

// Registers or unregisters the DLL at the given path.
class SelfRegWorkItem : public WorkItem {
 public:
  ~SelfRegWorkItem() override;

 private:
  friend class WorkItem;

  // Constructs a work item that will call upon a self-registering DLL to
  // register itself.
  // dll_path: The path to the DLL.
  // do_register: Whether this action is to register or unregister the DLL.
  // user_level_registration: If true, then the exports called
  //    "DllRegisterUserServer" and "DllUnregisterUserServer" will be called to
  //    register and unregister the DLL. If false, the default exports named
  //    "DllRegisterServer" and "DllUnregisterUserServer" will be used.
  SelfRegWorkItem(const std::wstring& dll_path, bool do_register,
                  bool user_level_registration);

  // WorkItem:
  bool DoImpl() override;
  void RollbackImpl() override;

  // Examines the DLL at dll_path looking for either DllRegisterServer (if
  // do_register is true) or DllUnregisterServer (if do_register is false).
  // Returns true if the DLL exports the function and it a call to it
  // succeeds, false otherwise.
  bool RegisterDll(bool do_register);

  // The path to the dll to be registered.
  std::wstring dll_path_;

  // Whether this work item will register or unregister the dll. The rollback
  // action just inverts this parameter.
  bool do_register_;

  // Whether to use alternate export names on the DLL that will perform
  // user level registration.
  bool user_level_registration_;
};

#endif  // CHROME_INSTALLER_UTIL_SELF_REG_WORK_ITEM_H__
