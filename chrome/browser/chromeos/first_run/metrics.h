// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_FIRST_RUN_METRICS_H_
#define CHROME_BROWSER_CHROMEOS_FIRST_RUN_METRICS_H_

namespace chromeos {
namespace first_run {

enum TutorialCompletion {
  // User left tutorial before finish.
  TUTORIAL_NOT_FINISHED,

  // Tutorial was completed with "Got It" button.
  TUTORIAL_COMPLETED_WITH_GOT_IT,

  // Tutorial was completed with "Keep Exploring" button, i.e. Help App was
  // launched after tutorial.
  TUTORIAL_COMPLETED_WITH_KEEP_EXPLORING,

  // Must be the last element.
  TUTORIAL_COMPLETION_SIZE
};

}  // namespace first_run
}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_FIRST_RUN_METRICS_H_
