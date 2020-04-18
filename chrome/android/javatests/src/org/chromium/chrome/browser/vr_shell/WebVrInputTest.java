// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.vr_shell;

import static org.chromium.chrome.browser.vr_shell.VrTestFramework.PAGE_LOAD_TIMEOUT_S;
import static org.chromium.chrome.browser.vr_shell.VrTestFramework.POLL_CHECK_INTERVAL_SHORT_MS;
import static org.chromium.chrome.browser.vr_shell.VrTestFramework.POLL_TIMEOUT_LONG_MS;
import static org.chromium.chrome.browser.vr_shell.VrTestFramework.POLL_TIMEOUT_SHORT_MS;
import static org.chromium.chrome.test.util.ChromeRestriction.RESTRICTION_TYPE_VIEWER_DAYDREAM;
import static org.chromium.chrome.test.util.ChromeRestriction.RESTRICTION_TYPE_VIEWER_NON_DAYDREAM;

import android.app.Activity;
import android.os.Build;
import android.os.SystemClock;
import android.support.test.filters.LargeTest;
import android.support.test.filters.MediumTest;
import android.view.MotionEvent;
import android.view.View;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.RuleChain;
import org.junit.runner.RunWith;

import org.chromium.base.ApplicationStatus;
import org.chromium.base.ThreadUtils;
import org.chromium.base.test.params.ParameterAnnotations.ClassParameter;
import org.chromium.base.test.params.ParameterAnnotations.UseRunnerDelegate;
import org.chromium.base.test.params.ParameterSet;
import org.chromium.base.test.params.ParameterizedRunner;
import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.DisableIf;
import org.chromium.base.test.util.MinAndroidSdkLevel;
import org.chromium.base.test.util.Restriction;
import org.chromium.base.test.util.RetryOnFailure;
import org.chromium.chrome.browser.ChromeSwitches;
import org.chromium.chrome.browser.customtabs.CustomTabActivity;
import org.chromium.chrome.browser.vr_shell.mock.MockVrDaydreamApi;
import org.chromium.chrome.browser.vr_shell.mock.MockVrIntentHandler;
import org.chromium.chrome.browser.vr_shell.rules.VrActivityRestriction;
import org.chromium.chrome.browser.vr_shell.util.TransitionUtils;
import org.chromium.chrome.browser.vr_shell.util.VrShellDelegateUtils;
import org.chromium.chrome.browser.vr_shell.util.VrTestRuleUtils;
import org.chromium.chrome.browser.vr_shell.util.VrTransitionUtils;
import org.chromium.chrome.test.ChromeActivityTestRule;
import org.chromium.chrome.test.ChromeJUnit4RunnerDelegate;
import org.chromium.content.browser.test.util.Criteria;
import org.chromium.content.browser.test.util.CriteriaHelper;
import org.chromium.content.browser.test.util.TouchCommon;
import org.chromium.content_public.browser.WebContents;

import java.lang.ref.WeakReference;
import java.util.List;
import java.util.concurrent.Callable;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicReference;

/**
 * End-to-end tests for sending input while using WebVR and WebXR.
 */
@RunWith(ParameterizedRunner.class)
@UseRunnerDelegate(ChromeJUnit4RunnerDelegate.class)
@CommandLineFlags.
Add({ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE, "enable-webvr", "enable-gamepad-extensions"})
@MinAndroidSdkLevel(Build.VERSION_CODES.KITKAT) // WebVR and WebXR are only supported on K+
public class WebVrInputTest {
    @ClassParameter
    private static List<ParameterSet> sClassParams =
            VrTestRuleUtils.generateDefaultVrTestRuleParameters();
    @Rule
    public RuleChain mRuleChain;

    private ChromeActivityTestRule mTestRule;
    private VrTestFramework mVrTestFramework;
    private XrTestFramework mXrTestFramework;

    public WebVrInputTest(Callable<ChromeActivityTestRule> callable) throws Exception {
        mTestRule = callable.call();
        mRuleChain = VrTestRuleUtils.wrapRuleInVrActivityRestrictionRule(mTestRule);
    }

    @Before
    public void setUp() throws Exception {
        mVrTestFramework = new VrTestFramework(mTestRule);
        mXrTestFramework = new XrTestFramework(mTestRule);
    }

