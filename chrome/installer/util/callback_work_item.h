// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_INSTALLER_UTIL_CALLBACK_WORK_ITEM_H_
#define CHROME_INSTALLER_UTIL_CALLBACK_WORK_ITEM_H_

#include "base/callback.h"
#include "base/compiler_specific.h"
#include "base/gtest_prod_util.h"
#include "chrome/installer/util/work_item.h"

// A work item that invokes a callback on Do() and Rollback().  In the following
// example, the function SomeWorkItemCallback() will be invoked when an item
// list is processed.
//
// // A callback invoked to do some work.
// bool SomeWorkItemCallback(const CallbackWorkItem& item) {
//   if (item.IsRollback()) {
//     // Rollback work goes here.  The return value is ignored in this case.
//     return true;
//   }
//
//   // Roll forward work goes here.  The return value indicates success/failure
//   // of the item.
//   return true;
// }
//
// void SomeFunctionThatAddsItemsToAList(WorkItemList* item_list) {
//   ...
//   item_list->AddCallbackWorkItem(base::Bind(&SomeWorkItemCallback));
//   ...
// }
class CallbackWorkItem : public WorkItem {
 public:
  ~CallbackWorkItem() override;

  bool IsRollback() const;

 private:
  friend class WorkItem;

  enum RollState {
    RS_UNDEFINED,
    RS_FORWARD,
    RS_BACKWARD,
  };

  explicit CallbackWorkItem(
      base::Callback<bool(const CallbackWorkItem&)> callback);

  // WorkItem:
  bool DoImpl() override;
  void RollbackImpl() override;

  base::Callback<bool(const CallbackWorkItem&)> callback_;
  RollState roll_state_;

  FRIEND_TEST_ALL_PREFIXES(CallbackWorkItemTest, TestFailure);
  FRIEND_TEST_ALL_PREFIXES(CallbackWorkItemTest, TestForwardBackward);
};

#endif  // CHROME_INSTALLER_UTIL_CALLBACK_WORK_ITEM_H_
