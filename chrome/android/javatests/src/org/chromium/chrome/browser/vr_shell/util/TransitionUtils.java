// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.vr_shell.util;

import static org.chromium.chrome.browser.vr_shell.TestFramework.POLL_CHECK_INTERVAL_SHORT_MS;

import android.app.Activity;
import android.content.Intent;
import android.net.Uri;

import org.junit.Assert;

import org.chromium.base.ContextUtils;
import org.chromium.base.ThreadUtils;
import org.chromium.chrome.browser.vr.VrMainActivity;
import org.chromium.chrome.browser.vr_shell.TestFramework;
import org.chromium.chrome.browser.vr_shell.TestVrShellDelegate;
import org.chromium.chrome.browser.vr_shell.VrIntentUtils;
import org.chromium.chrome.browser.vr_shell.VrShellDelegate;
import org.chromium.chrome.browser.vr_shell.VrShellImpl;
import org.chromium.chrome.browser.vr_shell.VrTestFramework;
import org.chromium.content.browser.test.util.Criteria;
import org.chromium.content.browser.test.util.CriteriaHelper;
import org.chromium.content.browser.test.util.DOMUtils;
import org.chromium.content_public.browser.WebContents;

import java.util.concurrent.Callable;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.TimeoutException;
import java.util.concurrent.atomic.AtomicBoolean;

/**
 * Class containing utility functions for transitioning between different
 * states in VR and XR, such as fullscreen, WebVR/WebXR presentation, and the VR browser.
 *
 * WebVR and WebXR-specific functionality can be found in the VrTransitionUtils and
 * XrTransitionUtils subclasses.
 *
 * All the transitions in this class are performed directly through Chrome,
 * as opposed to NFC tag simulation which involves receiving an intent from
 * an outside application (VR Services).
 */
public class TransitionUtils {
    /**
     * Forces the browser into VR mode via a VrShellDelegate call.
     */
    public static boolean forceEnterVr() {
        Boolean result = false;
        try {
            result = ThreadUtils.runOnUiThreadBlocking(new Callable<Boolean>() {
                @Override
                public Boolean call() throws Exception {
                    return VrShellDelegate.enterVrIfNecessary();
                }
            });
        } catch (ExecutionException e) {
        }
        return result;
    }

