// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.vr_shell.rules;

import org.chromium.chrome.browser.vr_shell.rules.VrActivityRestriction.SupportedActivity;

/**
 * Interface to be implemented by *VrTestRule rules, which allows them to be
 * conditionally skipped when used in conjunction with VrActivityRestrictionRule.
 */
public interface VrTestRule {
    /**
     * Get the VrActivityRestriction.SupportedActivity that this rule is restricted to running in.
     */
    public SupportedActivity getRestriction();

    /**
     * Whether the head tracking mode has been changed.
     */
    public boolean isTrackerDirty();

    /**
     * Tells the rule that the head tracking mode has been changed.
     */
    public void setTrackerDirty();
}
