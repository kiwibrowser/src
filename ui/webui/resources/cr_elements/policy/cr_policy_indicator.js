// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @fileoverview Polymer element for indicating policies by type. */
Polymer({
  is: 'cr-policy-indicator',

  behaviors: [CrPolicyIndicatorBehavior],

  properties: {
    iconAriaLabel: String,
  },
});
