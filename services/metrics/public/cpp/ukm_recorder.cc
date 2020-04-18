// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/metrics/public/cpp/ukm_recorder.h"

#include "base/bind.h"
#include "base/feature_list.h"
#include "build/build_config.h"
#include "services/metrics/public/cpp/delegating_ukm_recorder.h"
#include "services/metrics/public/cpp/ukm_entry_builder.h"

namespace ukm {

const base::Feature kUkmFeature = {"Ukm", base::FEATURE_ENABLED_BY_DEFAULT};

UkmRecorder::UkmRecorder() = default;

UkmRecorder::~UkmRecorder() = default;

// static
UkmRecorder* UkmRecorder::Get() {
  // Note that SourceUrlRecorderWebContentsObserver assumes that
  // DelegatingUkmRecorder::Get() is the canonical UkmRecorder instance. If this
  // changes, SourceUrlRecorderWebContentsObserver should be updated to match.
  return DelegatingUkmRecorder::Get();
}

// static
ukm::SourceId UkmRecorder::GetNewSourceID() {
  return AssignNewSourceId();
}

}  // namespace ukm
