// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.vr_shell;

import static org.chromium.chrome.browser.vr_shell.TestFramework.POLL_TIMEOUT_LONG_MS;
import static org.chromium.chrome.browser.vr_shell.VrTestFramework.PAGE_LOAD_TIMEOUT_S;
import static org.chromium.chrome.browser.vr_shell.VrTestFramework.POLL_CHECK_INTERVAL_SHORT_MS;
import static org.chromium.chrome.browser.vr_shell.VrTestFramework.POLL_TIMEOUT_LONG_MS;
import static org.chromium.chrome.browser.vr_shell.VrTestFramework.POLL_TIMEOUT_SHORT_MS;
import static org.chromium.chrome.test.util.ChromeRestriction.RESTRICTION_TYPE_DEVICE_NON_DAYDREAM;
import static org.chromium.chrome.test.util.ChromeRestriction.RESTRICTION_TYPE_VIEWER_DAYDREAM;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.support.test.InstrumentationRegistry;
import android.support.test.filters.LargeTest;
import android.support.test.filters.MediumTest;
import android.widget.ImageView;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.ApplicationStatus;
import org.chromium.base.ThreadUtils;
import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.DisabledTest;
import org.chromium.base.test.util.Restriction;
import org.chromium.base.test.util.RetryOnFailure;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ChromeSwitches;
import org.chromium.chrome.browser.ChromeTabbedActivity;
import org.chromium.chrome.browser.preferences.Preferences;
import org.chromium.chrome.browser.preferences.PreferencesLauncher;
import org.chromium.chrome.browser.preferences.website.SingleWebsitePreferences;
import org.chromium.chrome.browser.vr_shell.mock.MockVrDaydreamApi;
import org.chromium.chrome.browser.vr_shell.rules.ChromeTabbedActivityVrTestRule;
import org.chromium.chrome.browser.vr_shell.util.NfcSimUtils;
import org.chromium.chrome.browser.vr_shell.util.TransitionUtils;
import org.chromium.chrome.browser.vr_shell.util.VrShellDelegateUtils;
import org.chromium.chrome.browser.vr_shell.util.VrTransitionUtils;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.chrome.test.util.ActivityUtils;
import org.chromium.content.browser.test.util.Criteria;
import org.chromium.content.browser.test.util.CriteriaHelper;
import org.chromium.content.browser.test.util.DOMUtils;
import org.chromium.content.browser.test.util.JavaScriptUtils;

import java.lang.ref.WeakReference;
import java.util.List;
import java.util.concurrent.TimeoutException;
import java.util.concurrent.atomic.AtomicReference;

/**
 * End-to-end tests for state transitions in VR, e.g. exiting WebVR presentation
 * into the VR browser.
 */
@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags.Add({ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE})
public class VrShellTransitionTest {
    // We explicitly instantiate a rule here instead of using parameterization since this class
    // only ever runs in ChromeTabbedActivity.
    @Rule
    public ChromeTabbedActivityVrTestRule mTestRule = new ChromeTabbedActivityVrTestRule();

    private VrTestFramework mVrTestFramework;
    private XrTestFramework mXrTestFramework;

    @Before
    public void setUp() throws Exception {
        mVrTestFramework = new VrTestFramework(mTestRule);
        mXrTestFramework = new XrTestFramework(mTestRule);
    }

    private void enterVrShellNfc(boolean supported) {
        InstrumentationRegistry.getInstrumentation().waitForIdleSync();
        if (supported) {
            NfcSimUtils.simNfcScanUntilVrEntry(mTestRule.getActivity());
            Assert.assertTrue(VrShellDelegate.isInVr());
        } else {
            NfcSimUtils.simNfcScan(mTestRule.getActivity());
            Assert.assertFalse(VrShellDelegate.isInVr());
        }
        TransitionUtils.forceExitVr();
    }

    private void enterExitVrShell(boolean supported) {
        MockVrDaydreamApi mockApi = new MockVrDaydreamApi();
        if (!supported) {
            VrShellDelegateUtils.getDelegateInstance().overrideDaydreamApiForTesting(mockApi);
        }
        TransitionUtils.forceEnterVr();
        if (supported) {
            TransitionUtils.waitForVrEntry(POLL_TIMEOUT_LONG_MS);
            Assert.assertTrue(VrShellDelegate.isInVr());
        } else {
            Assert.assertFalse(mockApi.getLaunchInVrCalled());
            Assert.assertFalse(VrShellDelegate.isInVr());
        }
        TransitionUtils.forceExitVr();
        Assert.assertFalse(VrShellDelegate.isInVr());
    }