    private void assertAppButtonEffect(boolean shouldHaveExited, TestFramework framework) {
        String boolExpression = (framework instanceof VrTestFramework) ? "!vrDisplay.isPresenting" :
                                                                         "exclusiveSession == null";
        Assert.assertEquals("App button exited presentation", shouldHaveExited,
                TestFramework.pollJavaScriptBoolean(boolExpression, POLL_TIMEOUT_SHORT_MS,
                        framework.getFirstTabWebContents()));
    }

    /**
     * Tests that screen touches are not registered when in VR.
     */
    @Test
    @MediumTest
    @DisableIf.Build(message = "Flaky on K/L crbug.com/762126",
            sdk_is_less_than = Build.VERSION_CODES.M)
    @VrActivityRestriction({VrActivityRestriction.SupportedActivity.ALL})
    public void testScreenTapsNotRegistered() throws InterruptedException {
        screenTapsNotRegisteredImpl(
                VrTestFramework.getFileUrlForHtmlTestFile("test_screen_taps_not_registered"),
                mVrTestFramework);
    }

    /**
     * Tests that screen touches are not registered when in an exclusive session.
     */
    @Test
    @MediumTest
    @DisableIf.Build(message = "Flaky on K/L crbug.com/762126",
            sdk_is_less_than = Build.VERSION_CODES.M)
    @CommandLineFlags.Remove({"enable-webvr"})
    @CommandLineFlags.Add({"enable-features=WebXR"})
    @VrActivityRestriction({VrActivityRestriction.SupportedActivity.ALL})
    public void testScreenTapsNotRegistered_WebXr() throws InterruptedException {
        screenTapsNotRegisteredImpl(
                XrTestFramework.getFileUrlForHtmlTestFile("webxr_test_screen_taps_not_registered"),
                mXrTestFramework);
    }

    private void screenTapsNotRegisteredImpl(String url, final TestFramework framework)
            throws InterruptedException {
        framework.loadUrlAndAwaitInitialization(url, PAGE_LOAD_TIMEOUT_S);
        TestFramework.executeStepAndWait(
                "stepVerifyNoInitialTaps()", framework.getFirstTabWebContents());
        TransitionUtils.enterPresentationOrFail(framework);
        // Wait on VrShellImpl to say that its parent consumed the touch event
        // Set to 2 because there's an ACTION_DOWN followed by ACTION_UP
        final CountDownLatch touchRegisteredLatch = new CountDownLatch(2);
        ((VrShellImpl) TestVrShellDelegate.getVrShellForTesting())
                .setOnDispatchTouchEventForTesting(new OnDispatchTouchEventCallback() {
                    @Override
                    public void onDispatchTouchEvent(boolean parentConsumed) {
                        if (!parentConsumed) Assert.fail("Parent did not consume event");
                        touchRegisteredLatch.countDown();
                    }
                });
        TouchCommon.singleClickView(mTestRule.getActivity().getWindow().getDecorView());
        Assert.assertTrue("VrShellImpl dispatched touches",
                touchRegisteredLatch.await(POLL_TIMEOUT_SHORT_MS, TimeUnit.MILLISECONDS));
        TestFramework.executeStepAndWait(
                "stepVerifyNoAdditionalTaps()", framework.getFirstTabWebContents());
        TestFramework.endTest(framework.getFirstTabWebContents());
    }

