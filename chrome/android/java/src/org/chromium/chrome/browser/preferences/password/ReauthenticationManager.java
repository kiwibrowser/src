// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.preferences.password;

import android.app.Fragment;
import android.app.FragmentManager;
import android.app.FragmentTransaction;
import android.app.KeyguardManager;
import android.content.Context;
import android.os.Build;
import android.os.Bundle;
import android.support.annotation.IntDef;
import android.support.annotation.Nullable;
import android.view.View;

import org.chromium.base.VisibleForTesting;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

/**
 * This collection of static methods provides reauthentication primitives for passwords
 * settings UI.
 */
public final class ReauthenticationManager {
    // Used for various ways to override checks provided by this class.
    @Retention(RetentionPolicy.SOURCE)
    @IntDef({OVERRIDE_STATE_NOT_OVERRIDDEN, OVERRIDE_STATE_AVAILABLE, OVERRIDE_STATE_UNAVAILABLE})
    public @interface OverrideState {}
    public static final int OVERRIDE_STATE_NOT_OVERRIDDEN = 0;
    public static final int OVERRIDE_STATE_AVAILABLE = 1;
    public static final int OVERRIDE_STATE_UNAVAILABLE = 2;

    // Used to specify the scope of the reauthentication -- either to grant bulk access like, e.g.,
    // exporting passwords, or just one-at-a-time, like, e.g., viewing a single password.
    @Retention(RetentionPolicy.SOURCE)
    @IntDef({REAUTH_SCOPE_ONE_AT_A_TIME, REAUTH_SCOPE_BULK})
    public @interface ReauthScope {}
    public static final int REAUTH_SCOPE_ONE_AT_A_TIME = 0;
    public static final int REAUTH_SCOPE_BULK = 1;

    // Useful for retrieving the fragment in tests.
    @VisibleForTesting
    public static final String FRAGMENT_TAG = "reauthentication-manager-fragment";

    // Defines how long a successful reauthentication remains valid.
    @VisibleForTesting
    public static final int VALID_REAUTHENTICATION_TIME_INTERVAL_MILLIS = 60000;

    // Used for verifying if the last successful reauthentication is still valid. The null value
    // means there was no successful reauthentication yet.
    @Nullable
    private static Long sLastReauthTimeMillis;

    // Stores the reauth scope used when |sLastReauthTimeMillis| was reset last time.
    @ReauthScope
    private static int sLastReauthScope = REAUTH_SCOPE_ONE_AT_A_TIME;

    // Used in tests to override the result of checking for screen lock set-up. This allows the
    // tests to be independent of a particular device configuration.
    @OverrideState
    private static int sScreenLockSetUpOverride = OVERRIDE_STATE_NOT_OVERRIDDEN;

    // Used in tests to override the result of checking for availability of the screen-locking API.
    // This allows the tests to be independent of a particular device configuration.
    @OverrideState
    private static int sApiOverride = OVERRIDE_STATE_NOT_OVERRIDDEN;

    // Used in tests to avoid displaying the OS reauth dialog.
    private static boolean sSkipSystemReauth = false;

    /**
     * Clears the record of the last reauth so that a call to authenticationStillValid will return
     * false.
     */
    public static void resetLastReauth() {
        sLastReauthTimeMillis = null;
        sLastReauthScope = REAUTH_SCOPE_ONE_AT_A_TIME;
    }

    /**
     * Stores the timestamp of last reauthentication of the user.
     * @param timeStampMillis The time of the most recent successful user reauthentication.
     * @param scope The scope of the reauthentication as advertised to the user via UI.
     */
    public static void recordLastReauth(long timeStampMillis, @ReauthScope int scope) {
        sLastReauthTimeMillis = timeStampMillis;
        sLastReauthScope = scope;
    }

    @VisibleForTesting
    public static void setScreenLockSetUpOverride(@OverrideState int screenLockSetUpOverride) {
        sScreenLockSetUpOverride = screenLockSetUpOverride;
    }

