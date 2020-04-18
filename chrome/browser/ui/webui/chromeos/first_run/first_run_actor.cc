// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/chromeos/first_run/first_run_actor.h"

#include <limits>

#include "base/values.h"

namespace {
const int kNoneValue = std::numeric_limits<int>::min();
}

namespace chromeos {

FirstRunActor::StepPosition::StepPosition()
    : top_(kNoneValue),
      right_(kNoneValue),
      bottom_(kNoneValue),
      left_(kNoneValue) {
}

FirstRunActor::StepPosition& FirstRunActor::StepPosition::SetTop(int top) {
  top_ = top;
  return *this;
}

FirstRunActor::StepPosition& FirstRunActor::StepPosition::SetRight(int right) {
  right_ = right;
  return *this;
}

FirstRunActor::StepPosition&
FirstRunActor::StepPosition::SetBottom(int bottom) {
  bottom_ = bottom;
  return *this;
}

FirstRunActor::StepPosition& FirstRunActor::StepPosition::SetLeft(int left) {
  left_ = left;
  return *this;
}

base::DictionaryValue FirstRunActor::StepPosition::AsValue() const {
  base::DictionaryValue result;
  if (top_ != kNoneValue)
    result.SetInteger("top", top_);
  if (right_ != kNoneValue)
    result.SetInteger("right", right_);
  if (bottom_ != kNoneValue)
    result.SetInteger("bottom", bottom_);
  if (left_ != kNoneValue)
    result.SetInteger("left", left_);
  return result;
}

FirstRunActor::FirstRunActor()
    : delegate_(NULL) {
}

FirstRunActor::~FirstRunActor() {
  if (delegate())
    delegate()->OnActorDestroyed();
  delegate_ = NULL;
}

}  // namespace chromeos