    /**
     * Verifies that browser successfully transitions from 2D Chrome to the VR
     * browser when the Daydream View NFC tag is scanned on a Daydream-ready device.
     */
    @Test
    @Restriction({RESTRICTION_TYPE_VIEWER_DAYDREAM})
    @RetryOnFailure(message = "crbug.com/736527")
    @LargeTest
    public void test2dtoVrShellNfcSupported() {
        enterVrShellNfc(true /* supported */);
    }

    /**
     * Verifies that the browser does not transition from 2D chrome to the VR
     * browser when the Daydream View NFC tag is scanned on a non-Daydream-ready
     * device.
     */
    @Test
    @Restriction(RESTRICTION_TYPE_DEVICE_NON_DAYDREAM)
    @MediumTest
    public void test2dtoVrShellNfcUnsupported() {
        enterVrShellNfc(false /* supported */);
    }

    /**
     * Verifies that browser successfully transitions from 2D chrome to the VR
     * browser and back when the VrShellDelegate tells it to on a Daydream-ready
     * device.
     */
    @Test
    @Restriction(RESTRICTION_TYPE_VIEWER_DAYDREAM)
    @MediumTest
    public void test2dtoVrShellto2dSupported() {
        enterExitVrShell(true /* supported */);
    }

    /**
     * Verifies that browser successfully transitions from 2D chrome to the VR
     * browser when Chrome gets a VR intent.
     */
    @Test
    @Restriction(RESTRICTION_TYPE_VIEWER_DAYDREAM)
    @MediumTest
    public void testVrIntentStartsVrShell() {
        // Send a VR intent, which will open the link in a CTA.
        String url = VrTestFramework.getFileUrlForHtmlTestFile("test_navigation_2d_page");
        VrTransitionUtils.sendVrLaunchIntent(
                url, mTestRule.getActivity(), false /* autopresent */, true /* avoidRelaunch */);

        // Wait until a CTA is opened due to the intent
        final AtomicReference<ChromeTabbedActivity> cta =
                new AtomicReference<ChromeTabbedActivity>();
        CriteriaHelper.pollUiThread(new Criteria() {
            @Override
            public boolean isSatisfied() {
                List<WeakReference<Activity>> list = ApplicationStatus.getRunningActivities();
                for (WeakReference<Activity> ref : list) {
                    Activity activity = ref.get();
                    if (activity == null) continue;
                    if (activity instanceof ChromeTabbedActivity) {
                        cta.set((ChromeTabbedActivity) activity);
                        return true;
                    }
                }
                return false;
            }
        }, POLL_TIMEOUT_LONG_MS, POLL_CHECK_INTERVAL_SHORT_MS);

        // Wait until the tab is ready
        CriteriaHelper.pollUiThread(new Criteria() {
            @Override
            public boolean isSatisfied() {
                if (cta.get().getActivityTab() == null) return false;
                return !cta.get().getActivityTab().isLoading();
            }
        }, POLL_TIMEOUT_LONG_MS, POLL_CHECK_INTERVAL_SHORT_MS);

        VrTransitionUtils.waitForVrEntry(POLL_TIMEOUT_LONG_MS);
        Assert.assertTrue(VrShellDelegate.isInVr());
        Assert.assertEquals("Url correct", url, mTestRule.getWebContents().getVisibleUrl());
    }

    /**
     * Verifies that browser does not enter VR mode on Non-Daydream-ready devices.
     */
    @Test
    @Restriction(RESTRICTION_TYPE_DEVICE_NON_DAYDREAM)
    @MediumTest
    public void test2dtoVrShellto2dUnsupported() {
        enterExitVrShell(false /* supported */);
    }

