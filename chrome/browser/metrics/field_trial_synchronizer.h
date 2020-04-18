// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_METRICS_FIELD_TRIAL_SYNCHRONIZER_H_
#define CHROME_BROWSER_METRICS_FIELD_TRIAL_SYNCHRONIZER_H_

#include <string>
#include <vector>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/metrics/field_trial.h"

// This class is used by the browser process to communicate FieldTrial setting
// (field trial name and group) to any previously started renderers.
//
// This class registers itself as an observer of FieldTrialList. FieldTrialList
// notifies this class by calling it's OnFieldTrialGroupFinalized method when a
// group is selected (finalized) for a FieldTrial and OnFieldTrialGroupFinalized
// method sends the FieldTrial's name and the group to all renderer processes.
// Each renderer process creates the FieldTrial, and by using a 100% probability
// for the FieldTrial, forces the FieldTrial to have the same group string.

class FieldTrialSynchronizer
    : public base::RefCountedThreadSafe<FieldTrialSynchronizer>,
      public base::FieldTrialList::Observer {
 public:
  // Construction also sets up the global singleton instance.  This instance is
  // used to communicate between the UI and other threads, and is destroyed only
  // as the main thread (browser_main) terminates, which means all other threads
  // have completed, and will not need this instance any further. It adds itself
  // as an observer of FieldTrialList so that it gets notified whenever a group
  // is finalized in the browser process.
  FieldTrialSynchronizer();

  // Notify all renderer processes about the |group_name| that is finalized for
  // the given field trail (|field_trial_name|). This is called on UI thread.
  void NotifyAllRenderers(const std::string& field_trial_name,
                          const std::string& group_name);

  // FieldTrialList::Observer methods:

  // This method is called by the FieldTrialList singleton when a trial's group
  // is finalized. This method contacts all renderers (by calling
  // NotifyAllRenderers) to create a FieldTrial that carries the randomly
  // selected state from the browser process into all the renderer processes.
  void OnFieldTrialGroupFinalized(const std::string& name,
                                  const std::string& group_name) override;

 private:
  friend class base::RefCountedThreadSafe<FieldTrialSynchronizer>;
  ~FieldTrialSynchronizer() override;

  DISALLOW_COPY_AND_ASSIGN(FieldTrialSynchronizer);
};

#endif  // CHROME_BROWSER_METRICS_FIELD_TRIAL_SYNCHRONIZER_H_
