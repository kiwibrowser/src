// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.vr_shell;

import static org.chromium.chrome.browser.vr_shell.VrTestFramework.PAGE_LOAD_TIMEOUT_S;
import static org.chromium.chrome.browser.vr_shell.VrTestFramework.POLL_CHECK_INTERVAL_SHORT_MS;
import static org.chromium.chrome.browser.vr_shell.VrTestFramework.POLL_TIMEOUT_LONG_MS;
import static org.chromium.chrome.test.util.ChromeRestriction.RESTRICTION_TYPE_DEVICE_DAYDREAM;
import static org.chromium.chrome.test.util.ChromeRestriction.RESTRICTION_TYPE_VIEWER_DAYDREAM;

import android.support.test.filters.MediumTest;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.Restriction;
import org.chromium.chrome.browser.ChromeSwitches;
import org.chromium.chrome.browser.vr_shell.keyboard.TextEditAction;
import org.chromium.chrome.browser.vr_shell.mock.MockBrowserKeyboardInterface;
import org.chromium.chrome.browser.vr_shell.rules.ChromeTabbedActivityVrTestRule;
import org.chromium.chrome.browser.vr_shell.util.VrTransitionUtils;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.content.browser.test.util.Criteria;
import org.chromium.content.browser.test.util.CriteriaHelper;

/**
 * End-to-end tests for Daydream controller input while in the VR browser.
 */
@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags.Add({ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE})
@Restriction({RESTRICTION_TYPE_DEVICE_DAYDREAM, RESTRICTION_TYPE_VIEWER_DAYDREAM})
public class VrShellWebInputEditingTest {
    // We explicitly instantiate a rule here instead of using parameterization since this class
    // only ever runs in ChromeTabbedActivity.
    @Rule
    public ChromeTabbedActivityVrTestRule mVrTestRule = new ChromeTabbedActivityVrTestRule();

    private VrTestFramework mVrTestFramework;
    private EmulatedVrController mController;

    @Before
    public void setUp() throws Exception {
        mVrTestFramework = new VrTestFramework(mVrTestRule);
        mController = new EmulatedVrController(mVrTestRule.getActivity());
    }

    /**
     * Verifies that when a web input field is focused, the VrInputMethodManagerWrapper is asked to
     * spawn the keyboard. Moreover, we verify that an edit sent to web contents via the
     * VrInputConnection updates indices on the VrInputMethodManagerWrapper.
     */
    @Test
    @MediumTest
    @CommandLineFlags.Add("enable-features=VrLaunchIntents")
    public void testWebInputFocus() throws InterruptedException {
        // Load page in VR and make sure the controller is pointed at the content quad.
        mVrTestRule.loadUrl(VrTestFramework.getFileUrlForHtmlTestFile("test_web_input_editing"),
                PAGE_LOAD_TIMEOUT_S);
        VrTransitionUtils.forceEnterVr();
        VrTransitionUtils.waitForVrEntry(POLL_TIMEOUT_LONG_MS);
        mController.recenterView();

        VrShellImpl vrShellImpl = (VrShellImpl) TestVrShellDelegate.getVrShellForTesting();
        MockBrowserKeyboardInterface keyboard = new MockBrowserKeyboardInterface();
        vrShellImpl.getInputMethodManageWrapperForTesting().setBrowserKeyboardInterfaceForTesting(
                keyboard);

        // The webpage reacts to the first controller click by focusing its input field. Verify that
        // focus gain spawns the keyboard.
        mController.performControllerClick();
        CriteriaHelper.pollInstrumentationThread(new Criteria() {
            @Override
            public boolean isSatisfied() {
                Boolean visible = keyboard.getLastKeyboardVisibility();
                return visible != null && visible;
            }
        }, POLL_TIMEOUT_LONG_MS, POLL_CHECK_INTERVAL_SHORT_MS);

        // Add text to the input field via the input connection and verify that the keyboard
        // interface is called to update the indices.
        VrInputConnection ic = vrShellImpl.getVrInputConnectionForTesting();
        TextEditAction[] edits = {new TextEditAction(TextEditActionType.COMMIT_TEXT, "i", 1)};
        ic.onKeyboardEdit(edits);
        // Inserting 'i' should move the cursor by one character and there should be no composition.
        MockBrowserKeyboardInterface.Indices expectedIndices =
                new MockBrowserKeyboardInterface.Indices(1, 1, -1, -1);
        CriteriaHelper.pollInstrumentationThread(new Criteria() {
            @Override
            public boolean isSatisfied() {
                MockBrowserKeyboardInterface.Indices indices = keyboard.getLastIndices();
                return indices.equals(expectedIndices);
            }
        }, POLL_TIMEOUT_LONG_MS, POLL_CHECK_INTERVAL_SHORT_MS);

        // The second click should result in a focus loss and should hide the keyboard.
        mController.performControllerClick();
        CriteriaHelper.pollInstrumentationThread(new Criteria() {
            @Override
            public boolean isSatisfied() {
                Boolean visible = keyboard.getLastKeyboardVisibility();
                return visible != null && !visible;
            }
        }, POLL_TIMEOUT_LONG_MS, POLL_CHECK_INTERVAL_SHORT_MS);
    }
}
