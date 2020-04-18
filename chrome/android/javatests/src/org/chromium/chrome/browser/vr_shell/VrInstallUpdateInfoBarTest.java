// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.vr_shell;

import android.os.Build;
import android.support.test.filters.MediumTest;
import android.view.View;
import android.widget.TextView;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.RuleChain;
import org.junit.runner.RunWith;

import org.chromium.base.test.params.ParameterAnnotations.ClassParameter;
import org.chromium.base.test.params.ParameterAnnotations.UseRunnerDelegate;
import org.chromium.base.test.params.ParameterSet;
import org.chromium.base.test.params.ParameterizedRunner;
import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.MinAndroidSdkLevel;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ChromeSwitches;
import org.chromium.chrome.browser.vr_shell.rules.VrActivityRestriction;
import org.chromium.chrome.browser.vr_shell.util.VrInfoBarUtils;
import org.chromium.chrome.browser.vr_shell.util.VrShellDelegateUtils;
import org.chromium.chrome.browser.vr_shell.util.VrTestRuleUtils;
import org.chromium.chrome.test.ChromeActivityTestRule;
import org.chromium.chrome.test.ChromeJUnit4RunnerDelegate;

import java.util.List;
import java.util.concurrent.Callable;

/**
 * End-to-end tests for the InfoBar that prompts the user to update or install
 * VrCore (VR Services) when attempting to use a VR feature with an outdated
 * or entirely missing version.
 */
@RunWith(ParameterizedRunner.class)
@UseRunnerDelegate(ChromeJUnit4RunnerDelegate.class)
@CommandLineFlags.Add({ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE})
@MinAndroidSdkLevel(Build.VERSION_CODES.KITKAT) // WebVR is only supported on K+
public class VrInstallUpdateInfoBarTest {
    @ClassParameter
    private static List<ParameterSet> sClassParams =
            VrTestRuleUtils.generateDefaultVrTestRuleParameters();
    @Rule
    public RuleChain mRuleChain;

    private ChromeActivityTestRule mVrTestRule;
    private VrTestFramework mVrTestFramework;

    public VrInstallUpdateInfoBarTest(Callable<ChromeActivityTestRule> callable) throws Exception {
        mVrTestRule = callable.call();
        mRuleChain = VrTestRuleUtils.wrapRuleInVrActivityRestrictionRule(mVrTestRule);
    }

    @Before
    public void setUp() throws Exception {
        mVrTestFramework = new VrTestFramework(mVrTestRule);
    }

    /**
     * Helper function to run the tests checking for the upgrade/install InfoBar being present since
     * all that differs is the value returned by VrCoreVersionChecker and a couple asserts.
     *
     * @param checkerReturnCompatibility The compatibility to have the VrCoreVersionChecker return
     */
    private void infoBarTestHelper(final int checkerReturnCompatibility) {
        VrShellDelegateUtils.setVrCoreCompatibility(checkerReturnCompatibility);
        View decorView = mVrTestRule.getActivity().getWindow().getDecorView();
        if (checkerReturnCompatibility == VrCoreCompatibility.VR_READY) {
            VrInfoBarUtils.expectInfoBarPresent(mVrTestFramework, false);
        } else if (checkerReturnCompatibility == VrCoreCompatibility.VR_OUT_OF_DATE
                || checkerReturnCompatibility == VrCoreCompatibility.VR_NOT_AVAILABLE) {
            // Out of date and missing cases are the same, but with different text
            String expectedMessage, expectedButton;
            if (checkerReturnCompatibility == VrCoreCompatibility.VR_OUT_OF_DATE) {
                expectedMessage = mVrTestRule.getActivity().getString(
                        R.string.vr_services_check_infobar_update_text);
                expectedButton = mVrTestRule.getActivity().getString(
                        R.string.vr_services_check_infobar_update_button);
            } else {
                expectedMessage = mVrTestRule.getActivity().getString(
                        R.string.vr_services_check_infobar_install_text);
                expectedButton = mVrTestRule.getActivity().getString(
                        R.string.vr_services_check_infobar_install_button);
            }
            VrInfoBarUtils.expectInfoBarPresent(mVrTestFramework, true);
            TextView tempView = (TextView) decorView.findViewById(R.id.infobar_message);
            Assert.assertEquals(expectedMessage, tempView.getText().toString());
            tempView = (TextView) decorView.findViewById(R.id.button_primary);
            Assert.assertEquals(expectedButton, tempView.getText().toString());
        } else if (checkerReturnCompatibility == VrCoreCompatibility.VR_NOT_SUPPORTED) {
            VrInfoBarUtils.expectInfoBarPresent(mVrTestFramework, false);
        } else {
            Assert.fail("Invalid VrCoreVersionChecker compatibility: "
                    + String.valueOf(checkerReturnCompatibility));
        }
        VrShellDelegateUtils.getDelegateInstance().overrideVrCoreVersionCheckerForTesting(null);
    }

    /**
     * Tests that the upgrade/install VR Services InfoBar is not present when VR Services is
     * installed and up to date.
     */
    @Test
    @MediumTest
    @VrActivityRestriction({VrActivityRestriction.SupportedActivity.ALL})
    public void testInfoBarNotPresentWhenVrServicesCurrent() throws InterruptedException {
        infoBarTestHelper(VrCoreCompatibility.VR_READY);
    }

    /**
     * Tests that the upgrade VR Services InfoBar is present when VR Services is outdated.
     */
    @Test
    @MediumTest
    @VrActivityRestriction({VrActivityRestriction.SupportedActivity.ALL})
    public void testInfoBarPresentWhenVrServicesOutdated() throws InterruptedException {
        infoBarTestHelper(VrCoreCompatibility.VR_OUT_OF_DATE);
    }

    /**
     * Tests that the install VR Services InfoBar is present when VR Services is missing.
     */
    @Test
    @MediumTest
    @VrActivityRestriction({VrActivityRestriction.SupportedActivity.ALL})
    public void testInfoBarPresentWhenVrServicesMissing() throws InterruptedException {
        infoBarTestHelper(VrCoreCompatibility.VR_NOT_AVAILABLE);
    }

    /**
     * Tests that the install VR Services InfoBar is not present when VR is not supported on the
     * device.
     */
    @Test
    @MediumTest
    @VrActivityRestriction({VrActivityRestriction.SupportedActivity.ALL})
    public void testInfoBarNotPresentWhenVrServicesNotSupported() throws InterruptedException {
        infoBarTestHelper(VrCoreCompatibility.VR_NOT_SUPPORTED);
    }
}
