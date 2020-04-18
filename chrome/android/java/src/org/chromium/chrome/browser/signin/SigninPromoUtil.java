// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.signin;

import android.app.Activity;
import android.text.TextUtils;

import org.chromium.base.VisibleForTesting;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.chrome.browser.ChromeVersionInfo;
import org.chromium.chrome.browser.preferences.ChromePreferenceManager;
import org.chromium.chrome.browser.preferences.PrefServiceBridge;
import org.chromium.components.signin.AccountManagerFacade;
import org.chromium.components.signin.ChromeSigninController;
import org.chromium.ui.base.WindowAndroid;

import java.util.HashSet;
import java.util.Set;

/**
* Helper functions for promoting sign in.
*/
public class SigninPromoUtil {
    private SigninPromoUtil() {}

    /**
     * Launches the signin promo if it needs to be displayed.
     * @param activity The parent activity.
     * @return Whether the signin promo is shown.
     */
    public static boolean launchSigninPromoIfNeeded(final Activity activity) {
        ChromePreferenceManager preferenceManager = ChromePreferenceManager.getInstance();
        int currentMajorVersion = ChromeVersionInfo.getProductMajorVersion();
        boolean wasSignedIn =
                TextUtils.isEmpty(PrefServiceBridge.getInstance().getSyncLastAccountName());
        HashSet<String> accountNames =
                new HashSet<>(AccountManagerFacade.get().tryGetGoogleAccountNames());
        if (!shouldLaunchSigninPromo(preferenceManager, currentMajorVersion,
                    ChromeSigninController.get().isSignedIn(), wasSignedIn, accountNames)) {
            return false;
        }

        AccountSigninActivity.startIfAllowed(activity, SigninAccessPoint.SIGNIN_PROMO);
        preferenceManager.setSigninPromoLastShownVersion(currentMajorVersion);
        preferenceManager.setSigninPromoLastAccountNames(accountNames);
        return true;
    }

    /**
     * Launches the signin promo if it needs to be displayed.
     * @param preferenceManager the preference manager to get preferences
     * @param currentMajorVersion the current major version of Chrome
     * @param isSignedIn is user currently signed in
     * @param wasSignedIn has used manually signed out
     * @param accountNames the set of account names currently on device
     * @return Whether the signin promo should be shown.
     */
    @VisibleForTesting
    static boolean shouldLaunchSigninPromo(ChromePreferenceManager preferenceManager,
            int currentMajorVersion, boolean isSignedIn, boolean wasSignedIn,
            Set<String> accountNames) {
        int lastPromoMajorVersion = preferenceManager.getSigninPromoLastShownVersion();
        if (lastPromoMajorVersion == 0) {
            preferenceManager.setSigninPromoLastShownVersion(currentMajorVersion);
            return false;
        }

        // Don't show if user is signed in or there are no Google accounts on the device.
        if (isSignedIn) return false;
        if (accountNames.isEmpty()) return false;

        // Don't show if user has manually signed out.
        if (wasSignedIn) return false;

        // Promo can be shown at most once every 2 Chrome major versions.
        if (currentMajorVersion < lastPromoMajorVersion + 2) return false;

        // Don't show if no new accounts have been added after the last time promo was shown.
        Set<String> previousAccountNames = preferenceManager.getSigninPromoLastAccountNames();
        return previousAccountNames == null || !previousAccountNames.containsAll(accountNames);
    }

    /**
     * A convenience method to create an AccountSigninActivity, passing the access point as an
     * intent extra.
     * @param window WindowAndroid from which to get the Activity/Context.
     * @param accessPoint for metrics purposes.
     */
    @CalledByNative
    private static void openAccountSigninActivityForPromo(WindowAndroid window, int accessPoint) {
        Activity activity = window.getActivity().get();
        if (activity != null) {
            AccountSigninActivity.startIfAllowed(activity, accessPoint);
        }
    }
}
