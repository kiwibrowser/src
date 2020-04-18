// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.vr_shell;

import static org.chromium.chrome.browser.vr_shell.TestFramework.PAGE_LOAD_TIMEOUT_S;
import static org.chromium.chrome.browser.vr_shell.VrTestFramework.PAGE_LOAD_TIMEOUT_S;
import static org.chromium.chrome.browser.vr_shell.VrTestFramework.POLL_CHECK_INTERVAL_LONG_MS;
import static org.chromium.chrome.browser.vr_shell.VrTestFramework.POLL_CHECK_INTERVAL_SHORT_MS;
import static org.chromium.chrome.browser.vr_shell.VrTestFramework.POLL_TIMEOUT_LONG_MS;
import static org.chromium.chrome.browser.vr_shell.VrTestFramework.POLL_TIMEOUT_SHORT_MS;
import static org.chromium.chrome.test.util.ChromeRestriction.RESTRICTION_TYPE_DON_ENABLED;
import static org.chromium.chrome.test.util.ChromeRestriction.RESTRICTION_TYPE_VIEWER_DAYDREAM;

import android.annotation.TargetApi;
import android.app.Activity;
import android.graphics.Bitmap;
import android.graphics.Color;
import android.os.Build;
import android.os.SystemClock;
import android.support.test.InstrumentationRegistry;
import android.support.test.filters.LargeTest;
import android.support.test.filters.MediumTest;
import android.support.test.uiautomator.UiDevice;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.RuleChain;
import org.junit.runner.RunWith;

import org.chromium.base.ApplicationStatus;
import org.chromium.base.test.params.ParameterAnnotations.ClassParameter;
import org.chromium.base.test.params.ParameterAnnotations.UseRunnerDelegate;
import org.chromium.base.test.params.ParameterSet;
import org.chromium.base.test.params.ParameterizedRunner;
import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.MinAndroidSdkLevel;
import org.chromium.base.test.util.Restriction;
import org.chromium.base.test.util.RetryOnFailure;
import org.chromium.chrome.browser.ChromeActivity;
import org.chromium.chrome.browser.ChromeSwitches;
import org.chromium.chrome.browser.customtabs.CustomTabActivity;
import org.chromium.chrome.browser.vr_shell.mock.MockVrIntentHandler;
import org.chromium.chrome.browser.vr_shell.rules.VrActivityRestriction;
import org.chromium.chrome.browser.vr_shell.util.NfcSimUtils;
import org.chromium.chrome.browser.vr_shell.util.TransitionUtils;
import org.chromium.chrome.browser.vr_shell.util.VrTestRuleUtils;
import org.chromium.chrome.browser.vr_shell.util.VrTransitionUtils;
import org.chromium.chrome.test.ChromeActivityTestRule;
import org.chromium.chrome.test.ChromeJUnit4RunnerDelegate;
import org.chromium.content.browser.test.util.Criteria;
import org.chromium.content.browser.test.util.CriteriaHelper;
import org.chromium.content_public.browser.WebContents;

import java.lang.ref.WeakReference;
import java.util.List;
import java.util.concurrent.Callable;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;
import java.util.concurrent.atomic.AtomicReference;

/**
 * End-to-end tests for transitioning between WebVR and WebXR's magic window and
 * presentation modes.
 */
@RunWith(ParameterizedRunner.class)
@UseRunnerDelegate(ChromeJUnit4RunnerDelegate.class)
@CommandLineFlags.Add({ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE, "enable-webvr"})
@MinAndroidSdkLevel(Build.VERSION_CODES.KITKAT) // WebVR and WebXR are only supported on K+
@TargetApi(Build.VERSION_CODES.KITKAT) // Necessary to allow taking screenshots with UiAutomation
public class WebVrTransitionTest {
    @ClassParameter
    private static List<ParameterSet> sClassParams =
            VrTestRuleUtils.generateDefaultVrTestRuleParameters();
    @Rule
    public RuleChain mRuleChain;

    private ChromeActivityTestRule mTestRule;
    private VrTestFramework mVrTestFramework;
    private XrTestFramework mXrTestFramework;

