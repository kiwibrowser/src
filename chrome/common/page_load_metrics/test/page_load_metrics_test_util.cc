// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/page_load_metrics/test/page_load_metrics_test_util.h"

#include "chrome/common/page_load_metrics/page_load_metrics.mojom.h"
#include "chrome/common/page_load_metrics/page_load_metrics_util.h"

using page_load_metrics::OptionalMin;

void PopulateRequiredTimingFields(
    page_load_metrics::mojom::PageLoadTiming* inout_timing) {
  if (inout_timing->interactive_timing->interactive_detection &&
      !inout_timing->interactive_timing->interactive) {
    inout_timing->interactive_timing->interactive =
        inout_timing->interactive_timing->interactive_detection;
  }
  if (inout_timing->interactive_timing->interactive &&
      !inout_timing->paint_timing->first_meaningful_paint) {
    inout_timing->paint_timing->first_meaningful_paint =
        inout_timing->interactive_timing->interactive;
  }
  if (inout_timing->paint_timing->first_meaningful_paint &&
      !inout_timing->paint_timing->first_contentful_paint) {
    inout_timing->paint_timing->first_contentful_paint =
        inout_timing->paint_timing->first_meaningful_paint;
  }
  if ((inout_timing->paint_timing->first_text_paint ||
       inout_timing->paint_timing->first_image_paint ||
       inout_timing->paint_timing->first_contentful_paint) &&
      !inout_timing->paint_timing->first_paint) {
    inout_timing->paint_timing->first_paint =
        OptionalMin(OptionalMin(inout_timing->paint_timing->first_text_paint,
                                inout_timing->paint_timing->first_image_paint),
                    inout_timing->paint_timing->first_contentful_paint);
  }
  if (inout_timing->paint_timing->first_paint &&
      !inout_timing->document_timing->first_layout) {
    inout_timing->document_timing->first_layout =
        inout_timing->paint_timing->first_paint;
  }
  if (inout_timing->document_timing->load_event_start &&
      !inout_timing->document_timing->dom_content_loaded_event_start) {
    inout_timing->document_timing->dom_content_loaded_event_start =
        inout_timing->document_timing->load_event_start;
  }
  if (inout_timing->document_timing->first_layout &&
      !inout_timing->parse_timing->parse_start) {
    inout_timing->parse_timing->parse_start =
        inout_timing->document_timing->first_layout;
  }
  if (inout_timing->document_timing->dom_content_loaded_event_start &&
      !inout_timing->parse_timing->parse_stop) {
    inout_timing->parse_timing->parse_stop =
        inout_timing->document_timing->dom_content_loaded_event_start;
  }
  if (inout_timing->parse_timing->parse_stop &&
      !inout_timing->parse_timing->parse_start) {
    inout_timing->parse_timing->parse_start =
        inout_timing->parse_timing->parse_stop;
  }
  if (inout_timing->parse_timing->parse_start &&
      !inout_timing->response_start) {
    inout_timing->response_start = inout_timing->parse_timing->parse_start;
  }
  if (inout_timing->parse_timing->parse_start) {
    if (!inout_timing->parse_timing->parse_blocked_on_script_load_duration)
      inout_timing->parse_timing->parse_blocked_on_script_load_duration =
          base::TimeDelta();
    if (!inout_timing->parse_timing
             ->parse_blocked_on_script_execution_duration) {
      inout_timing->parse_timing->parse_blocked_on_script_execution_duration =
          base::TimeDelta();
    }
    if (!inout_timing->parse_timing
             ->parse_blocked_on_script_load_from_document_write_duration) {
      inout_timing->parse_timing
          ->parse_blocked_on_script_load_from_document_write_duration =
          base::TimeDelta();
    }
    if (!inout_timing->parse_timing
             ->parse_blocked_on_script_execution_from_document_write_duration) {
      inout_timing->parse_timing
          ->parse_blocked_on_script_execution_from_document_write_duration =
          base::TimeDelta();
    }
  }
}
