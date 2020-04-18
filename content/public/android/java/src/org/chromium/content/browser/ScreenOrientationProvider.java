// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser;

import android.app.Activity;
import android.content.Context;
import android.content.pm.ActivityInfo;
import android.content.pm.PackageManager;
import android.view.Surface;

import org.chromium.base.Log;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.content_public.browser.ScreenOrientationDelegate;
import org.chromium.content_public.common.ScreenOrientationConstants;
import org.chromium.content_public.common.ScreenOrientationValues;
import org.chromium.ui.base.WindowAndroid;
import org.chromium.ui.display.DisplayAndroid;

import javax.annotation.Nullable;

/**
 * This is the implementation of the C++ counterpart ScreenOrientationProvider.
 */
@JNINamespace("content")
public class ScreenOrientationProvider {
    private static final String TAG = "cr.ScreenOrientation";
    private static ScreenOrientationDelegate sDelegate;

    private static int getOrientationFromWebScreenOrientations(byte orientation,
            @Nullable WindowAndroid window, Context context) {
        switch (orientation) {
            case ScreenOrientationValues.DEFAULT:
                return ActivityInfo.SCREEN_ORIENTATION_UNSPECIFIED;
            case ScreenOrientationValues.PORTRAIT_PRIMARY:
                return ActivityInfo.SCREEN_ORIENTATION_PORTRAIT;
            case ScreenOrientationValues.PORTRAIT_SECONDARY:
                return ActivityInfo.SCREEN_ORIENTATION_REVERSE_PORTRAIT;
            case ScreenOrientationValues.LANDSCAPE_PRIMARY:
                return ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE;
            case ScreenOrientationValues.LANDSCAPE_SECONDARY:
                return ActivityInfo.SCREEN_ORIENTATION_REVERSE_LANDSCAPE;
            case ScreenOrientationValues.PORTRAIT:
                return ActivityInfo.SCREEN_ORIENTATION_SENSOR_PORTRAIT;
            case ScreenOrientationValues.LANDSCAPE:
                return ActivityInfo.SCREEN_ORIENTATION_SENSOR_LANDSCAPE;
            case ScreenOrientationValues.ANY:
                return ActivityInfo.SCREEN_ORIENTATION_FULL_SENSOR;
            case ScreenOrientationValues.NATURAL:
                // If the tab is being reparented, we don't have a display strongly associated with
                // it, so we get the default display.
                DisplayAndroid displayAndroid = (window != null) ? window.getDisplay()
                        : DisplayAndroid.getNonMultiDisplay(context);
                int rotation = displayAndroid.getRotation();
                if (rotation == Surface.ROTATION_0 || rotation == Surface.ROTATION_180) {
                    if (displayAndroid.getDisplayHeight() >= displayAndroid.getDisplayWidth()) {
                        return ActivityInfo.SCREEN_ORIENTATION_PORTRAIT;
                    }
                    return ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE;
                } else {
                    if (displayAndroid.getDisplayHeight() < displayAndroid.getDisplayWidth()) {
                        return ActivityInfo.SCREEN_ORIENTATION_PORTRAIT;
                    }
                    return ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE;
                }
            default:
                Log.w(TAG, "Trying to lock to unsupported orientation!");
                return ActivityInfo.SCREEN_ORIENTATION_UNSPECIFIED;
        }
    }

    @CalledByNative
    public static void lockOrientation(@Nullable WindowAndroid window, byte webScreenOrientation) {
        if (sDelegate != null && !sDelegate.canLockOrientation()) return;

        // WindowAndroid may be null if the tab is being reparented.
        if (window == null) return;
        Activity activity = window.getActivity().get();

        // Locking orientation is only supported for web contents that have an associated activity.
        // Note that we can't just use the focused activity, as that would lead to bugs where
        // unlockOrientation unlocks a different activity to the one that was locked.
        if (activity == null) return;

        int orientation = getOrientationFromWebScreenOrientations(webScreenOrientation, window,
                activity);
        if (orientation == ActivityInfo.SCREEN_ORIENTATION_UNSPECIFIED) {
            return;
        }

        activity.setRequestedOrientation(orientation);
    }

    @CalledByNative
    public static void unlockOrientation(@Nullable WindowAndroid window) {
        // WindowAndroid may be null if the tab is being reparented.
        if (window == null) return;
        Activity activity = window.getActivity().get();

        // Locking orientation is only supported for web contents that have an associated activity.
        // Note that we can't just use the focused activity, as that would lead to bugs where
        // unlockOrientation unlocks a different activity to the one that was locked.
        if (activity == null) return;

        int defaultOrientation = ActivityInfo.SCREEN_ORIENTATION_UNSPECIFIED;

        // Activities opened from a shortcut may have EXTRA_ORIENTATION set. In
        // which case, we want to use that as the default orientation.
        int orientation = activity.getIntent().getIntExtra(
                ScreenOrientationConstants.EXTRA_ORIENTATION,
                ScreenOrientationValues.DEFAULT);
        defaultOrientation = getOrientationFromWebScreenOrientations(
                (byte) orientation, window, activity);

        try {
            if (defaultOrientation == ActivityInfo.SCREEN_ORIENTATION_UNSPECIFIED) {
                ActivityInfo info = activity.getPackageManager().getActivityInfo(
                        activity.getComponentName(), PackageManager.GET_META_DATA);
                defaultOrientation = info.screenOrientation;
            }
        } catch (PackageManager.NameNotFoundException e) {
            // Do nothing, defaultOrientation should be SCREEN_ORIENTATION_UNSPECIFIED.
        } finally {
            if (sDelegate == null || sDelegate.canUnlockOrientation(activity, defaultOrientation)) {
                activity.setRequestedOrientation(defaultOrientation);
            }
        }
    }

    @CalledByNative
    static boolean isOrientationLockEnabled() {
        return sDelegate == null || sDelegate.canLockOrientation();
    }

    public static void setOrientationDelegate(ScreenOrientationDelegate delegate) {
        sDelegate = delegate;
    }

    private ScreenOrientationProvider() {
    }
}
