// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/first_run/step.h"

#include <stddef.h>

#include <cctype>
#include <memory>

#include "base/metrics/histogram_macros.h"
#include "chrome/browser/ui/webui/chromeos/first_run/first_run_actor.h"

namespace {

// Converts from "with-dashes-names" to "WithDashesNames".
std::string ToCamelCase(const std::string& name) {
  std::string result;
  bool next_to_upper = true;
  for (size_t i = 0; i < name.length(); ++i) {
    if (name[i] == '-') {
      next_to_upper = true;
    } else if (next_to_upper) {
      result.push_back(std::toupper(name[i]));
      next_to_upper = false;
    } else {
      result.push_back(name[i]);
    }
  }
  return result;
}

}  // namespace

namespace chromeos {
namespace first_run {

Step::Step(const std::string& name,
           FirstRunController* controller,
           FirstRunActor* actor)
    : name_(name), first_run_controller_(controller), actor_(actor) {
  DCHECK(first_run_controller_);
}

Step::~Step() { RecordCompletion(); }

void Step::Show() {
  show_time_ = base::Time::Now();
  DoShow();
}

void Step::OnBeforeHide() {
  actor()->RemoveBackgroundHoles();
  DoOnBeforeHide();
}

void Step::OnAfterHide() {
  RecordCompletion();
  DoOnAfterHide();
}

void Step::RecordCompletion() {
  if (show_time_.is_null())
    return;
  std::string histogram_name =
      "CrosFirstRun.TimeSpentOnStep" + ToCamelCase(name());
  // Equivalent to using UMA_HISTOGRAM_CUSTOM_TIMES with 50 buckets on range
  // [100ms, 3 min.]. UMA_HISTOGRAM_CUSTOM_TIMES can not be used here, because
  // |histogram_name| is calculated dynamically and changes from call to call.
  base::HistogramBase* histogram = base::Histogram::FactoryTimeGet(
      histogram_name,
      base::TimeDelta::FromMilliseconds(100),
      base::TimeDelta::FromMinutes(3),
      50,
      base::HistogramBase::kUmaTargetedHistogramFlag);
  histogram->AddTime(base::Time::Now() - show_time_);
  show_time_ = base::Time();
}

}  // namespace first_run
}  // namespace chromeos

