// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.util;

import android.accessibilityservice.AccessibilityServiceInfo;
import android.annotation.SuppressLint;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.net.Uri;
import android.os.Build;
import android.support.v7.app.AlertDialog;
import android.view.Gravity;
import android.view.View;
import android.view.accessibility.AccessibilityManager;

import org.chromium.base.ContextUtils;
import org.chromium.base.PackageUtils;
import org.chromium.base.TraceEvent;
import org.chromium.chrome.R;
import org.chromium.ui.widget.Toast;

import java.util.List;

/**
 * Exposes information about the current accessibility state
 */
public class AccessibilityUtil {
    // Whether we've already shown an alert that they have an old version of TalkBack running.
    private static boolean sOldTalkBackVersionAlertShown;

    // The link to download or update TalkBack from the Play Store.
    private static final String TALKBACK_MARKET_LINK =
            "market://search?q=pname:com.google.android.marvin.talkback";

    // The package name for TalkBack, an Android accessibility service.
    private static final String TALKBACK_PACKAGE_NAME =
            "com.google.android.marvin.talkback";

    // The minimum TalkBack version that we support. This is the version that shipped with
    // KitKat, from fall 2013. Versions older than that should be updated.
    private static final int MIN_TALKBACK_VERSION = 105;

    private AccessibilityUtil() { }

    /**
     * Checks to see that this device has accessibility and touch exploration enabled.
     * @return        Whether or not accessibility and touch exploration are enabled.
     */
    public static boolean isAccessibilityEnabled() {
        TraceEvent.begin("AccessibilityManager::isAccessibilityEnabled");
        AccessibilityManager manager =
                (AccessibilityManager) ContextUtils.getApplicationContext().getSystemService(
                        Context.ACCESSIBILITY_SERVICE);
        boolean retVal =
                manager != null && manager.isEnabled() && manager.isTouchExplorationEnabled();

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP && manager != null
                && manager.isEnabled() && !retVal) {
            List<AccessibilityServiceInfo> services = manager.getEnabledAccessibilityServiceList(
                    AccessibilityServiceInfo.FEEDBACK_ALL_MASK);
            for (AccessibilityServiceInfo service : services) {
                if (canPerformGestures(service)) {
                    retVal = true;
                    break;
                }
            }
        }

        TraceEvent.end("AccessibilityManager::isAccessibilityEnabled");
        return retVal;
    }

    /**
     * Checks whether the given {@link AccesibilityServiceInfo} can perform gestures.
     * @param service The service to check.
     * @return Whether the {@code service} can perform gestures. On N+, this relies on the
     *         capabilities the service can perform. On L & M, this looks specifically for
     *         Switch Access.
     */
    private static boolean canPerformGestures(AccessibilityServiceInfo service) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N) {
            return (service.getCapabilities()
                           & AccessibilityServiceInfo.CAPABILITY_CAN_PERFORM_GESTURES)
                    != 0;
        } else if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            return service.getResolveInfo() != null
                    && service.getResolveInfo().toString().contains("switchaccess");
        }
        return false;
    }

    /**
     * Checks to see if an old version of TalkBack is running that Chrome doesn't support,
     * and if so, shows an alert dialog prompting the user to update the app.
     * @param context A {@link Context} instance.
     * @return        True if the dialog was shown.
     */
    public static boolean showWarningIfOldTalkbackRunning(Context context) {
        AccessibilityManager manager = (AccessibilityManager)
                context.getSystemService(Context.ACCESSIBILITY_SERVICE);
        if (manager == null) return false;

        boolean isTalkbackRunning = false;
        try {
            List<AccessibilityServiceInfo> services =
                    manager.getEnabledAccessibilityServiceList(
                            AccessibilityServiceInfo.FEEDBACK_SPOKEN);
            for (AccessibilityServiceInfo service : services) {
                if (service.getId().contains(TALKBACK_PACKAGE_NAME)) isTalkbackRunning = true;
            }
        } catch (NullPointerException e) {
            // getEnabledAccessibilityServiceList() can throw an NPE due to a bad
            // AccessibilityService.
        }
        if (!isTalkbackRunning) return false;

        if (PackageUtils.getPackageVersion(context, TALKBACK_PACKAGE_NAME) < MIN_TALKBACK_VERSION
                && !sOldTalkBackVersionAlertShown) {
            showOldTalkbackVersionAlertOnce(context);
            return true;
        }

        return false;
    }

    private static void showOldTalkbackVersionAlertOnce(final Context context) {
        if (sOldTalkBackVersionAlertShown) return;
        sOldTalkBackVersionAlertShown = true;

        AlertDialog.Builder builder = new AlertDialog.Builder(context, R.style.AlertDialogTheme)
                .setTitle(R.string.old_talkback_title)
                .setPositiveButton(R.string.update_from_market,
                        new DialogInterface.OnClickListener() {
                        @Override
                            public void onClick(DialogInterface dialog, int id) {
                                Uri marketUri = Uri.parse(TALKBACK_MARKET_LINK);
                                Intent marketIntent = new Intent(
                                        Intent.ACTION_VIEW, marketUri);
                                context.startActivity(marketIntent);
                            }
                        })
                .setNegativeButton(R.string.cancel_talkback_alert,
                        new DialogInterface.OnClickListener() {
                        @Override
                            public void onClick(DialogInterface dialog, int id) {
                                // Do nothing, this alert is only shown once either way.
                            }
                        });
        AlertDialog dialog = builder.create();
        dialog.show();
    }

    /**
     * Shows the content description toast for items on the toolbar.
     * @param context The context to use for the toast.
     * @param view The view to anchor the toast.
     * @param description The string shown in the toast.
     * @return Whether a toast has been shown successfully.
     */
    @SuppressLint("RtlHardcoded")
    public static boolean showAccessibilityToast(
            Context context, View view, CharSequence description) {
        if (description == null) return false;

        final int screenWidth = context.getResources().getDisplayMetrics().widthPixels;
        final int screenHeight = context.getResources().getDisplayMetrics().heightPixels;
        final int[] screenPos = new int[2];
        view.getLocationOnScreen(screenPos);
        final int width = view.getWidth();
        final int height = view.getHeight();

        final int horizontalGravity =
                (screenPos[0] < screenWidth / 2) ? Gravity.LEFT : Gravity.RIGHT;
        final int xOffset = (screenPos[0] < screenWidth / 2)
                ? screenPos[0] + width / 2
                : screenWidth - screenPos[0] - width / 2;
        final int yOffset = (screenPos[1] < screenHeight / 2) ? screenPos[1] + height / 2
                                                              : screenPos[1] - height * 3 / 2;

        Toast toast = Toast.makeText(context, description, Toast.LENGTH_SHORT);
        toast.setGravity(Gravity.TOP | horizontalGravity, xOffset, yOffset);
        toast.show();
        return true;
    }
}