    /**
     * Tests that Daydream controller clicks are registered as gamepad button pressed.
     */
    @Test
    @LargeTest
    @Restriction(RESTRICTION_TYPE_VIEWER_DAYDREAM)
    @VrActivityRestriction({VrActivityRestriction.SupportedActivity.ALL})
    public void testControllerClicksRegisteredOnDaydream() throws InterruptedException {
        EmulatedVrController controller = new EmulatedVrController(mTestRule.getActivity());
        mVrTestFramework.loadUrlAndAwaitInitialization(
                VrTestFramework.getFileUrlForHtmlTestFile("test_gamepad_button"),
                PAGE_LOAD_TIMEOUT_S);
        // Wait to enter VR
        VrTransitionUtils.enterPresentationOrFail(mVrTestFramework.getFirstTabWebContents());
        // The Gamepad API can flakily fail to detect the gamepad from a single button press, so
        // spam it with button presses
        boolean controllerConnected = false;
        for (int i = 0; i < 10; i++) {
            controller.performControllerClick();
            if (VrTestFramework
                            .runJavaScriptOrFail("index != -1", POLL_TIMEOUT_SHORT_MS,
                                    mVrTestFramework.getFirstTabWebContents())
                            .equals("true")) {
                controllerConnected = true;
                break;
            }
        }
        Assert.assertTrue("Gamepad API detected controller", controllerConnected);
        // It's possible for input to get backed up if the emulated controller is being slow, so
        // ensure that any outstanding output has been received before starting by waiting for
        // 60 frames (1 second) of not receiving input.
        VrTestFramework.pollJavaScriptBoolean("isInputDrained()", POLL_TIMEOUT_LONG_MS,
                mVrTestFramework.getFirstTabWebContents());
        // Have a separate start condition so that the above presses/releases don't get
        // accidentally detected during the actual test
        VrTestFramework.runJavaScriptOrFail("canStartTest = true;", POLL_TIMEOUT_SHORT_MS,
                mVrTestFramework.getFirstTabWebContents());
        // Send a controller click and wait for JavaScript to receive it.
        controller.sendClickButtonToggleEvent();
        VrTestFramework.waitOnJavaScriptStep(mVrTestFramework.getFirstTabWebContents());
        controller.sendClickButtonToggleEvent();
        VrTestFramework.waitOnJavaScriptStep(mVrTestFramework.getFirstTabWebContents());
        VrTestFramework.endTest(mVrTestFramework.getFirstTabWebContents());
    }

    /**
     * Tests that Daydream controller clicks are registered as XR input in an exclusive session.
     */
    @Test
    @MediumTest
    @Restriction(RESTRICTION_TYPE_VIEWER_DAYDREAM)
    @CommandLineFlags.Remove({"enable-webvr"})
    @CommandLineFlags.Add({"enable-features=WebXR"})
    @VrActivityRestriction({VrActivityRestriction.SupportedActivity.ALL})
    public void testControllerClicksRegisteredOnDaydream_WebXr() throws InterruptedException {
        EmulatedVrController controller = new EmulatedVrController(mTestRule.getActivity());
        mXrTestFramework.loadUrlAndAwaitInitialization(
                XrTestFramework.getFileUrlForHtmlTestFile("test_webxr_input"), PAGE_LOAD_TIMEOUT_S);
        TransitionUtils.enterPresentationOrFail(mXrTestFramework);

        int numIterations = 10;
        XrTestFramework.runJavaScriptOrFail(
                "stepSetupListeners(" + String.valueOf(numIterations) + ")", POLL_TIMEOUT_SHORT_MS,
                mXrTestFramework.getFirstTabWebContents());

        // Click the touchpad a bunch of times and make sure they're all registered.
        for (int i = 0; i < numIterations; i++) {
            controller.sendClickButtonToggleEvent();
            controller.sendClickButtonToggleEvent();
            // The controller emulation can sometimes deliver controller input at weird times such
            // that we only register 8 or 9 of the 10 press/release pairs. So, send a press/release
            // and wait for it to register before doing another.
            XrTestFramework.waitOnJavaScriptStep(mXrTestFramework.getFirstTabWebContents());
        }

        XrTestFramework.endTest(mXrTestFramework.getFirstTabWebContents());
    }