    /**
     * Tests that we exit fullscreen mode after exiting VR from cinema mode.
     */
    @Test
    @Restriction(RESTRICTION_TYPE_VIEWER_DAYDREAM)
    @MediumTest
    public void testExitFullscreenAfterExitingVrFromCinemaMode()
            throws InterruptedException, TimeoutException {
        VrTransitionUtils.forceEnterVr();
        VrTransitionUtils.waitForVrEntry(POLL_TIMEOUT_LONG_MS);
        mVrTestFramework.loadUrlAndAwaitInitialization(
                VrTestFramework.getFileUrlForHtmlTestFile("test_navigation_2d_page"),
                PAGE_LOAD_TIMEOUT_S);
        DOMUtils.clickNode(mVrTestFramework.getFirstTabWebContents(), "fullscreen",
                false /* goThroughRootAndroidView */);
        VrTestFramework.waitOnJavaScriptStep(mVrTestFramework.getFirstTabWebContents());

        Assert.assertTrue(DOMUtils.isFullscreen(mVrTestFramework.getFirstTabWebContents()));
        VrTransitionUtils.forceExitVr();
        // The fullscreen exit from exiting VR isn't necessarily instantaneous, so give it
        // a bit of time.
        CriteriaHelper.pollInstrumentationThread(new Criteria() {
            @Override
            public boolean isSatisfied() {
                try {
                    return !DOMUtils.isFullscreen(mVrTestFramework.getFirstTabWebContents());
                } catch (InterruptedException | TimeoutException e) {
                    return false;
                }
            }
        }, POLL_TIMEOUT_SHORT_MS, POLL_CHECK_INTERVAL_SHORT_MS);
    }

    /**
     * Tests that the reported display dimensions are correct when exiting
     * from WebVR presentation to the VR browser.
     */
    @Test
    @CommandLineFlags.Add("enable-webvr")
    @Restriction(RESTRICTION_TYPE_VIEWER_DAYDREAM)
    @MediumTest
    public void testExitPresentationWebVrToVrShell()
            throws IllegalArgumentException, InterruptedException, TimeoutException {
        exitPresentationToVrShellImpl(
                VrTestFramework.getFileUrlForHtmlTestFile("test_navigation_webvr_page"),
                mVrTestFramework, "vrDisplay.exitPresent();");
    }

    /**
     * Tests that the reported display dimensions are correct when exiting
     * from WebVR presentation to the VR browser.
     */
    @Test
    @CommandLineFlags.Add("enable-features=WebXR")
    @Restriction(RESTRICTION_TYPE_VIEWER_DAYDREAM)
    @MediumTest
    public void testExitPresentationWebXrToVrShell()
            throws IllegalArgumentException, InterruptedException, TimeoutException {
        exitPresentationToVrShellImpl(
                XrTestFramework.getFileUrlForHtmlTestFile("test_navigation_webxr_page"),
                mXrTestFramework, "exclusiveSession.end();");
    }

    private void exitPresentationToVrShellImpl(String url, TestFramework framework,
            String exitPresentString) throws InterruptedException {
        TransitionUtils.forceEnterVr();
        TransitionUtils.waitForVrEntry(POLL_TIMEOUT_LONG_MS);
        framework.loadUrlAndAwaitInitialization(url, PAGE_LOAD_TIMEOUT_S);
        VrShellImpl vrShellImpl = (VrShellImpl) TestVrShellDelegate.getVrShellForTesting();
        float expectedWidth = vrShellImpl.getContentWidthForTesting();
        float expectedHeight = vrShellImpl.getContentHeightForTesting();
        TransitionUtils.enterPresentationOrFail(framework);

        // Validate our size is what we expect while in VR.
        // We aren't comparing for equality because there is some rounding that occurs.
        String javascript = "Math.abs(screen.width - " + expectedWidth + ") <= 1 && "
                + "Math.abs(screen.height - " + expectedHeight + ") <= 1";
        Assert.assertTrue(TestFramework.pollJavaScriptBoolean(
                javascript, POLL_TIMEOUT_LONG_MS, framework.getFirstTabWebContents()));

        // Exit presentation through JavaScript.
        TestFramework.runJavaScriptOrFail(
                exitPresentString, POLL_TIMEOUT_SHORT_MS, framework.getFirstTabWebContents());

        Assert.assertTrue(TestFramework.pollJavaScriptBoolean(
                javascript, POLL_TIMEOUT_LONG_MS, framework.getFirstTabWebContents()));
    }

    /**
     * Tests that entering WebVR presentation from the VR browser, exiting presentation, and
     * re-entering presentation works. This is a regression test for crbug.com/799999.
     */
    @Test
    @CommandLineFlags.Add("enable-webvr")
    @Restriction(RESTRICTION_TYPE_VIEWER_DAYDREAM)
    @MediumTest
    public void testWebVrReEntryFromVrBrowser() throws InterruptedException, TimeoutException {
        reEntryFromVrBrowserImpl(
                VrTestFramework.getFileUrlForHtmlTestFile("test_webvr_reentry_from_vr_browser"),
                mVrTestFramework);
    }