    @VisibleForTesting
    public static void setApiOverride(@OverrideState int apiOverride) {
        // Ensure that tests don't accidentally try to launch the OS-provided lock screen.
        if (apiOverride == OVERRIDE_STATE_AVAILABLE) {
            PasswordReauthenticationFragment.preventLockingForTesting();
        }

        sApiOverride = apiOverride;
    }

    @VisibleForTesting
    public static void setSkipSystemReauth(boolean skipSystemReauth) {
        sSkipSystemReauth = skipSystemReauth;
    }

    /**
     * Checks whether reauthentication is available on the device at all.
     * @return The result of the check.
     */
    public static boolean isReauthenticationApiAvailable() {
        switch (sApiOverride) {
            case OVERRIDE_STATE_NOT_OVERRIDDEN:
                return Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP;
            case OVERRIDE_STATE_AVAILABLE:
                return true;
            case OVERRIDE_STATE_UNAVAILABLE:
                return false;
        }
        // This branch is not reachable.
        assert false;
        return false;
    }

    /**
     * Initiates the reauthentication prompt with a given description.
     *
     * @param descriptionId   The resource ID of the string to be displayed to explain the reason
     *                        for the reauthentication.
     * @param containerViewId The ID of the container, fragments of which will get replaced with the
     *                        reauthentication prompt. It may be equal to View.NO_ID in tests.
     * @param fragmentManager For putting the lock screen on the transaction stack.
     */
    public static void displayReauthenticationFragment(int descriptionId, int containerViewId,
            FragmentManager fragmentManager, @ReauthScope int scope) {
        if (sSkipSystemReauth) return;

        Fragment passwordReauthentication = new PasswordReauthenticationFragment();
        Bundle args = new Bundle();
        args.putInt(PasswordReauthenticationFragment.DESCRIPTION_ID, descriptionId);
        args.putInt(PasswordReauthenticationFragment.SCOPE_ID, scope);
        passwordReauthentication.setArguments(args);

        FragmentTransaction fragmentTransaction = fragmentManager.beginTransaction();
        if (containerViewId == View.NO_ID) {
            fragmentTransaction.add(passwordReauthentication, FRAGMENT_TAG);
        } else {
            fragmentTransaction.replace(containerViewId, passwordReauthentication, FRAGMENT_TAG);
        }
        fragmentTransaction.addToBackStack(null);
        fragmentTransaction.commit();
    }

    /** Checks whether authentication is recent enough to be valid. The authentication is valid as
     * long as the user authenticated less than {@code VALID_REAUTHENTICATION_TIME_INTERVAL_MILLIS}
     * milliseconds ago, for a scope including the passed {@code scope} argument. The {@code BULK}
     * scope includes the {@code ONE_AT_A_TIME} scope.
     * @param scope The scope the reauth should be valid for. */
    public static boolean authenticationStillValid(@ReauthScope int scope) {
        final boolean scopeIncluded =
                scope == sLastReauthScope || sLastReauthScope == REAUTH_SCOPE_BULK;
        return sLastReauthTimeMillis != null && scopeIncluded
                && (System.currentTimeMillis() - sLastReauthTimeMillis)
                < VALID_REAUTHENTICATION_TIME_INTERVAL_MILLIS;
    }

    /**
     * Checks whether the user set up screen lock so that it can be used for reauthentication. Can
     * be overridden in tests.
     * @param context The context to retrieve the KeyguardManager to find out.
     */
    public static boolean isScreenLockSetUp(Context context) {
        switch (sScreenLockSetUpOverride) {
            case OVERRIDE_STATE_NOT_OVERRIDDEN:
                return ((KeyguardManager) context.getSystemService(Context.KEYGUARD_SERVICE))
                        .isKeyguardSecure();
            case OVERRIDE_STATE_AVAILABLE:
                return true;
            case OVERRIDE_STATE_UNAVAILABLE:
                return false;
        }
        // This branch is not reachable.
        assert false;
        return false;
    }
}
