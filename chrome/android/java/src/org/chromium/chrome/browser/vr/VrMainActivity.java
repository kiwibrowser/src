// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.vr;

import android.app.Activity;
import android.content.Intent;
import android.content.res.Configuration;
import android.os.Build;
import android.os.Bundle;

import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.base.ApplicationStatus;
import org.chromium.base.Log;
import org.chromium.base.TraceEvent;
import org.chromium.chrome.browser.ChromeActivity;
import org.chromium.chrome.browser.LaunchIntentDispatcher;
import org.chromium.chrome.browser.document.ChromeLauncherActivity;
import org.chromium.chrome.browser.util.IntentUtils;
import org.chromium.chrome.browser.vr_shell.VrIntentUtils;
import org.chromium.chrome.browser.vr_shell.VrShellDelegate;

import java.lang.ref.WeakReference;

/**
 * This is the VR equivalent of {@link ChromeLauncherActivity}. It exists because the Android
 * platform doesn't inherently support hybrid VR Activities (like Chrome uses). All VR intents for
 * Chrome are routed through this activity as its manifest entry contains VR specific attributes to
 * ensure a smooth transition into Chrome VR, and allows us to properly handle both implicit and
 * explicit VR intents that may be missing VR categories when they get sent.
 *
 * This Activity doesn't inherit from ChromeLauncherActivity because we need to be able to finish
 * and relaunch this Launcher without calling ChromeLauncherActivity#onCreate which would fire the
 * intent we don't yet want to fire.
 */
public class VrMainActivity extends Activity {
    private static final String TAG = "VrMainActivity";

    @Override
    public void onCreate(Bundle savedInstanceState) {
        TraceEvent.begin("VrMainActivity.onCreate");
        try {
            super.onCreate(savedInstanceState);

            boolean hasDaydreamCategory = getIntent().hasCategory(VrIntentUtils.DAYDREAM_CATEGORY);

            if (!VrShellDelegate.deviceSupportsVrLaunches()
                    || (!hasDaydreamCategory && !VrShellDelegate.isInVrSession())) {
                StringBuilder error = new StringBuilder("Attempted to launch Chrome into VR ");
                if (!VrShellDelegate.deviceSupportsVrLaunches()) {
                    error.append("on a device that doesn't support Chrome in VR.");
                } else {
                    error.append("without first being in a VR session or supplying the Daydream "
                            + "category. Did you mean to use DaydreamApi#launchInVr()?");
                }
                Log.e(TAG, error.toString());

                // Remove VR flags and re-target back to the 2D launcher.
                VrIntentUtils.removeVrExtras(getIntent());
                getIntent().setClass(this, ChromeLauncherActivity.class);

                // Daydream drops the MAIN action for unknown reasons...
                if (getIntent().getAction() == null) getIntent().setAction(Intent.ACTION_MAIN);
                startActivity(getIntent());
                finish();
                return;
            }

            // If the launcher was launched from a 2D context (without calling
            // DaydreamApi#launchInVr), then we need to relaunch the launcher in VR to allow
            // downstream Activities to make assumptions about whether they're in VR or not, and
            // ensure the VR configuration is set before launching Activities as they use the VR
            // configuration to set style attributes.
            boolean needsRelaunch;
            if (Build.VERSION.SDK_INT < Build.VERSION_CODES.O) {
                needsRelaunch = !VrShellDelegate.isInVrSession();
            } else {
                Configuration config = getResources().getConfiguration();
                int uiMode = config.uiMode & Configuration.UI_MODE_TYPE_MASK;
                needsRelaunch = uiMode != Configuration.UI_MODE_TYPE_VR_HEADSET;
            }
            // The check for relaunching does not work properly if the DON flow is skipped, which
            // is the case during tests. So, allow intents to specify that relaunching is not
            // necessary.
            if (IntentUtils.safeGetBooleanExtra(
                        getIntent(), VrIntentUtils.AVOID_RELAUNCH_EXTRA, false)) {
                needsRelaunch = false;
            }
            if (needsRelaunch) {
                // Under some situations, like with the skip DON flow developer setting on, we can
                // get stuck in a relaunch loop as the VR Headset configuration won't get set. Add
                // an extra to never relaunch more than once.
                getIntent().putExtra(VrIntentUtils.AVOID_RELAUNCH_EXTRA, true);
                VrIntentUtils.launchInVr(getIntent(), this);
                finish();
                return;
            }

            // We don't set VrMode for the launcher in the manifest because that causes weird things
            // to happen when you send a VR intent to Chrome from a non-VR app, so we need to set it
            // here.
            VrShellDelegate.setVrModeEnabled(this, true);

            // Daydream likes to remove the Daydream category from explicit intents for some reason.
            // Since only implicit intents with the Daydream category can be routed here, it's safe
            // to assume that the intent *should* have the Daydream category, which simplifies our
            // intent handling.
            getIntent().addCategory(VrIntentUtils.DAYDREAM_CATEGORY);

            for (WeakReference<Activity> weakActivity : ApplicationStatus.getRunningActivities()) {
                final Activity activity = weakActivity.get();
                if (activity == null) continue;
                if (activity instanceof ChromeActivity) {
                    if (VrShellDelegate.willChangeDensityInVr((ChromeActivity) activity)) {
                        // In the rare case that entering VR will trigger a density change (and
                        // hence an Activity recreation), just return to Daydream home and kill the
                        // process, as there's no good way to recreate without showing 2D UI
                        // in-headset.
                        finish();
                        System.exit(0);
                    }
                }
            }

            @LaunchIntentDispatcher.Action
            int dispatchAction = LaunchIntentDispatcher.dispatch(this, getIntent());
            switch (dispatchAction) {
                case LaunchIntentDispatcher.Action.FINISH_ACTIVITY:
                    finish();
                    break;
                case LaunchIntentDispatcher.Action.FINISH_ACTIVITY_REMOVE_TASK:
                    ApiCompatibilityUtils.finishAndRemoveTask(this);
                    break;
                default:
                    assert false : "Intent dispatcher finished with action " + dispatchAction
                                   + ", finishing anyway";
                    finish();
                    break;
            }
        } finally {
            TraceEvent.end("VrMainActivity.onCreate");
        }
    }
}