    /**
     * Tests that entering WebVR presentation from the VR browser, exiting presentation, and
     * re-entering presentation works. This is a regression test for crbug.com/799999.
     */
    @Test
    @CommandLineFlags.Add("enable-features=WebXR")
    @Restriction(RESTRICTION_TYPE_VIEWER_DAYDREAM)
    @MediumTest
    public void testWebXrReEntryFromVrBrowser() throws InterruptedException, TimeoutException {
        reEntryFromVrBrowserImpl(
                XrTestFramework.getFileUrlForHtmlTestFile("test_webxr_reentry_from_vr_browser"),
                mXrTestFramework);
    }

    private void reEntryFromVrBrowserImpl(String url, TestFramework framework)
            throws InterruptedException {
        TransitionUtils.forceEnterVr();
        TransitionUtils.waitForVrEntry(POLL_TIMEOUT_LONG_MS);
        EmulatedVrController controller = new EmulatedVrController(mTestRule.getActivity());

        framework.loadUrlAndAwaitInitialization(url, PAGE_LOAD_TIMEOUT_S);
        TransitionUtils.enterPresentationOrFail(framework);

        TestFramework.executeStepAndWait(
                "stepVerifyFirstPresent()", framework.getFirstTabWebContents());
        // The bug did not reproduce with vrDisplay.exitPresent(), so it might not reproduce with
        // session.end(). Instead, use the controller to exit.
        controller.pressReleaseAppButton();
        TestFramework.executeStepAndWait(
                "stepVerifyMagicWindow()", framework.getFirstTabWebContents());

        TransitionUtils.enterPresentationOrFail(framework);
        TestFramework.executeStepAndWait(
                "stepVerifySecondPresent()", framework.getFirstTabWebContents());

        TestFramework.endTest(framework.getFirstTabWebContents());
    }

