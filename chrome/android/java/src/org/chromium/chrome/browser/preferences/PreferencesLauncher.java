// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.preferences;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;

import org.chromium.base.ContextUtils;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.chrome.browser.preferences.autofill.AutofillAndPaymentsPreferences;
import org.chromium.chrome.browser.preferences.password.SavePasswordsPreferences;
import org.chromium.chrome.browser.preferences.privacy.ClearBrowsingDataTabsFragment;
import org.chromium.content_public.browser.WebContents;

import java.lang.ref.WeakReference;

/**
 * A utility class for launching Chrome Settings.
 */
public class PreferencesLauncher {
    private static final String TAG = "PreferencesLauncher";

    /**
     * Launches settings, either on the top-level page or on a subpage.
     *
     * @param context The current Activity, or an application context if no Activity is available.
     * @param fragmentName The name of the fragment to show, or null to show the top-level page.
     */
    public static void launchSettingsPage(Context context, String fragmentName) {
        Intent intent = createIntentForSettingsPage(context, fragmentName);
        context.startActivity(intent);
    }

    /**
     * Creates an intent for launching settings, either on the top-level settings page or a specific
     * subpage.
     *
     * @param context The current Activity, or an application context if no Activity is available.
     * @param fragmentName The name of the fragment to show, or null to show the top-level page.
     */
    public static Intent createIntentForSettingsPage(Context context, String fragmentName) {
        Intent intent = new Intent();
        intent.setClass(context, Preferences.class);
        if (!(context instanceof Activity)) {
            intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            intent.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
        }
        if (fragmentName != null) {
            intent.putExtra(Preferences.EXTRA_SHOW_FRAGMENT, fragmentName);
        }
        return intent;
    }

    /**
     * Creates an intent for launching clear browsing data, either on the top-level settings page or
     * a specific subpage.
     *
     * @param context The current Activity, or an application context if no Activity is available.
     */
    public static Intent createIntentForClearBrowsingDataPage(Context context) {
        return createIntentForSettingsPage(context, ClearBrowsingDataTabsFragment.class.getName());
    }

    @CalledByNative
    private static void showAutofillSettings(WebContents webContents) {
        WeakReference<Activity> currentActivity =
                webContents.getTopLevelNativeWindow().getActivity();

        launchSettingsPage(currentActivity.get(),
                AutofillAndPaymentsPreferences.class.getName());
    }

    @CalledByNative
    private static void showPasswordSettings() {
        launchSettingsPage(
                ContextUtils.getApplicationContext(), SavePasswordsPreferences.class.getName());
    }
}
