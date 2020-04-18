// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/installer/util/callback_work_item.h"

#include "base/callback.h"
#include "base/logging.h"
#include "chrome/installer/util/work_item.h"

CallbackWorkItem::CallbackWorkItem(
    base::Callback<bool(const CallbackWorkItem&)> callback)
    : callback_(callback),
      roll_state_(RS_UNDEFINED) {
}

CallbackWorkItem::~CallbackWorkItem() {
}

bool CallbackWorkItem::DoImpl() {
  DCHECK_EQ(roll_state_, RS_UNDEFINED);

  roll_state_ = RS_FORWARD;
  bool result = callback_.Run(*this);
  roll_state_ = RS_UNDEFINED;

  return result;
}

void CallbackWorkItem::RollbackImpl() {
  DCHECK_EQ(roll_state_, RS_UNDEFINED);

  roll_state_ = RS_BACKWARD;
  ignore_result(callback_.Run(*this));
  roll_state_ = RS_UNDEFINED;
}

bool CallbackWorkItem::IsRollback() const {
  DCHECK_NE(roll_state_, RS_UNDEFINED);
  return roll_state_ == RS_BACKWARD;
}