    /**
     * Tests that you can enter VR in Overview mode, and the NTP shows up.
     */
    @Test
    @Restriction(RESTRICTION_TYPE_VIEWER_DAYDREAM)
    @MediumTest
    public void testEnterVrInOverviewMode() throws InterruptedException, TimeoutException {
        final ChromeTabbedActivity activity = mTestRule.getActivity();
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                ImageView tabSwitcher = (ImageView) activity.findViewById(R.id.tab_switcher_button);
                tabSwitcher.callOnClick();
            }
        });

        Assert.assertTrue(activity.isInOverviewMode());

        MockVrDaydreamApi mockApi = new MockVrDaydreamApi();
        VrShellDelegateUtils.getDelegateInstance().overrideDaydreamApiForTesting(mockApi);
        Assert.assertTrue(VrTransitionUtils.forceEnterVr());
        VrTransitionUtils.waitForVrEntry(POLL_TIMEOUT_LONG_MS);
        Assert.assertTrue(VrShellDelegateUtils.getDelegateInstance().isVrEntryComplete());
        Assert.assertFalse(mockApi.getExitFromVrCalled());
        Assert.assertFalse(mockApi.getLaunchVrHomescreenCalled());
        VrShellDelegateUtils.getDelegateInstance().overrideDaydreamApiForTesting(null);
    }

    /**
     * Tests that attempting to start an Activity through the Activity context in VR triggers DOFF.
     */
    @Test
    @DisabledTest(message = "https://crbug.com/831589")
    @Restriction(RESTRICTION_TYPE_VIEWER_DAYDREAM)
    @MediumTest
    public void testStartActivityTriggersDoffChromeActivity()
            throws InterruptedException, TimeoutException {
        testStartActivityTriggersDoffImpl(mTestRule.getActivity());
    }

    /**
     * Tests that attempting to start an Activity through the Application context in VR triggers
     * DOFF.
     */
    @Test
    @DisabledTest(message = "https://crbug.com/831589")
    @Restriction(RESTRICTION_TYPE_VIEWER_DAYDREAM)
    @MediumTest
    public void testStartActivityTriggersDoffAppContext()
            throws InterruptedException, TimeoutException {
        testStartActivityTriggersDoffImpl(mTestRule.getActivity().getApplicationContext());
    }

    private void testStartActivityTriggersDoffImpl(Context context)
            throws InterruptedException, TimeoutException {
        Assert.assertTrue(TransitionUtils.forceEnterVr());
        TransitionUtils.waitForVrEntry(POLL_TIMEOUT_LONG_MS);
        Assert.assertTrue(VrShellDelegateUtils.getDelegateInstance().isVrEntryComplete());

        MockVrDaydreamApi mockApi = new MockVrDaydreamApi();
        mockApi.setExitFromVrReturnValue(false);
        VrShellDelegateUtils.getDelegateInstance().overrideDaydreamApiForTesting(mockApi);
        ThreadUtils.runOnUiThreadBlocking(() -> {
            Intent preferencesIntent = PreferencesLauncher.createIntentForSettingsPage(
                    context, SingleWebsitePreferences.class.getName());
            context.startActivity(preferencesIntent);
            VrShellDelegateUtils.getDelegateInstance().acceptDoffPromptForTesting();
        });

        CriteriaHelper.pollUiThread(() -> { return mockApi.getExitFromVrCalled(); });
        Assert.assertFalse(mockApi.getLaunchVrHomescreenCalled());
        mockApi.close();

        MockVrDaydreamApi mockApiWithDoff = new MockVrDaydreamApi();
        mockApiWithDoff.setExitFromVrReturnValue(true);

        VrShellDelegateUtils.getDelegateInstance().overrideDaydreamApiForTesting(mockApiWithDoff);

        ThreadUtils.runOnUiThreadBlocking(() -> {
            PreferencesLauncher.launchSettingsPage(context, null);
            VrShellDelegateUtils.getDelegateInstance().acceptDoffPromptForTesting();
        });
        CriteriaHelper.pollUiThread(
                () -> { return VrShellDelegateUtils.getDelegateInstance().isShowingDoff(); });
        ThreadUtils.runOnUiThreadBlocking(() -> {
            mTestRule.getActivity().onActivityResult(
                    VrShellDelegate.EXIT_VR_RESULT, Activity.RESULT_OK, null);
        });

        ActivityUtils.waitForActivity(
                InstrumentationRegistry.getInstrumentation(), Preferences.class);
        VrShellDelegateUtils.getDelegateInstance().overrideDaydreamApiForTesting(null);
    }

    /**
     * Tests that attempting to start an Activity through the Activity context in VR triggers DOFF.
     */
    @Test
    @Restriction(RESTRICTION_TYPE_VIEWER_DAYDREAM)
    @MediumTest
    public void testStartActivityIfNeeded() throws InterruptedException, TimeoutException {
        Activity context = mTestRule.getActivity();
        Assert.assertTrue(TransitionUtils.forceEnterVr());
        TransitionUtils.waitForVrEntry(POLL_TIMEOUT_LONG_MS);
        Assert.assertTrue(VrShellDelegateUtils.getDelegateInstance().isVrEntryComplete());

        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                Intent preferencesIntent = PreferencesLauncher.createIntentForSettingsPage(
                        context, SingleWebsitePreferences.class.getName());
                Assert.assertFalse(context.startActivityIfNeeded(preferencesIntent, 0));
            }
        });
    }

    /**
     * Tests that exiting VR while a permission prompt or JavaScript dialog is being displayed
     * does not cause a browser crash. Regression test for https://crbug.com/821443.
     */
    @Test
    @Restriction(RESTRICTION_TYPE_VIEWER_DAYDREAM)
    @LargeTest
    @CommandLineFlags.Add("enable-features=VrBrowsingNativeAndroidUi")
    public void testExitVrWithPromptDisplayed() throws InterruptedException, TimeoutException {
        mVrTestFramework.loadUrlAndAwaitInitialization(
                VrTestFramework.getFileUrlForHtmlTestFile("test_navigation_2d_page"),
                PAGE_LOAD_TIMEOUT_S);

        // Test JavaScript dialogs.
        Assert.assertTrue(TransitionUtils.forceEnterVr());
        TransitionUtils.waitForVrEntry(POLL_TIMEOUT_LONG_MS);
        // Alerts block JavaScript execution until they're closed, so we can't use the normal
        // runJavaScriptOrFail, as that will time out.
        JavaScriptUtils.executeJavaScript(mVrTestFramework.getFirstTabWebContents(),
                "alert('Please no crash')");
        TransitionUtils.waitForNativeUiPrompt(POLL_TIMEOUT_LONG_MS);
        TransitionUtils.forceExitVr();

        // Test permission prompts.
        Assert.assertTrue(TransitionUtils.forceEnterVr());
        TransitionUtils.waitForVrEntry(POLL_TIMEOUT_LONG_MS);
        VrTestFramework.runJavaScriptOrFail("navigator.getUserMedia({video: true}, ()=>{}, ()=>{})",
                POLL_TIMEOUT_SHORT_MS, mVrTestFramework.getFirstTabWebContents());
        TransitionUtils.waitForNativeUiPrompt(POLL_TIMEOUT_LONG_MS);
        TransitionUtils.forceExitVr();
    }
}
