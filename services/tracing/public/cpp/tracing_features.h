// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file defines all the public base::FeatureList features for the
// services/tracing module.

#ifndef SERVICES_TRACING_PUBLIC_CPP_TRACING_FEATURES_H_
#define SERVICES_TRACING_PUBLIC_CPP_TRACING_FEATURES_H_

#include "base/component_export.h"
#include "base/feature_list.h"

namespace features {

// The features should be documented alongside the definition of their values
// in the .cc file.
extern const COMPONENT_EXPORT(TRACING_CPP) base::Feature
    kTracingPerfettoBackend;

}  // namespace features

namespace tracing {

bool COMPONENT_EXPORT(TRACING_CPP) TracingUsesPerfettoBackend();

}  // namespace tracing

#endif  // SERVICES_TRACING_PUBLIC_CPP_TRACING_FEATURES_H_
