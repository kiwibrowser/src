// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SECURITY_STATE_CORE_INSECURE_INPUT_EVENT_DATA_H_
#define COMPONENTS_SECURITY_STATE_CORE_INSECURE_INPUT_EVENT_DATA_H_

namespace security_state {

// Stores flags about Input Events in the page that may influence
// its security level.
struct InsecureInputEventData {
  InsecureInputEventData()
      : insecure_field_edited(false),
        password_field_shown(false),
        credit_card_field_edited(false) {}
  ~InsecureInputEventData() {}

  // True if the user edited a field on a non-secure page.
  bool insecure_field_edited;

  // True if a password input field was shown on a non-secure page.
  bool password_field_shown;

  // True if a credit card input field was edited on a non-secure page.
  bool credit_card_field_edited;
};

}  // namespace security_state

#endif  // COMPONENTS_SECURITY_STATE_CORE_INSECURE_INPUT_EVENT_DATA_H_
