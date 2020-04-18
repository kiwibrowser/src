// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.preferences;

import android.content.Context;
import android.preference.Preference;
import android.support.annotation.Nullable;
import android.view.View;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.util.ViewUtils;
import org.chromium.ui.widget.Toast;

/**
 * Utilities and common methods to handle settings managed by policies.
 */
public class ManagedPreferencesUtils {

    /**
     * Shows a toast indicating that the previous action is managed by the system administrator.
     *
     * This is usually used to explain to the user why a given control is disabled in the settings.
     *
     * @param context The context where the Toast will be shown.
     */
    public static void showManagedByAdministratorToast(Context context) {
        Toast.makeText(context, context.getString(R.string.managed_by_your_administrator),
                Toast.LENGTH_LONG).show();
    }
    /**
     * Shows a toast indicating that the previous action is managed by the parent(s) of the
     * supervised user.
     * This is usually used to explain to the user why a given control is disabled in the settings.
     *
     * @param context The context where the Toast will be shown.
     */
    public static void showManagedByParentToast(Context context) {
        boolean singleParentIsManager =
                PrefServiceBridge.getInstance().getSupervisedUserSecondCustodianName().isEmpty();
        Toast.makeText(context, context.getString(singleParentIsManager
                ? R.string.managed_by_your_parent : R.string.managed_by_your_parents),
                Toast.LENGTH_LONG).show();
    }

    /**
     * @return the resource ID for the Managed By Enterprise icon.
     */
    public static int getManagedByEnterpriseIconId() {
        return R.drawable.controlled_setting_mandatory;
    }

    /**
     * Initializes the Preference based on the state of any policies that may affect it,
     * e.g. by showing a managed icon or disabling clicks on the preference.
     *
     * This should be called once, before the preference is displayed.
     *
     * @param delegate The delegate that controls whether the preference is managed. May be null,
     *         then this method does nothing.
     * @param preference The Preference that is being initialized
     */
    public static void initPreference(
            @Nullable ManagedPreferenceDelegate delegate, Preference preference) {
        if (delegate == null) return;

        if (delegate.isPreferenceControlledByPolicy(preference)) {
            preference.setIcon(getManagedByEnterpriseIconId());
        } else if (delegate.isPreferenceControlledByCustodian(preference)) {
            preference.setIcon(R.drawable.ic_account_child_grey600_36dp);
        }

        if (delegate.isPreferenceClickDisabledByPolicy(preference)) {
            // Disable the views and prevent the Preference from mucking with the enabled state.
            preference.setShouldDisableView(false);

            // Prevent default click behavior.
            preference.setFragment(null);
            preference.setIntent(null);
            preference.setOnPreferenceClickListener(null);
        }
    }

    /**
     * Disables the Preference's views if the preference is not clickable.
     *
     * Note: this disables the View instead of disabling the Preference, so that the Preference
     * still receives click events, which will trigger a "Managed by your administrator" toast.
     *
     * This should be called from the Preference's onBindView() method.
     *
     * @param delegate The delegate that controls whether the preference is managed. May be null,
     *         then this method does nothing.
     * @param preference The Preference that owns the view
     * @param view The View that was bound to the Preference
     */
    public static void onBindViewToPreference(
            @Nullable ManagedPreferenceDelegate delegate, Preference preference, View view) {
        if (delegate != null && delegate.isPreferenceClickDisabledByPolicy(preference)) {
            ViewUtils.setEnabledRecursive(view, false);
        }
    }

    /**
     * Intercepts the click event if the given Preference is managed and shows a toast in that case.
     *
     * This should be called from the Preference's onClick() method.
     *
     * @param delegate The delegate that controls whether the preference is managed. May be null,
     *         then this method does nothing and returns false.
     * @param preference The Preference that was clicked.
     * @return true if the click event was handled by this helper and shouldn't be further
     *         propagated; false otherwise.
     */
    public static boolean onClickPreference(
            @Nullable ManagedPreferenceDelegate delegate, Preference preference) {
        if (delegate == null || !delegate.isPreferenceClickDisabledByPolicy(preference)) {
            return false;
        }

        if (delegate.isPreferenceControlledByPolicy(preference)) {
            showManagedByAdministratorToast(preference.getContext());
        } else if (delegate.isPreferenceControlledByCustodian(preference)) {
            showManagedByParentToast(preference.getContext());
        } else {
            // If the preference is disabled, it should be either because it's managed by enterprise
            // policy or by the custodian.
            assert false;
        }
        return true;
    }
}
