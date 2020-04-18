// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/omnibox/browser/omnibox_log.h"

OmniboxLog::OmniboxLog(
    const base::string16& text,
    bool just_deleted_text,
    metrics::OmniboxInputType input_type,
    bool is_popup_open,
    size_t selected_index,
    WindowOpenDisposition disposition,
    bool is_paste_and_go,
    SessionID tab_id,
    metrics::OmniboxEventProto::PageClassification current_page_classification,
    base::TimeDelta elapsed_time_since_user_first_modified_omnibox,
    size_t completed_length,
    base::TimeDelta elapsed_time_since_last_change_to_default_match,
    const AutocompleteResult& result)
    : text(text),
      just_deleted_text(just_deleted_text),
      input_type(input_type),
      is_popup_open(is_popup_open),
      selected_index(selected_index),
      disposition(disposition),
      is_paste_and_go(is_paste_and_go),
      tab_id(tab_id),
      current_page_classification(current_page_classification),
      elapsed_time_since_user_first_modified_omnibox(
          elapsed_time_since_user_first_modified_omnibox),
      completed_length(completed_length),
      elapsed_time_since_last_change_to_default_match(
          elapsed_time_since_last_change_to_default_match),
      result(result) {}

OmniboxLog::~OmniboxLog() {}
