// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.vr_shell;

import android.app.Activity;
import android.app.ActivityOptions;
import android.content.Context;
import android.content.Intent;
import android.content.res.Configuration;
import android.os.Build;
import android.os.Bundle;
import android.view.Display;

import org.chromium.base.VisibleForTesting;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.IntentHandler;
import org.chromium.chrome.browser.util.IntentUtils;
import org.chromium.chrome.browser.vr.VrMainActivity;

/**
 * Utilities dealing with extracting information about VR intents.
 */
public class VrIntentUtils {
    private static final String DAYDREAM_HOME_PACKAGE = "com.google.android.vr.home";
    // The Daydream Home app adds this extra to auto-present intents.
    public static final String AUTOPRESENT_WEVBVR_EXTRA = "browser.vr.AUTOPRESENT_WEBVR";
    public static final String DAYDREAM_CATEGORY = "com.google.intent.category.DAYDREAM";
    // Tells Chrome not to relaunch itself when receiving a VR intent. This is used by tests since
    // the relaunch logic does not work properly with the DON flow skipped.
    public static final String AVOID_RELAUNCH_EXTRA =
            "org.chromium.chrome.browser.vr_shell.AVOID_RELAUNCH";

    static final String VR_FRE_INTENT_EXTRA = "org.chromium.chrome.browser.vr_shell.VR_FRE";
    static final String VR_FRE_CALLER_INTENT_EXTRA =
            "org.chromium.chrome.browser.vr_shell.VR_FRE_CALLER";

    private static VrIntentHandler sHandlerInstance;

    /**
     * Handles VR intent checking for VrShellDelegate.
     * TODO(ymalik): There's no reason for this to be an interface, refactor
     * into default implementation that tests can override.
     */
    public interface VrIntentHandler {
        /**
         * Determines whether the given intent is a VR intent from Daydream Home.
         * @param intent The intent to check
         * @return Whether the intent is a VR intent and originated from Daydream Home
         */
        boolean isTrustedDaydreamIntent(Intent intent);

        /**
         * Determines whether the given intent is a VR intent that is allowed to auto-present WebVR
         * content.
         * @param intent The intent to check
         * @return Whether the intent should be allowed to auto-present.
         */
        boolean isTrustedAutopresentIntent(Intent intent);
    }

    private static VrIntentHandler createInternalVrIntentHandler() {
        return new VrIntentHandler() {
            @Override
            public boolean isTrustedDaydreamIntent(Intent intent) {
                return isVrIntent(intent)
                        && IntentHandler.isIntentFromTrustedApp(intent, DAYDREAM_HOME_PACKAGE);
            }

            @Override
            public boolean isTrustedAutopresentIntent(Intent intent) {
                // Note that all auto-present intents may not have the intent extra because the user
                // may have an older version of the Daydream app which doesn't add this extra.
                // This is probably fine because we mostly use isTrustedDaydreamIntent above to
                // start auto-presentation. We should switch those calls to use this method when
                // we're sure that most clients have the change.
                return isTrustedDaydreamIntent(intent)
                        && IntentUtils.safeGetBooleanExtra(intent, AUTOPRESENT_WEVBVR_EXTRA, false);
            }
        };
    }

    /**
     * Gets the static VrIntentHandler instance.
     * @return The VrIntentHandler instance
     */
    public static VrIntentHandler getHandlerInstance() {
        if (sHandlerInstance == null) {
            sHandlerInstance = createInternalVrIntentHandler();
        }
        return sHandlerInstance;
    }

    @VisibleForTesting
    public static void setHandlerInstanceForTesting(VrIntentHandler handler) {
        sHandlerInstance = handler;
    }

    /**
     * @return Whether or not the given intent is a VR-specific intent.
     */
    public static boolean isVrIntent(Intent intent) {
        // For simplicity, we only return true here if VR is enabled on the platform and this intent
        // is not fired from a recent apps page. The latter is there so that we don't enter VR mode
        // when we're being resumed from the recent apps in 2D mode.
        // Note that Daydream removes the Daydream category for deep-links (for no real reason). In
        // addition to the category, DAYDREAM_VR_EXTRA tells us that this intent is coming directly
        // from VR.
        return intent != null && intent.hasCategory(DAYDREAM_CATEGORY)
                && !launchedFromRecentApps(intent) && VrShellDelegate.isVrEnabled();
    }

    /**
     * @param activity The Activity to check.
     * @param intent The intent the Activity was launched with.
     * @return Whether this Activity is launching into VR.
     */
    public static boolean isLaunchingIntoVr(Activity activity, Intent intent) {
        if (!VrShellDelegate.deviceSupportsVrLaunches()) return false;
        return isLaunchingIntoVrBrowsing(activity, intent) || isCustomTabVrIntent(intent);
    }

    private static boolean isLaunchingIntoVrBrowsing(Activity activity, Intent intent) {
        return isVrIntent(intent) && VrShellDelegate.activitySupportsVrBrowsing(activity);
    }

