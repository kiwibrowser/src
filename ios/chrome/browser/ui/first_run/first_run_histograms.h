// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_FIRST_RUN_FIRST_RUN_HISTOGRAMS_H_
#define IOS_CHROME_BROWSER_UI_FIRST_RUN_FIRST_RUN_HISTOGRAMS_H_

#include "base/metrics/histogram.h"

// HISTOGRAM_POINTER_BLOCK differ from STATIC_HISTOGRAM_POINTER_BLOCK
// on using static histogram pointer, in this one we create a new histogram
// from the histogram_factory_get_invocation each time this is called.
// This is needed on the first run because the same funciton try to log
// different histogram names, also we don't need static histogram as first run
// logging only happens once (may be little more in some cases).
#define HISTOGRAM_POINTER_BLOCK(constant_histogram_name,                       \
                                histogram_add_method_invocation,               \
                                histogram_factory_get_invocation)              \
  do {                                                                         \
    base::HistogramBase* histogram_pointer = histogram_factory_get_invocation; \
    histogram_pointer->histogram_add_method_invocation;                        \
  } while (0)

// This UMA_HISTOGRAM_CUSTOM_TIMES_FIRST_RUN uses the HISTOGRAM_POINTER_BLOCK
// instead of STATIC_HISTOGRAM_POINTER_BLOCK which handles being called with
// different names from same function.
#define UMA_HISTOGRAM_CUSTOM_TIMES_FIRST_RUN(name, sample, min, max, \
                                             bucket_count)           \
  HISTOGRAM_POINTER_BLOCK(name, AddTime(sample),                     \
                          base::Histogram::FactoryTimeGet(           \
                              name, min, max, bucket_count,          \
                              base::HistogramBase::kUmaTargetedHistogramFlag))

#endif  // IOS_CHROME_BROWSER_UI_FIRST_RUN_FIRST_RUN_HISTOGRAMS_H_