    public WebVrTransitionTest(Callable<ChromeActivityTestRule> callable) throws Exception {
        mTestRule = callable.call();
        mRuleChain = VrTestRuleUtils.wrapRuleInVrActivityRestrictionRule(mTestRule);
    }

    @Before
    public void setUp() throws Exception {
        mVrTestFramework = new VrTestFramework(mTestRule);
        mXrTestFramework = new XrTestFramework(mTestRule);
    }

    /**
     * Tests that a successful requestPresent call actually enters VR
     */
    @Test
    @MediumTest
    @VrActivityRestriction({VrActivityRestriction.SupportedActivity.ALL})
    public void testRequestPresentEntersVr() throws InterruptedException {
        testPresentationEntryImpl(
                VrTestFramework.getFileUrlForHtmlTestFile("generic_webvr_page"), mVrTestFramework);
    }

    /**
     * Tests that a successful request for an exclusive session actually enters VR.
     */
    @Test
    @MediumTest
    @CommandLineFlags.Remove({"enable-webvr"})
    @CommandLineFlags.Add({"enable-features=WebXR"})
    @VrActivityRestriction({VrActivityRestriction.SupportedActivity.ALL})
    public void testRequestSessionEntersVr() throws InterruptedException {
        testPresentationEntryImpl(
                XrTestFramework.getFileUrlForHtmlTestFile("generic_webxr_page"), mXrTestFramework);
    }

    private void testPresentationEntryImpl(String url, TestFramework framework)
            throws InterruptedException {
        framework.loadUrlAndAwaitInitialization(url, PAGE_LOAD_TIMEOUT_S);
        TransitionUtils.enterPresentationOrFail(framework);
        Assert.assertTrue("VrShellDelegate is in VR", VrShellDelegate.isInVr());

        // Initial Pixel Test - Verify that the Canvas is blue.
        // The Canvas is set to blue while presenting.
        final UiDevice uiDevice =
                UiDevice.getInstance(InstrumentationRegistry.getInstrumentation());

        CriteriaHelper.pollInstrumentationThread(new Criteria() {
            @Override
            public boolean isSatisfied() {
                Bitmap screenshot = InstrumentationRegistry.getInstrumentation()
                                            .getUiAutomation()
                                            .takeScreenshot();

                if (screenshot != null) {
                    // Calculate center of eye coordinates.
                    int height = uiDevice.getDisplayHeight() / 2;
                    int width = uiDevice.getDisplayWidth() / 4;

                    // Verify screen is blue.
                    int pixel = screenshot.getPixel(width, height);
                    // Workaround for the immersive mode popup sometimes being rendered over the
                    // screen on K, which causes the pure blue to be darkened to (0, 0, 127).
                    // TODO(https://crbug.com/819021): Only check pure blue.
                    return pixel == Color.BLUE || pixel == Color.rgb(0, 0, 127);
                }
                return false;
            }
        }, POLL_TIMEOUT_LONG_MS, POLL_CHECK_INTERVAL_LONG_MS);
    }

    /**
     * Tests that WebVR is not exposed if the flag is not on and the page does
     * not have an origin trial token.
     */
    @Test
    @MediumTest
    @CommandLineFlags.Remove({"enable-webvr"})
    @VrActivityRestriction({VrActivityRestriction.SupportedActivity.ALL})
    public void testWebVrDisabledWithoutFlagSet() throws InterruptedException {
        // TODO(bsheedy): Remove this test once WebVR is on by default without
        // requiring an origin trial.
        apiDisabledWithoutFlagSetImpl(
                VrTestFramework.getFileUrlForHtmlTestFile("test_webvr_disabled_without_flag_set"),
                mVrTestFramework);
    }

    /**
     * Tests that WebXR is not exposed if the flag is not on and the page does
     * not have an origin trial token.
     */
    @Test
    @MediumTest
    @CommandLineFlags.Remove({"enable-webvr"})
    @VrActivityRestriction({VrActivityRestriction.SupportedActivity.ALL})
    public void testWebXrDisabledWithoutFlagSet() throws InterruptedException {
        // TODO(bsheedy): Remove this test once WebXR is on by default without
        // requiring an origin trial.
        apiDisabledWithoutFlagSetImpl(
                XrTestFramework.getFileUrlForHtmlTestFile("test_webxr_disabled_without_flag_set"),
                mXrTestFramework);
    }