    /**
     * @return whether the given intent is should open in a Custom Tab.
     */
    public static boolean isCustomTabVrIntent(Intent intent) {
        // TODO(crbug.com/719661): Currently, only Daydream intents open in a CustomTab. We should
        // probably change this once we figure out core CCT flows in VR.
        if (intent == null) return false;
        return IntentHandler.getUrlFromIntent(intent) != null
                && getHandlerInstance().isTrustedDaydreamIntent(intent);
    }

    /**
     * This function returns an intent that will launch a VR activity that will prompt the
     * user to take off their headset and forward the freIntent to the standard
     * 2D FRE activity.
     *
     * @param freCallerIntent The intent that is used to launch the caller.
     * @param freIntent       The intent that will be used to start the first run in 2D mode.
     * @return The intermediate VR activity intent.
     */
    public static Intent setupVrFreIntent(
            Context context, Intent freCallerIntent, Intent freIntent) {
        if (!VrShellDelegate.isVrEnabled()) return freIntent;
        Intent intent = new Intent();
        intent.setClassName(context, VrFirstRunActivity.class.getName());
        intent.addCategory(DAYDREAM_CATEGORY);
        intent.putExtra(VR_FRE_CALLER_INTENT_EXTRA, new Intent(freCallerIntent));
        intent.putExtra(VR_FRE_INTENT_EXTRA, new Intent(freIntent));
        return intent;
    }

    /**
     * @return Options that a VR-specific Chrome activity should be launched with.
     */
    public static Bundle getVrIntentOptions(Context context) {
        if (!VrShellDelegate.isVrEnabled()) return null;
        // These options are used to start the Activity with a custom animation to keep it hidden
        // for a few hundred milliseconds - enough time for us to draw the first black view.
        // The animation is sufficient to hide the 2D screenshot but not to the 2D UI while the
        // WebVR page is being loaded because the animation is somehow cancelled when we try to
        // enter VR (I don't know what's canceling it). To hide the 2D UI, we resort to the black
        // overlay view added in {@link startWithVrIntentPreNative}.
        int animation = VrShellDelegate.USE_HIDE_ANIMATION ? R.anim.stay_hidden : 0;
        ActivityOptions options = ActivityOptions.makeCustomAnimation(context, animation, 0);
        if (VrShellDelegate.getVrClassesWrapper().bootsToVr()) {
            if (Build.VERSION.SDK_INT < Build.VERSION_CODES.O) {
                assert false;
            } else {
                options.setLaunchDisplayId(Display.DEFAULT_DISPLAY);
            }
        }
        return options.toBundle();
    }

    /**
     * @param intent The intent to launch in VR after going through the DON (Device On) flow.
     * @param activity The activity context to launch the intent from.
     */
    public static void launchInVr(Intent intent, Activity activity) {
        VrShellDelegate.getVrDaydreamApi().launchInVr(intent);
    }

    /**
     * @param intent The intent to possibly forward to the VR launcher.
     * @param activity The activity launching the intent.
     * @return whether the intent was forwarded to the VR launcher.
     */
    public static boolean maybeForwardToVrLauncher(Intent intent, Activity activity) {
        // Standalone VR devices use 2D-in-VR rendering on O+.
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.O) return false;
        if (activity instanceof VrMainActivity) return false;
        if (wouldUse2DInVrRenderingMode(activity) && VrShellDelegate.deviceSupportsVrLaunches()) {
            Intent vrIntent = new Intent(intent);
            vrIntent.setComponent(null);
            vrIntent.setPackage(activity.getPackageName());
            vrIntent.addCategory(VrIntentUtils.DAYDREAM_CATEGORY);
            if (vrIntent.resolveActivity(activity.getPackageManager()) != null) {
                VrIntentUtils.launchInVr(vrIntent, activity);
                return true;
            }
        }
        return false;
    }

    /**
     * @param activity A context for reading the current device configuration.
     * @return Whether launching a non-VR Activity would trigger the 2D-in-VR rendering path.
     */
    public static boolean wouldUse2DInVrRenderingMode(Activity activity) {
        Configuration config = activity.getResources().getConfiguration();
        int uiMode = config.uiMode & Configuration.UI_MODE_TYPE_MASK;
        if (uiMode != Configuration.UI_MODE_TYPE_VR_HEADSET) return false;
        VrClassesWrapper wrapper = VrShellDelegate.getVrClassesWrapper();
        return wrapper != null && (wrapper.bootsToVr() || wrapper.supports2dInVr());
    }

    /**
     * @return Whether the intent is fired from the recent apps overview.
     */
    /* package */ static boolean launchedFromRecentApps(Intent intent) {
        return ((intent.getFlags() & Intent.FLAG_ACTIVITY_LAUNCHED_FROM_HISTORY) != 0);
    }

    /**
     * Removes VR specific extras from the given intent to make it a non-VR intent.
     */
    public static void removeVrExtras(Intent intent) {
        if (intent == null) return;
        intent.removeCategory(DAYDREAM_CATEGORY);
        assert !isVrIntent(intent);
    }
}
