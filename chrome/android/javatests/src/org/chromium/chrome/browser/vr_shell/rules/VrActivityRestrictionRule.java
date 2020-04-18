// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.vr_shell.rules;

import org.junit.Assume;
import org.junit.rules.TestRule;
import org.junit.runner.Description;
import org.junit.runners.model.Statement;

import org.chromium.chrome.browser.vr_shell.rules.VrActivityRestriction.SupportedActivity;
import org.chromium.chrome.browser.vr_shell.util.VrTestRuleUtils;

/**
 * Rule that conditionally skips a test if the current VrTestRule's Activity is not
 * one of the supported Activity types for the test.
 */
public class VrActivityRestrictionRule implements TestRule {
    private SupportedActivity mCurrentRestriction;

    public VrActivityRestrictionRule(SupportedActivity currentRestriction) {
        mCurrentRestriction = currentRestriction;
    }

    @Override
    public Statement apply(final Statement base, final Description desc) {
        // Check if the test has a VrActivityRestriction annotation
        VrActivityRestriction annotation = desc.getAnnotation(VrActivityRestriction.class);
        if (annotation == null) {
            if (mCurrentRestriction == SupportedActivity.CTA) {
                // Default to running in ChromeTabbedActivity if no restriction annotation
                return base;
            }
            return generateIgnoreStatement();
        }

        SupportedActivity[] activities = annotation.value();
        for (int i = 0; i < activities.length; i++) {
            if (activities[i] == mCurrentRestriction || activities[i] == SupportedActivity.ALL) {
                return base;
            }
        }
        return generateIgnoreStatement();
    }

    private Statement generateIgnoreStatement() {
        return new Statement() {
            @Override
            public void evaluate() {
                Assume.assumeTrue("Test ignored because "
                                + VrTestRuleUtils.supportedActivityToString(mCurrentRestriction)
                                + " was not one of the specified activities to run the test in.",
                        false);
            }
        };
    }
}