    private void apiDisabledWithoutFlagSetImpl(String url, TestFramework framework)
            throws InterruptedException {
        framework.loadUrlAndAwaitInitialization(url, PAGE_LOAD_TIMEOUT_S);
        TestFramework.waitOnJavaScriptStep(framework.getFirstTabWebContents());
        TestFramework.endTest(framework.getFirstTabWebContents());
    }

    /**
     * Tests that scanning the Daydream View NFC tag on supported devices fires the
     * vrdisplayactivate event and the event allows presentation without a user gesture.
     */
    @Test
    @LargeTest
    @Restriction(RESTRICTION_TYPE_VIEWER_DAYDREAM)
    @VrActivityRestriction({VrActivityRestriction.SupportedActivity.ALL})
    public void testNfcFiresVrdisplayactivate() throws InterruptedException {
        mVrTestFramework.loadUrlAndAwaitInitialization(
                VrTestFramework.getFileUrlForHtmlTestFile("test_nfc_fires_vrdisplayactivate"),
                PAGE_LOAD_TIMEOUT_S);
        VrTestFramework.runJavaScriptOrFail(
                "addListener()", POLL_TIMEOUT_LONG_MS, mVrTestFramework.getFirstTabWebContents());
        NfcSimUtils.simNfcScanUntilVrEntry(mTestRule.getActivity());
        VrTestFramework.waitOnJavaScriptStep(mVrTestFramework.getFirstTabWebContents());
        VrTestFramework.endTest(mVrTestFramework.getFirstTabWebContents());
        // VrCore has a 2000 ms debounce timeout on NFC scans. When run multiple times in different
        // activities, it is possible for a latter test to be run in the 2 seconds after the
        // previous test's NFC scan, causing it to fail flakily. So, wait 2 seconds to ensure that
        // can't happen.
        SystemClock.sleep(2000);
    }

    /**
     * Tests that the requestPresent promise doesn't resolve if the DON flow is
     * not completed.
     */
    @Test
    @MediumTest
    @Restriction({RESTRICTION_TYPE_VIEWER_DAYDREAM, RESTRICTION_TYPE_DON_ENABLED})
    @VrActivityRestriction({VrActivityRestriction.SupportedActivity.ALL})
    public void testPresentationPromiseUnresolvedDuringDon() throws InterruptedException {
        mVrTestFramework.loadUrlAndAwaitInitialization(
                VrTestFramework.getFileUrlForHtmlTestFile(
                        "test_presentation_promise_unresolved_during_don"),
                PAGE_LOAD_TIMEOUT_S);
        VrTransitionUtils.enterPresentationAndWait(mVrTestFramework.getFirstTabWebContents());
        VrTestFramework.endTest(mVrTestFramework.getFirstTabWebContents());
    }

    /**
     * Tests that the exclusive session promise doesn't resolve if the DON flow is
     * not completed.
     */
    @Test
    @MediumTest
    @Restriction({RESTRICTION_TYPE_VIEWER_DAYDREAM, RESTRICTION_TYPE_DON_ENABLED})
    @CommandLineFlags.Remove({"enable-webvr"})
    @CommandLineFlags.Add({"enable-features=WebXR"})
    @VrActivityRestriction({VrActivityRestriction.SupportedActivity.ALL})
    public void testPresentationPromiseUnresolvedDuringDon_WebXr()
            throws InterruptedException {
        presentationPromiseUnresolvedDuringDonImpl(
                XrTestFramework.getFileUrlForHtmlTestFile(
                        "webxr_test_presentation_promise_unresolved_during_don"),
                mXrTestFramework);
    }

    private void presentationPromiseUnresolvedDuringDonImpl(String url, TestFramework framework)
            throws InterruptedException {
        framework.loadUrlAndAwaitInitialization(url, PAGE_LOAD_TIMEOUT_S);
        TransitionUtils.enterPresentationAndWait(framework.getFirstTabWebContents());
        TestFramework.endTest(framework.getFirstTabWebContents());
    }

