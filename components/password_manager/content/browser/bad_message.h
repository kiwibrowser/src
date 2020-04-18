// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PASSWORD_MANAGER_CONTENT_BROWSER_BAD_MESSAGE_H_
#define COMPONENTS_PASSWORD_MANAGER_CONTENT_BROWSER_BAD_MESSAGE_H_

namespace content {
class RenderProcessHost;
}

namespace password_manager {
// The browser process often chooses to terminate a renderer if it receives
// a bad IPC message. The reasons are tracked for metrics.
//
// See also content/browser/bad_message.h.
//
// NOTE: Do not remove or reorder elements in this list. Add new entries at the
// end. Items may be renamed but do not change the values. We rely on the enum
// values in histograms.
enum class BadMessageReason {
  CPMD_BAD_ORIGIN_FORMS_PARSED = 1,
  CPMD_BAD_ORIGIN_FORMS_RENDERED = 2,
  CPMD_BAD_ORIGIN_FORM_SUBMITTED = 3,
  CPMD_BAD_ORIGIN_FOCUSED_PASSWORD_FORM_FOUND = 4,
  CPMD_BAD_ORIGIN_IN_PAGE_NAVIGATION = 5,
  CPMD_BAD_ORIGIN_PASSWORD_NO_LONGER_GENERATED = 6,
  CPMD_BAD_ORIGIN_PRESAVE_GENERATED_PASSWORD = 7,
  CPMD_BAD_ORIGIN_SAVE_GENERATION_FIELD_DETECTED_BY_CLASSIFIER = 8,
  CPMD_BAD_ORIGIN_SHOW_FALLBACK_FOR_SAVING = 9,

  // Please add new elements here. The naming convention is abbreviated class
  // name (e.g. ContentPasswordManagerDriver becomes CPMD) plus a unique
  // description of the reason. After making changes, you MUST update
  // histograms.xml by running:
  // "python tools/metrics/histograms/update_bad_message_reasons.py"
  BAD_MESSAGE_MAX
};

namespace bad_message {
// Called when the browser receives a bad IPC message from a renderer process on
// the UI thread. Logs the event, records a histogram metric for the |reason|,
// and terminates the process for |host|.
void ReceivedBadMessage(content::RenderProcessHost* host,
                        BadMessageReason reason);

}  // namespace bad_message
}  // namespace password_manager

#endif  // COMPONENTS_PASSWORD_MANAGER_CONTENT_BROWSER_BAD_MESSAGE_H_
