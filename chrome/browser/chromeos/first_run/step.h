// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_FIRST_RUN_STEP_H_
#define CHROME_BROWSER_CHROMEOS_FIRST_RUN_STEP_H_

#include <string>

#include "base/macros.h"
#include "base/time/time.h"

namespace chromeos {

class FirstRunActor;
class FirstRunController;

namespace first_run {

class Step {
 public:
  Step(const std::string& name,
       FirstRunController* controller,
       FirstRunActor* actor);
  virtual ~Step();

  // Step shows its content.
  void Show();

  // Called before hiding step.
  void OnBeforeHide();

  // Called after step has been hidden.
  void OnAfterHide();

  const std::string& name() const { return name_; }

 protected:
  FirstRunController* first_run_controller() { return first_run_controller_; }
  FirstRunActor* actor() const { return actor_; }

  // Called from Show method.
  virtual void DoShow() = 0;

  // Called from OnBeforeHide. Step implementation could override this method to
  // react on corresponding event.
  virtual void DoOnBeforeHide() {}

  // Called from OnAfterHide. Step implementation could override this method to
  // react on event.
  virtual void DoOnAfterHide() {}

 private:
  // Records time spent on step to UMA.
  void RecordCompletion();

  std::string name_;
  FirstRunController* first_run_controller_;
  FirstRunActor* actor_;
  base::Time show_time_;

  DISALLOW_COPY_AND_ASSIGN(Step);
};

}  // namespace first_run
}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_FIRST_RUN_STEP_H_
