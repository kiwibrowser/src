// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "skia/ext/skia_histogram.h"

#include <type_traits>
#include "base/metrics/histogram_macros.h"

// In order to prevent Chrome headers from leaking into Skia, we use a raw
// intptr_t in the header, rather than the base::subtle::AtomicWord. Make sure
// this is a valid assumption.
static_assert(std::is_same<intptr_t, base::subtle::AtomicWord>::value,
              "To allow for header decoupling, skia_histogram.h uses intptr_t "
              "instead of a base::subtle::AtomicWord. These must be the same "
              "type");

namespace skia {

// Wrapper around HISTOGRAM_POINTER_USE - mimics UMA_HISTOGRAM_BOOLEAN but
// allows for an external atomic_histogram_pointer.
void HistogramBoolean(intptr_t* atomic_histogram_pointer,
                      const char* name,
                      bool sample) {
  HISTOGRAM_POINTER_USE(
      atomic_histogram_pointer, name, AddBoolean(sample),
      base::BooleanHistogram::FactoryGet(
          name, base::HistogramBase::kUmaTargetedHistogramFlag));
}

// Wrapper around HISTOGRAM_POINTER_USE - mimics UMA_HISTOGRAM_ENUMERATION but
// allows for an external atomic_histogram_pointer.
void HistogramEnumeration(intptr_t* atomic_histogram_pointer,
                          const char* name,
                          int sample,
                          int boundary_value) {
  HISTOGRAM_POINTER_USE(atomic_histogram_pointer, name, Add(sample),
                        base::LinearHistogram::FactoryGet(
                            name, 1, boundary_value, boundary_value + 1,
                            base::HistogramBase::kUmaTargetedHistogramFlag));
}

}  // namespace skia