    /**
     * Forces the browser out of VR mode via a VrShellDelegate call.
     */
    public static void forceExitVr() {
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                VrShellDelegateUtils.getDelegateInstance().shutdownVr(
                        true /* disableVrMode */, true /* stayingInChrome */);
            }
        });
    }

    /**
     * Waits until the given VrShellDelegate's isInVR() returns true. Should
     * only be used when VR support is expected.
     * @param timeout How long to wait before giving up, in milliseconds
     */
    public static void waitForVrEntry(final int timeout) {
        // If VR Shell is supported, mInVr should eventually go to true
        // Relatively long timeout because sometimes GVR takes a while to enter VR
        CriteriaHelper.pollUiThread(Criteria.equals(true, new Callable<Boolean>() {
            @Override
            public Boolean call() {
                return VrShellDelegateUtils.getDelegateInstance().isVrEntryComplete();
            }
        }), timeout, POLL_CHECK_INTERVAL_SHORT_MS);
    }

    /**
     * Sends a click event directly to the WebGL canvas, triggering its onclick
     * that by default calls WebVR's requestPresent. Will have a similar result to
     * CardboardUtils.sendCardboardClick if not already presenting, but less prone
     * to errors, e.g. if there's dialog in the center of the screen blocking the canvas.
     *
     * Only meant to be used alongside the test framework from VrTestFramework.
     * @param cvc The ContentViewCore for the tab the canvas is in.
     */
    public static void enterPresentation(WebContents webContents) {
        try {
            DOMUtils.clickNode(webContents, "webgl-canvas", false /* goThroughRootAndroidView */);
        } catch (InterruptedException | TimeoutException e) {
            Assert.fail("Failed to click canvas to enter presentation: " + e.toString());
        }
    }

    /**
     * Sends a click event directly to the WebGL canvas then waits for the
     * JavaScript step to finish.
     *
     * Only meant to be used alongside the test framework from VrTestFramework.
     * @param cvc The ContentViewCore for the tab the canvas is in.
     * @param webContents The WebContents for the tab the JavaScript step is in.
     */
    public static void enterPresentationAndWait(WebContents webContents) {
        enterPresentation(webContents);
        TestFramework.waitOnJavaScriptStep(webContents);
    }

    /**
     * Allows the use of enterPresentationOrFail for shared WebVR and WebXR tests without having to
     * check whether we need to use the WebVR or WebXR version every time.
     */
    public static void enterPresentationOrFail(TestFramework framework) {
        if (framework instanceof VrTestFramework) {
            VrTransitionUtils.enterPresentationOrFail(framework.getFirstTabWebContents());
        } else {
            XrTransitionUtils.enterPresentationOrFail(framework.getFirstTabWebContents());
        }
    }

    /**
     * @return Whether the VR back button is enabled.
     */
    public static Boolean isBackButtonEnabled() {
        final AtomicBoolean isBackButtonEnabled = new AtomicBoolean();
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                isBackButtonEnabled.set(
                        TestVrShellDelegate.getVrShellForTesting().isBackButtonEnabled());
            }
        });
        return isBackButtonEnabled.get();
    }

    /**
     * @return Whether the VR forward button is enabled.
     */
    public static Boolean isForwardButtonEnabled() {
        final AtomicBoolean isForwardButtonEnabled = new AtomicBoolean();
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                isForwardButtonEnabled.set(
                        TestVrShellDelegate.getVrShellForTesting().isForwardButtonEnabled());
            }
        });
        return isForwardButtonEnabled.get();
    }

    /**
     * Navigates VrShell back.
     */
    public static void navigateBack() {
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                TestVrShellDelegate.getVrShellForTesting().navigateBack();
            }
        });
    }

    /**
     * Navigates VrShell forward.
     */
    public static void navigateForward() {
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                TestVrShellDelegate.getVrShellForTesting().navigateForward();
            }
        });
    }

    /**
     * Sends an intent to Chrome telling it to launch in VR mode. If the given autopresent param is
     * true, this is expected to fail unless the trusted intent check is disabled in
     * VrShellDelegate.
     *
     * @param url String containing the URL to open
     * @param activity The activity to launch the intent from
     * @param autopresent If this intent is expected to auto-present WebVR
     */
    public static void sendVrLaunchIntent(
            String url, final Activity activity, boolean autopresent, boolean avoidRelaunch) {
        // Create an intent that will launch Chrome at the specified URL.
        final Intent intent =
                new Intent(ContextUtils.getApplicationContext(), VrMainActivity.class);
        intent.setData(Uri.parse(url));
        intent.addCategory(VrIntentUtils.DAYDREAM_CATEGORY);
        VrShellDelegate.getVrClassesWrapper().setupVrIntent(intent);

        if (autopresent) {
            // Daydream removes this category for deep-linked URLs for legacy reasons.
            intent.removeCategory(VrIntentUtils.DAYDREAM_CATEGORY);
            intent.putExtra(VrIntentUtils.AUTOPRESENT_WEVBVR_EXTRA, true);
        }
        if (avoidRelaunch) intent.putExtra(VrIntentUtils.AVOID_RELAUNCH_EXTRA, true);

        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                VrShellDelegate.getVrDaydreamApi().launchInVr(intent);
            }
        });
    }

    /**
     * Waits until either a JavaScript dialog or permission prompt is being displayed using the
     * Android native UI in the VR browser.
     */
    public static void waitForNativeUiPrompt(final int timeout) {
        CriteriaHelper.pollInstrumentationThread(new Criteria() {
            @Override
            public boolean isSatisfied() {
                VrShellImpl vrShell = (VrShellImpl) TestVrShellDelegate.getVrShellForTesting();
                return vrShell.isDisplayingDialogView();
            }
        }, timeout, POLL_CHECK_INTERVAL_SHORT_MS);
    }
}