    /**
     * Tests that the requestPresent promise is rejected if the DON flow is canceled.
     */
    @Test
    @MediumTest
    @Restriction({RESTRICTION_TYPE_VIEWER_DAYDREAM, RESTRICTION_TYPE_DON_ENABLED})
    @VrActivityRestriction({VrActivityRestriction.SupportedActivity.ALL})
    public void testPresentationPromiseRejectedIfDonCanceled() throws InterruptedException {
        presentationPromiseRejectedIfDonCanceledImpl(
                VrTestFramework.getFileUrlForHtmlTestFile(
                        "test_presentation_promise_rejected_if_don_canceled"),
                mVrTestFramework);
    }

    /**
     * Tests that the exclusive session promise is rejected if the DON flow is canceled.
     */
    @Test
    @MediumTest
    @Restriction({RESTRICTION_TYPE_VIEWER_DAYDREAM, RESTRICTION_TYPE_DON_ENABLED})
    @CommandLineFlags.Remove({"enable-webvr"})
    @CommandLineFlags.Add({"enable-features=WebXR"})
    @VrActivityRestriction({VrActivityRestriction.SupportedActivity.ALL})
    public void testPresentationPromiseRejectedIfDonCanceled_WebXr()
            throws InterruptedException {
        presentationPromiseRejectedIfDonCanceledImpl(
                XrTestFramework.getFileUrlForHtmlTestFile(
                        "webxr_test_presentation_promise_rejected_if_don_canceled"),
                mXrTestFramework);
    }

    private void presentationPromiseRejectedIfDonCanceledImpl(String url, TestFramework framework)
            throws InterruptedException {
        framework.loadUrlAndAwaitInitialization(url, PAGE_LOAD_TIMEOUT_S);
        final UiDevice uiDevice =
                UiDevice.getInstance(InstrumentationRegistry.getInstrumentation());
        TransitionUtils.enterPresentation(framework.getFirstTabWebContents());
        // Wait until the DON flow appears to be triggered
        // TODO(bsheedy): Make this less hacky if there's ever an explicit way to check if the
        // DON flow is currently active https://crbug.com/758296
        CriteriaHelper.pollUiThread(new Criteria() {
            @Override
            public boolean isSatisfied() {
                return uiDevice.getCurrentPackageName().equals("com.google.vr.vrcore");
            }
        }, POLL_TIMEOUT_LONG_MS, POLL_CHECK_INTERVAL_SHORT_MS);
        uiDevice.pressBack();
        TestFramework.waitOnJavaScriptStep(framework.getFirstTabWebContents());
        TestFramework.endTest(framework.getFirstTabWebContents());
    }

