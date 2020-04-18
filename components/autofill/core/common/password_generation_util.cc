// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/autofill/core/common/password_generation_util.h"

#include "base/command_line.h"
#include "base/metrics/field_trial.h"
#include "base/metrics/histogram_macros.h"
#include "components/autofill/core/common/autofill_switches.h"

namespace autofill {
namespace password_generation {

PasswordGenerationActions::PasswordGenerationActions()
    : learn_more_visited(false),
      password_accepted(false),
      password_edited(false),
      password_regenerated(false) {
}

PasswordGenerationActions::~PasswordGenerationActions() {
}

void LogUserActions(PasswordGenerationActions actions) {
  UserAction action = IGNORE_FEATURE;
  if (actions.password_accepted) {
    if (actions.password_edited)
      action = ACCEPT_AFTER_EDITING;
    else
      action = ACCEPT_ORIGINAL_PASSWORD;
  } else if (actions.learn_more_visited) {
    action = LEARN_MORE;
  }
  UMA_HISTOGRAM_ENUMERATION("PasswordGeneration.UserActions",
                            action, ACTION_ENUM_COUNT);
}

void LogPasswordGenerationEvent(PasswordGenerationEvent event) {
  UMA_HISTOGRAM_ENUMERATION("PasswordGeneration.Event",
                            event, EVENT_ENUM_COUNT);
}

bool IsPasswordGenerationEnabled() {
  // Always fetch the field trial group to ensure it is reported correctly.
  // The command line flags will be associated with a group that is reported
  // so long as trial is actually queried.
  std::string group_name =
      base::FieldTrialList::FindFullName("PasswordGeneration");

  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  if (command_line->HasSwitch(switches::kDisablePasswordGeneration))
    return false;

  if (command_line->HasSwitch(switches::kEnablePasswordGeneration))
    return true;

  return group_name != "Disabled";
}

}  // namespace password_generation
}  // namespace autofill
