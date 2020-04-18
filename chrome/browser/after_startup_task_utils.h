// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_AFTER_STARTUP_TASK_UTILS_H_
#define CHROME_BROWSER_AFTER_STARTUP_TASK_UTILS_H_

#include "base/bind.h"
#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/task_runner.h"

namespace android {
class AfterStartupTaskUtilsJNI;
}

class AfterStartupTaskUtils {
 public:
  // A helper TaskRunner which merely forwards to
  // AfterStartupTaskUtils::PostTask(). Doesn't support tasks with a non-zero
  // delay.
  class Runner : public base::TaskRunner {
   public:
    explicit Runner(scoped_refptr<base::TaskRunner> destination_runner);

    // Overrides from base::TaskRunner:
    bool PostDelayedTask(const base::Location& from_here,
                         base::OnceClosure task,
                         base::TimeDelta delay) override;
    bool RunsTasksInCurrentSequence() const override;

   private:
    ~Runner() override;

    const scoped_refptr<base::TaskRunner> destination_runner_;

    DISALLOW_COPY_AND_ASSIGN(Runner);
  };

  // Observes startup and when complete runs tasks that have accrued.
  static void StartMonitoringStartup();

  // Used to augment the behavior of BrowserThread::PostAfterStartupTask
  // for chrome. Tasks are queued until startup is complete.
  // Note: see browser_thread.h
  static void PostTask(
      const base::Location& from_here,
      const scoped_refptr<base::TaskRunner>& destination_runner,
      base::OnceClosure task);

  // Returns true if browser startup is complete. Only use this on a one-off
  // basis; If you need to poll this function constantly, use the above
  // PostTask() API instead.
  static bool IsBrowserStartupComplete();

  // For use by unit tests where we don't have normal content loading
  // infrastructure and thus StartMonitoringStartup() is unsuitable.
  static void SetBrowserStartupIsCompleteForTesting();

  static void UnsafeResetForTesting();

 private:
  // TODO(wkorman): Look into why Android calls
  // SetBrowserStartupIsComplete() directly. Ideally it would use
  // StartMonitoringStartup() as the normal approach.
  friend class android::AfterStartupTaskUtilsJNI;

  static void SetBrowserStartupIsComplete();

  DISALLOW_IMPLICIT_CONSTRUCTORS(AfterStartupTaskUtils);
};

#endif  // CHROME_BROWSER_AFTER_STARTUP_TASK_UTILS_H_