    /**
     * Tests that an intent from a trusted app such as Daydream Home allows WebVR content
     * to auto present without the need for a user gesture.
     */
    @Test
    @MediumTest
    @CommandLineFlags.Add("enable-features=WebVrAutopresentFromIntent")
    @Restriction(RESTRICTION_TYPE_VIEWER_DAYDREAM)
    public void testTrustedIntentAllowsAutoPresent() throws InterruptedException {
        VrIntentUtils.setHandlerInstanceForTesting(new MockVrIntentHandler(
                true /* useMockImplementation */, true /* treatIntentsAsTrusted */));

        // Send an autopresent intent, which will open the link in a CCT
        VrTransitionUtils.sendVrLaunchIntent(
                VrTestFramework.getFileUrlForHtmlTestFile("test_webvr_autopresent"),
                mTestRule.getActivity(), true /* autopresent */, true /* avoidRelaunch */);

        // Wait until a CCT is opened due to the intent
        final AtomicReference<CustomTabActivity> cct = new AtomicReference<CustomTabActivity>();
        CriteriaHelper.pollUiThread(new Criteria() {
            @Override
            public boolean isSatisfied() {
                List<WeakReference<Activity>> list = ApplicationStatus.getRunningActivities();
                for (WeakReference<Activity> ref : list) {
                    Activity activity = ref.get();
                    if (activity == null) continue;
                    if (activity instanceof CustomTabActivity) {
                        cct.set((CustomTabActivity) activity);
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
                if (cct.get().getActivityTab() == null) return false;
                return !cct.get().getActivityTab().isLoading();
            }
        }, POLL_TIMEOUT_LONG_MS, POLL_CHECK_INTERVAL_SHORT_MS);

        WebContents wc = cct.get().getActivityTab().getWebContents();
        VrTestFramework.waitOnJavaScriptStep(wc);
        VrTestFramework.endTest(wc);
    }

    /**
     * Tests that the omnibox reappears after exiting VR.
     */
    @Test
    @MediumTest
    public void testControlsVisibleAfterExitingVr() throws InterruptedException {
        controlsVisibleAfterExitingVrImpl(
                VrTestFramework.getFileUrlForHtmlTestFile("generic_webvr_page"), mVrTestFramework);
    }

    /**
     * Tests that the omnibox reappears after exiting an exclusive session.
     */
    @Test
    @MediumTest
    @CommandLineFlags.Remove({"enable-webvr"})
    @CommandLineFlags.Add({"enable-features=WebXR"})
    public void testControlsVisibleAfterExitingVr_WebXr() throws InterruptedException {
        controlsVisibleAfterExitingVrImpl(
                XrTestFramework.getFileUrlForHtmlTestFile("generic_webxr_page"), mXrTestFramework);
    }

    private void controlsVisibleAfterExitingVrImpl(String url, final TestFramework framework)
            throws InterruptedException {
        framework.loadUrlAndAwaitInitialization(url, PAGE_LOAD_TIMEOUT_S);
        TransitionUtils.enterPresentationOrFail(framework);
        TransitionUtils.forceExitVr();
        // The hiding of the controls may only propagate after VR has exited, so give it a chance
        // to propagate. In the worst case this test will erroneously pass, but should never
        // erroneously fail, and should only be flaky if omnibox showing is broken.
        Thread.sleep(100);
        CriteriaHelper.pollUiThread(new Criteria() {
            @Override
            public boolean isSatisfied() {
                ChromeActivity activity = framework.getRule().getActivity();
                return activity.getFullscreenManager().getBrowserControlHiddenRatio() == 0.0;
            }
        }, POLL_TIMEOUT_SHORT_MS, POLL_CHECK_INTERVAL_SHORT_MS);
    }

    /**
     * Tests that window.requestAnimationFrame stops firing while in WebVR presentation, but resumes
     * afterwards.
     */
    @Test
    @MediumTest
    @VrActivityRestriction({VrActivityRestriction.SupportedActivity.ALL})
    @RetryOnFailure
    public void testWindowRafStopsFiringWhilePresenting() throws InterruptedException {
        windowRafStopsFiringWhilePresentingImpl(
                VrTestFramework.getFileUrlForHtmlTestFile(
                        "test_window_raf_stops_firing_while_presenting"),
                mVrTestFramework);
    }

    /**
     * Tests that window.requestAnimationFrame stops firing while in a WebXR exclusive session, but
     * resumes afterwards.
     */
    @Test
    @MediumTest
    @CommandLineFlags.Remove({"enable-webvr"})
    @CommandLineFlags.Add({"enable-features=WebXR"})
    @VrActivityRestriction({VrActivityRestriction.SupportedActivity.ALL})
    public void testWindowRafStopsFiringWhilePresenting_WebXr()
            throws InterruptedException {
        windowRafStopsFiringWhilePresentingImpl(
                XrTestFramework.getFileUrlForHtmlTestFile(
                        "webxr_test_window_raf_stops_firing_during_exclusive_session"),
                mXrTestFramework);
    }

    private void windowRafStopsFiringWhilePresentingImpl(String url, TestFramework framework)
            throws InterruptedException {
        framework.loadUrlAndAwaitInitialization(url, PAGE_LOAD_TIMEOUT_S);
        TestFramework.executeStepAndWait(
                "stepVerifyBeforePresent()", framework.getFirstTabWebContents());
        // Pausing of window.rAF is done asynchronously, so wait until that's done.
        final CountDownLatch vsyncPausedLatch = new CountDownLatch(1);
        TestVrShellDelegate.getInstance().setVrShellOnVSyncPausedCallback(
                () -> { vsyncPausedLatch.countDown(); });
        TransitionUtils.enterPresentationOrFail(framework);
        vsyncPausedLatch.await(POLL_TIMEOUT_SHORT_MS, TimeUnit.MILLISECONDS);
        TestFramework.executeStepAndWait(
                "stepVerifyDuringPresent()", framework.getFirstTabWebContents());
        TransitionUtils.forceExitVr();
        TestFramework.executeStepAndWait(
                "stepVerifyAfterPresent()", framework.getFirstTabWebContents());
        TestFramework.endTest(framework.getFirstTabWebContents());
    }

    /**
     * Tests renderer crashes while in WebVR presentation stay in VR.
     */
    @Test
    @MediumTest
    @Restriction(RESTRICTION_TYPE_VIEWER_DAYDREAM)
    public void testRendererKilledInWebVrStaysInVr()
            throws IllegalArgumentException, InterruptedException, TimeoutException {
        rendererKilledInVrStaysInVrImpl(
                VrTestFramework.getFileUrlForHtmlTestFile("generic_webvr_page"), mVrTestFramework);
    }

    /**
     * Tests renderer crashes while in WebXR presentation stay in VR.
     */
    @Test
    @MediumTest
    @Restriction(RESTRICTION_TYPE_VIEWER_DAYDREAM)
    @CommandLineFlags.Remove({"enable-webvr"})
    @CommandLineFlags.Add({"enable-features=WebXR"})
    public void testRendererKilledInWebXrStaysInVr()
            throws IllegalArgumentException, InterruptedException, TimeoutException {
        rendererKilledInVrStaysInVrImpl(
                XrTestFramework.getFileUrlForHtmlTestFile("generic_webxr_page"), mXrTestFramework);
    }

    private void rendererKilledInVrStaysInVrImpl(String url, TestFramework framework)
            throws InterruptedException {
        framework.loadUrlAndAwaitInitialization(url, PAGE_LOAD_TIMEOUT_S);
        TransitionUtils.enterPresentationOrFail(framework);
        framework.simulateRendererKilled();
        Assert.assertTrue("Browser is in VR", VrShellDelegate.isInVr());
    }

    /**
     * Tests that window.rAF continues to fire when we have a non-exclusive session.
     */
    @Test
    @MediumTest
    @CommandLineFlags.Remove({"enable-webvr"})
    @CommandLineFlags.Add({"enable-features=WebXR"})
    @VrActivityRestriction({VrActivityRestriction.SupportedActivity.ALL})
    public void testWindowRafFiresDuringNonExclusiveSession() throws InterruptedException {
        mXrTestFramework.loadUrlAndAwaitInitialization(
                XrTestFramework.getFileUrlForHtmlTestFile(
                        "test_window_raf_fires_during_non_exclusive_session"),
                PAGE_LOAD_TIMEOUT_S);
        XrTestFramework.waitOnJavaScriptStep(mXrTestFramework.getFirstTabWebContents());
        XrTestFramework.endTest(mXrTestFramework.getFirstTabWebContents());
    }

    /**
     * Tests that non-exclusive sessions stop receiving rAFs during an exclusive session, but resume
     * once the exclusive session ends.
     */
    @Test
    @MediumTest
    @CommandLineFlags.Remove({"enable-webvr"})
    @CommandLineFlags.Add({"enable-features=WebXR"})
    @VrActivityRestriction({VrActivityRestriction.SupportedActivity.ALL})
    public void testNonExclusiveStopsDuringExclusive() throws InterruptedException {
        mXrTestFramework.loadUrlAndAwaitInitialization(
                XrTestFramework.getFileUrlForHtmlTestFile(
                        "test_non_exclusive_stops_during_exclusive"),
                PAGE_LOAD_TIMEOUT_S);
        XrTestFramework.executeStepAndWait(
                "stepBeforeExclusive()", mXrTestFramework.getFirstTabWebContents());
        TransitionUtils.enterPresentationOrFail(mXrTestFramework);
        XrTestFramework.executeStepAndWait(
                "stepDuringExclusive()", mXrTestFramework.getFirstTabWebContents());
        TransitionUtils.forceExitVr();
        XrTestFramework.executeStepAndWait(
                "stepAfterExclusive()", mXrTestFramework.getFirstTabWebContents());
        XrTestFramework.endTest(mXrTestFramework.getFirstTabWebContents());
    }
}
