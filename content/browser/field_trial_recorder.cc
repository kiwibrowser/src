// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/field_trial_recorder.h"

#include "base/metrics/field_trial.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

namespace content {

FieldTrialRecorder::FieldTrialRecorder() = default;

FieldTrialRecorder::~FieldTrialRecorder() = default;

// static
void FieldTrialRecorder::Create(
    mojom::FieldTrialRecorderRequest request) {
  mojo::MakeStrongBinding(std::make_unique<FieldTrialRecorder>(),
                          std::move(request));
}

void FieldTrialRecorder::FieldTrialActivated(const std::string& trial_name) {
  DCHECK(thread_checker_.CalledOnValidThread());
  // Activate the trial in the browser process to match its state in the
  // renderer. This is done by calling FindFullName which finalizes the group
  // and activates the trial.
  base::FieldTrialList::FindFullName(trial_name);
}

}  // namespce content
