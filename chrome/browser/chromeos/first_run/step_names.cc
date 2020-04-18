// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/first_run/step_names.h"

namespace chromeos {
namespace first_run {

// This values should be synced with ids of corresponding steps in HTML-side.
// Also there are metric recording how much time user spent on every step.
// Metric's name has format "CrosFirstRun.TimeSpentOnStep[StepName]", where
// |StepName| is camel-cased version of |step-name|. Corresponding record
// should be added to "histograms.xml" file for every step listed here.
const char kAppListStep[] = "app-list";
const char kTrayStep[] = "tray";
const char kHelpStep[] = "help";

}  // namespace first_run
}  // namespace chromeos