    private long sendScreenTouchDown(final View view, final int x, final int y) {
        long downTime = SystemClock.uptimeMillis();
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                view.dispatchTouchEvent(
                        MotionEvent.obtain(downTime, downTime, MotionEvent.ACTION_DOWN, x, y, 0));
            }
        });
        return downTime;
    }

    private void sendScreenTouchUp(final View view, final int x, final int y, final long downTime) {
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                long now = SystemClock.uptimeMillis();
                view.dispatchTouchEvent(
                        MotionEvent.obtain(downTime, now, MotionEvent.ACTION_UP, x, y, 0));
            }
        });
    }

    /**
     * Tests that screen touches are still registered when the viewer is Cardboard.
     */
    @Test
    @MediumTest
    @Restriction(RESTRICTION_TYPE_VIEWER_NON_DAYDREAM)
    @VrActivityRestriction({VrActivityRestriction.SupportedActivity.ALL})
    public void testScreenTapsRegisteredOnCardboard() throws InterruptedException {
        mVrTestFramework.loadUrlAndAwaitInitialization(
                VrTestFramework.getFileUrlForHtmlTestFile("test_gamepad_button"),
                PAGE_LOAD_TIMEOUT_S);
        // This boolean is used by testControllerClicksRegisteredOnDaydream to prevent some
        // flakiness, but is unnecessary here, so set immediately
        VrTestFramework.runJavaScriptOrFail("canStartTest = true;", POLL_TIMEOUT_SHORT_MS,
                mVrTestFramework.getFirstTabWebContents());
        // Wait to enter VR
        VrTransitionUtils.enterPresentationOrFail(mVrTestFramework.getFirstTabWebContents());
        int x = mVrTestFramework.getFirstTabContentView().getWidth() / 2;
        int y = mVrTestFramework.getFirstTabContentView().getHeight() / 2;
        // TODO(mthiesse, https://crbug.com/758374): Injecting touch events into the root GvrLayout
        // (VrShellImpl) is flaky. Sometimes the events just don't get routed to the presentation
        // view for no apparent reason. We should figure out why this is and see if it's fixable.
        final View presentationView = ((VrShellImpl) TestVrShellDelegate.getVrShellForTesting())
                                              .getPresentationViewForTesting();
        long downTime = sendScreenTouchDown(presentationView, x, y);
        VrTestFramework.waitOnJavaScriptStep(mVrTestFramework.getFirstTabWebContents());
        sendScreenTouchUp(presentationView, x, y, downTime);
        VrTestFramework.waitOnJavaScriptStep(mVrTestFramework.getFirstTabWebContents());
        VrTestFramework.endTest(mVrTestFramework.getFirstTabWebContents());
    }

    /**
     * Tests that screen touches are registered as XR input when the viewer is Cardboard.
     */
    @Test
    @MediumTest
    @Restriction(RESTRICTION_TYPE_VIEWER_NON_DAYDREAM)
    @CommandLineFlags.Remove({"enable-webvr"})
    @CommandLineFlags.Add({"enable-features=WebXR"})
    @VrActivityRestriction({VrActivityRestriction.SupportedActivity.ALL})
    public void testScreenTapsRegisteredOnCardboard_WebXr() throws InterruptedException {
        EmulatedVrController controller = new EmulatedVrController(mTestRule.getActivity());
        mXrTestFramework.loadUrlAndAwaitInitialization(
                XrTestFramework.getFileUrlForHtmlTestFile("test_webxr_input"), PAGE_LOAD_TIMEOUT_S);
        TransitionUtils.enterPresentationOrFail(mXrTestFramework);
        int numIterations = 10;
        XrTestFramework.runJavaScriptOrFail(
                "stepSetupListeners(" + String.valueOf(numIterations) + ")", POLL_TIMEOUT_SHORT_MS,
                mXrTestFramework.getFirstTabWebContents());

        int x = mXrTestFramework.getFirstTabContentView().getWidth() / 2;
        int y = mXrTestFramework.getFirstTabContentView().getHeight() / 2;
        // TODO(mthiesse, https://crbug.com/758374): Injecting touch events into the root GvrLayout
        // (VrShellImpl) is flaky. Sometimes the events just don't get routed to the presentation
        // view for no apparent reason. We should figure out why this is and see if it's fixable.
        final View presentationView = ((VrShellImpl) TestVrShellDelegate.getVrShellForTesting())
                                              .getPresentationViewForTesting();

        // Tap the screen a bunch of times and make sure that they're all registered.
        // Android doesn't seem to like sending touch events too quickly, so have a short delay
        // between events.
        for (int i = 0; i < numIterations; i++) {
            long downTime = sendScreenTouchDown(presentationView, x, y);
            SystemClock.sleep(100);
            sendScreenTouchUp(presentationView, x, y, downTime);
            SystemClock.sleep(100);
        }

        XrTestFramework.waitOnJavaScriptStep(mXrTestFramework.getFirstTabWebContents());
        XrTestFramework.endTest(mXrTestFramework.getFirstTabWebContents());
    }

    /**
     * Tests that focus is locked to the presenting display for purposes of VR input.
     */
    @Test
    @MediumTest
    @VrActivityRestriction({VrActivityRestriction.SupportedActivity.ALL})
    public void testPresentationLocksFocus() throws InterruptedException {
        presentationLocksFocusImpl(
                VrTestFramework.getFileUrlForHtmlTestFile("test_presentation_locks_focus"),
                mVrTestFramework);
    }

    /**
     * Tests that focus is locked to the device with an exclusive session for the purposes of
     * VR input.
     */
    @Test
    @MediumTest
    @CommandLineFlags.Remove({"enable-webvr"})
    @CommandLineFlags.Add({"enable-features=WebXR"})
    @VrActivityRestriction({VrActivityRestriction.SupportedActivity.ALL})
    public void testPresentationLocksFocus_WebXr() throws InterruptedException {
        presentationLocksFocusImpl(
                XrTestFramework.getFileUrlForHtmlTestFile("webxr_test_presentation_locks_focus"),
                mXrTestFramework);
    }

    private void presentationLocksFocusImpl(String url, TestFramework framework)
            throws InterruptedException {
        framework.loadUrlAndAwaitInitialization(url, PAGE_LOAD_TIMEOUT_S);
        TransitionUtils.enterPresentationOrFail(framework);
        TestFramework.executeStepAndWait(
                "stepSetupFocusLoss()", framework.getFirstTabWebContents());
        TestFramework.endTest(framework.getFirstTabWebContents());
    }

    /**
     * Verifies that pressing the Daydream controller's 'app' button causes the user to exit
     * WebVR presentation.
     */
    @Test
    @MediumTest
    @Restriction(RESTRICTION_TYPE_VIEWER_DAYDREAM)
    @RetryOnFailure(message = "Very rarely, button press not registered (race condition?)")
    public void testAppButtonExitsPresentation() throws InterruptedException {
        appButtonExitsPresentationImpl(
                VrTestFramework.getFileUrlForHtmlTestFile("generic_webvr_page"), mVrTestFramework);
    }

    /**
     * Tests that pressing the Daydream controller's 'app' button causes the user to exit a
     * WebXR exclusive session.
     */
    @Test
    @MediumTest
    @Restriction(RESTRICTION_TYPE_VIEWER_DAYDREAM)
    @CommandLineFlags.Remove({"enable-webvr"})
    @CommandLineFlags.Add({"enable-features=WebXR"})
    @RetryOnFailure(message = "Very rarely, button press not registered (race condition?)")
    public void testAppButtonExitsPresentation_WebXr() throws InterruptedException {
        appButtonExitsPresentationImpl(
                XrTestFramework.getFileUrlForHtmlTestFile("generic_webxr_page"), mXrTestFramework);
    }

    private void appButtonExitsPresentationImpl(String url, TestFramework framework)
            throws InterruptedException {
        framework.loadUrlAndAwaitInitialization(url, PAGE_LOAD_TIMEOUT_S);
        TransitionUtils.enterPresentationOrFail(framework);
        EmulatedVrController controller = new EmulatedVrController(mTestRule.getActivity());
        controller.pressReleaseAppButton();
        assertAppButtonEffect(true /* shouldHaveExited */, framework);
    }

    /**
     * Verifies that pressing the Daydream controller's 'app' button does not cause the user to exit
     * WebVR presentation when VR browsing is disabled.
     */
    @Test
    @MediumTest
    @Restriction(RESTRICTION_TYPE_VIEWER_DAYDREAM)
    @VrActivityRestriction({VrActivityRestriction.SupportedActivity.ALL})
    public void testAppButtonNoopsWhenBrowsingDisabled()
            throws InterruptedException, ExecutionException {
        appButtonNoopsTestImpl(
                VrTestFramework.getFileUrlForHtmlTestFile("generic_webvr_page"), mVrTestFramework);
    }

    /**
     * Verifies that pressing the Daydream controller's 'app' button does not cause the user to exit
     * WebVR presentation when VR browsing isn't supported by the Activity.
     */
    @Test
    @MediumTest
    @Restriction(RESTRICTION_TYPE_VIEWER_DAYDREAM)
    @VrActivityRestriction({VrActivityRestriction.SupportedActivity.WAA,
            VrActivityRestriction.SupportedActivity.CCT})
    public void
    testAppButtonNoopsWhenBrowsingNotSupported() throws InterruptedException, ExecutionException {
        appButtonNoopsTestImpl(
                VrTestFramework.getFileUrlForHtmlTestFile("generic_webvr_page"), mVrTestFramework);
    }

    /**
     * Verifies that pressing the Daydream controller's 'app' button does not cause the user to exit
     * a WebXR exclusive session when VR browsing is disabled.
     */
    @Test
    @MediumTest
    @Restriction(RESTRICTION_TYPE_VIEWER_DAYDREAM)
    @VrActivityRestriction({VrActivityRestriction.SupportedActivity.ALL})
    @CommandLineFlags.Remove({"enable-webvr"})
    @CommandLineFlags.Add({"enable-features=WebXR"})
    public void testAppButtonNoopsWhenBrowsingDisabled_WebXr()
            throws InterruptedException, ExecutionException {
        appButtonNoopsTestImpl(
                XrTestFramework.getFileUrlForHtmlTestFile("generic_webxr_page"), mXrTestFramework);
    }

    /**
     * Verifies that pressing the Daydream controller's 'app' button does not cause the user to exit
     * a WebXR exclusive session when VR browsing isn't supported by the Activity.
     */
    @Test
    @MediumTest
    @Restriction(RESTRICTION_TYPE_VIEWER_DAYDREAM)
    @VrActivityRestriction({VrActivityRestriction.SupportedActivity.WAA,
            VrActivityRestriction.SupportedActivity.CCT})
    @CommandLineFlags.Remove({"enable-webvr"})
    @CommandLineFlags.Add({"enable-features=WebXR"})
    public void testAppButtonNoopsWhenBrowsingNotSupported_WebXr()
            throws InterruptedException, ExecutionException {
        appButtonNoopsTestImpl(
                XrTestFramework.getFileUrlForHtmlTestFile("generic_webxr_page"), mXrTestFramework);
    }

    private void appButtonNoopsTestImpl(String url, TestFramework framework)
            throws InterruptedException, ExecutionException {
        VrShellDelegateUtils.getDelegateInstance().setVrBrowsingDisabled(true);
        framework.loadUrlAndAwaitInitialization(url, PAGE_LOAD_TIMEOUT_S);
        TransitionUtils.enterPresentationOrFail(framework);

        MockVrDaydreamApi mockApi = new MockVrDaydreamApi();
        VrShellDelegateUtils.getDelegateInstance().overrideDaydreamApiForTesting(mockApi);

        EmulatedVrController controller = new EmulatedVrController(mTestRule.getActivity());
        controller.pressReleaseAppButton();
        Assert.assertFalse("App button left Chrome",
                ThreadUtils.runOnUiThreadBlocking(new Callable<Boolean>() {
                    @Override
                    public Boolean call() throws Exception {
                        return mockApi.getExitFromVrCalled()
                                || mockApi.getLaunchVrHomescreenCalled();
                    }
                }));
        assertAppButtonEffect(false /* shouldHaveExited */, framework);
        VrShellDelegateUtils.getDelegateInstance().overrideDaydreamApiForTesting(null);
    }

    /**
     * Verifies that pressing the Daydream controller's 'app' button does not cause the user to exit
     * WebVR presentation if they entered it via a Deep Link intent.
     */
    @Test
    @MediumTest
    @CommandLineFlags.Add("enable-features=WebVrAutopresentFromIntent")
    @Restriction(RESTRICTION_TYPE_VIEWER_DAYDREAM)
    public void testAppButtonNoopsWhenDeepLinked() throws InterruptedException, ExecutionException {
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

        // Wait for autopresent to kick in
        WebContents wc = cct.get().getActivityTab().getWebContents();
        VrTestFramework.waitOnJavaScriptStep(wc);
        Assert.assertTrue("CCT entered presentation",
                VrTestFramework.pollJavaScriptBoolean(
                        "vrDisplay.isPresenting", POLL_TIMEOUT_LONG_MS, wc));

        // Verify that pressing the app button does nothing
        MockVrDaydreamApi mockApi = new MockVrDaydreamApi();
        VrShellDelegateUtils.getDelegateInstance().overrideDaydreamApiForTesting(mockApi);

        EmulatedVrController controller = new EmulatedVrController(cct.get());
        controller.pressReleaseAppButton();
        Assert.assertFalse("App button left Chrome",
                ThreadUtils.runOnUiThreadBlocking(new Callable<Boolean>() {
                    @Override
                    public Boolean call() throws Exception {
                        return mockApi.getExitFromVrCalled()
                                || mockApi.getLaunchVrHomescreenCalled();
                    }
                }));
        Assert.assertFalse("App button exited WebVR presentation",
                VrTestFramework.pollJavaScriptBoolean(
                        "!vrDisplay.isPresenting", POLL_TIMEOUT_SHORT_MS, wc));

        VrTestFramework.endTest(wc);
        VrShellDelegateUtils.getDelegateInstance().overrideDaydreamApiForTesting(null);
    }

    /**
     * Tests that focus loss updates synchronously.
     */
    @Test
    @MediumTest
    @Restriction(RESTRICTION_TYPE_VIEWER_DAYDREAM)
    @VrActivityRestriction({VrActivityRestriction.SupportedActivity.ALL})
    public void testFocusUpdatesSynchronously() throws InterruptedException {
        mVrTestFramework.loadUrlAndAwaitInitialization(
                VrTestFramework.getFileUrlForHtmlTestFile(
                        "generic_webvr_page_with_activate_listener"),
                PAGE_LOAD_TIMEOUT_S);

        CriteriaHelper.pollUiThread(new Criteria("DisplayActivate was never registered.") {
            @Override
            public boolean isSatisfied() {
                return VrShellDelegateUtils.getDelegateInstance().isListeningForWebVrActivate();
            }
        }, POLL_TIMEOUT_LONG_MS, POLL_CHECK_INTERVAL_SHORT_MS);
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                mTestRule.getActivity().getActivityTab().getContentViewCore().onPause();

                Assert.assertFalse(
                        VrShellDelegateUtils.getDelegateInstance().isListeningForWebVrActivate());
            }
        });
    }

    /**
     * Verifies that pressing the Daydream controller's 'app' button causes the user to exit
     * WebVR presentation even when the page is not submitting frames.
     */
    @Test
    @MediumTest
    @Restriction(RESTRICTION_TYPE_VIEWER_DAYDREAM)
    @RetryOnFailure(message = "Very rarely, button press not registered (race condition?)")
    public void testAppButtonAfterPageStopsSubmitting() throws InterruptedException {
        appButtonAfterPageStopsSubmittingImpl(
                VrTestFramework.getFileUrlForHtmlTestFile("webvr_page_submits_once"),
                mVrTestFramework);
    }

    /**
     * Verifies that pressing the Daydream controller's 'app' button causes the user to exit
     * a WebXR presentation even when the page is not submitting frames.
     */
    @Test
    @MediumTest
    @Restriction(RESTRICTION_TYPE_VIEWER_DAYDREAM)
    @CommandLineFlags.Remove({"enable-webvr"})
    @CommandLineFlags.Add({"enable-features=WebXR"})
    @RetryOnFailure(message = "Very rarely, button press not registered (race condition?)")
    public void testAppButtonAfterPageStopsSubmitting_WebXr() throws InterruptedException {
        appButtonAfterPageStopsSubmittingImpl(
                XrTestFramework.getFileUrlForHtmlTestFile("webxr_page_submits_once"),
                mXrTestFramework);
    }

    private void appButtonAfterPageStopsSubmittingImpl(String url, TestFramework framework)
            throws InterruptedException {
        framework.loadUrlAndAwaitInitialization(url, PAGE_LOAD_TIMEOUT_S);
        TransitionUtils.enterPresentationOrFail(framework);
        // Wait for page to stop submitting frames.
        TestFramework.waitOnJavaScriptStep(framework.getFirstTabWebContents());
        EmulatedVrController controller = new EmulatedVrController(mTestRule.getActivity());
        controller.pressReleaseAppButton();
        assertAppButtonEffect(true /* shouldHaveExited */, framework);
    }
}
