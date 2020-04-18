// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.vr_shell.rules;

import org.junit.runner.Description;
import org.junit.runners.model.Statement;

import org.chromium.chrome.browser.vr_shell.TestVrShellDelegate;
import org.chromium.chrome.browser.vr_shell.rules.VrActivityRestriction.SupportedActivity;
import org.chromium.chrome.browser.vr_shell.util.HeadTrackingUtils;
import org.chromium.chrome.browser.webapps.WebappActivityTestRule;

/**
 * VR extension of WebappActivityTestRule. Applies WebappActivityTestRule then opens
 * up a WebappActivity to a blank page.
 */
public class WebappActivityVrTestRule extends WebappActivityTestRule implements VrTestRule {
    private boolean mTrackerDirty;

    @Override
    public Statement apply(final Statement base, final Description desc) {
        return super.apply(new Statement() {
            @Override
            public void evaluate() throws Throwable {
                HeadTrackingUtils.checkForAndApplyHeadTrackingModeAnnotation(
                        WebappActivityVrTestRule.this, desc);
                startWebappActivity();
                TestVrShellDelegate.createTestVrShellDelegate(getActivity());
                try {
                    base.evaluate();
                } finally {
                    if (isTrackerDirty()) HeadTrackingUtils.revertTracker();
                }
            }
        }, desc);
    }

    @Override
    public SupportedActivity getRestriction() {
        return SupportedActivity.WAA;
    }

    @Override
    public boolean isTrackerDirty() {
        return mTrackerDirty;
    }

    @Override
    public void setTrackerDirty() {
        mTrackerDirty = true;
    }
}
