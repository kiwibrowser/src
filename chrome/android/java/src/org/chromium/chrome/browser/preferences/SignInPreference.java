// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.preferences;

import android.accounts.Account;
import android.content.Context;
import android.preference.Preference;
import android.preference.PreferenceFragment;
import android.support.annotation.Nullable;
import android.support.v7.content.res.AppCompatResources;
import android.util.AttributeSet;
import android.view.View;

import org.chromium.base.metrics.RecordUserAction;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.firstrun.FirstRunSignInProcessor;
import org.chromium.chrome.browser.signin.AccountManagementFragment;
import org.chromium.chrome.browser.signin.AccountSigninActivity;
import org.chromium.chrome.browser.signin.DisplayableProfileData;
import org.chromium.chrome.browser.signin.PersonalizedSigninPromoView;
import org.chromium.chrome.browser.signin.ProfileDataCache;
import org.chromium.chrome.browser.signin.SigninAccessPoint;
import org.chromium.chrome.browser.signin.SigninManager;
import org.chromium.chrome.browser.signin.SigninManager.SignInAllowedObserver;
import org.chromium.chrome.browser.signin.SigninPromoController;
import org.chromium.chrome.browser.sync.ProfileSyncService;
import org.chromium.chrome.browser.sync.ProfileSyncService.SyncStateChangedListener;
import org.chromium.chrome.browser.util.ViewUtils;
import org.chromium.components.signin.AccountManagerFacade;
import org.chromium.components.signin.AccountsChangeObserver;
import org.chromium.components.signin.ChromeSigninController;
import org.chromium.components.sync.AndroidSyncSettings;

import java.util.Collections;

/**
 * A preference that displays "Sign in to Chrome" when the user is not sign in, and displays
 * the user's name, email, profile image and sync error icon if necessary when the user is signed
 * in.
 */
public class SignInPreference
        extends Preference implements SignInAllowedObserver, ProfileDataCache.Observer,
                                      AndroidSyncSettings.AndroidSyncSettingsObserver,
                                      SyncStateChangedListener, AccountsChangeObserver {
    private boolean mWasGenericSigninPromoDisplayed;
    private boolean mViewEnabled;
    private @Nullable SigninPromoController mSigninPromoController;
    private final ProfileDataCache mProfileDataCache;

    /**
     * Constructor for inflating from XML.
     */
    public SignInPreference(Context context, AttributeSet attrs) {
        super(context, attrs);

        int imageSize = context.getResources().getDimensionPixelSize(R.dimen.user_picture_size);
        mProfileDataCache = new ProfileDataCache(context, imageSize);

        setOnPreferenceClickListener(preference
                -> AccountSigninActivity.startIfAllowed(getContext(), SigninAccessPoint.SETTINGS));
    }

    /**
     * Starts listening for updates to the sign-in and sync state.
     */
    void registerForUpdates() {
        AccountManagerFacade.get().addObserver(this);
        SigninManager.get().addSignInAllowedObserver(this);
        mProfileDataCache.addObserver(this);
        FirstRunSignInProcessor.updateSigninManagerFirstRunCheckDone();
        AndroidSyncSettings.registerObserver(getContext(), this);
        ProfileSyncService syncService = ProfileSyncService.get();
        if (syncService != null) {
            syncService.addSyncStateChangedListener(this);
        }

        update();
    }

    /**
     * Stops listening for updates to the sign-in and sync state. Every call to registerForUpdates()
     * must be matched with a call to this method.
     */
    void unregisterForUpdates() {
        AccountManagerFacade.get().removeObserver(this);
        SigninManager.get().removeSignInAllowedObserver(this);
        mProfileDataCache.removeObserver(this);
        AndroidSyncSettings.unregisterObserver(getContext(), this);
        ProfileSyncService syncService = ProfileSyncService.get();
        if (syncService != null) {
            syncService.removeSyncStateChangedListener(this);
        }
    }

    /**
     * Should be called when the {@link PreferenceFragment} which used {@link SignInPreference} gets
     * destroyed. Used to record "ImpressionsTilDismiss" histogram.
     */
    void onPreferenceFragmentDestroyed() {
        if (mSigninPromoController != null) {
            mSigninPromoController.onPromoDestroyed();
        }
    }

    /**
     * Updates the title, summary, and image based on the current sign-in state.
     */
    private void update() {
        if (SigninManager.get().isSigninDisabledByPolicy()) {
            setupSigninDisabled();
            return;
        }

        String accountName = ChromeSigninController.get().getSignedInAccountName();
        if (accountName != null) {
            setupSignedIn(accountName);
            return;
        }

        if (ChromePreferenceManager.getInstance().getSettingsPersonalizedSigninPromoDismissed()) {
            // Don't show the new promo if it was dismissed by the user.
            setupGenericPromo();
            return;
        }

        if (mSigninPromoController != null) {
            // Don't change the promo type if the new promo is already being shown.
            setupPersonalizedPromo();
            return;
        }

        if (SigninPromoController.hasNotReachedImpressionLimit(SigninAccessPoint.SETTINGS)) {
            setupPersonalizedPromo();
            return;
        }

        setupGenericPromo();
    }

    private void setupSigninDisabled() {
        setLayoutResource(R.layout.account_management_account_row);
        setTitle(R.string.sign_in_to_chrome);
        setSummary(R.string.sign_in_to_chrome_disabled_summary);
        setFragment(null);
        setIcon(ManagedPreferencesUtils.getManagedByEnterpriseIconId());
        setWidgetLayoutResource(0);
        setViewEnabled(false);
        mSigninPromoController = null;
        mWasGenericSigninPromoDisplayed = false;
    }

    private void setupPersonalizedPromo() {
        setLayoutResource(R.layout.personalized_signin_promo_view_settings);
        setTitle("");
        setSummary("");
        setFragment(null);
        setIcon(null);
        setWidgetLayoutResource(0);
        setViewEnabled(true);

        if (mSigninPromoController == null) {
            mSigninPromoController = new SigninPromoController(SigninAccessPoint.SETTINGS);
        }

        mWasGenericSigninPromoDisplayed = false;
        notifyChanged();
    }

    private void setupGenericPromo() {
        setLayoutResource(R.layout.account_management_account_row);
        setTitle(R.string.sign_in_to_chrome);
        setSummary(R.string.sign_in_to_chrome_summary);
        setFragment(null);
        setIcon(AppCompatResources.getDrawable(getContext(), R.drawable.logo_avatar_anonymous));
        setWidgetLayoutResource(0);
        setViewEnabled(true);
        mSigninPromoController = null;

        if (!mWasGenericSigninPromoDisplayed) {
            RecordUserAction.record("Signin_Impression_FromSettings");
        }

        mWasGenericSigninPromoDisplayed = true;
    }

    private void setupSignedIn(String accountName) {
        mProfileDataCache.update(Collections.singletonList(accountName));
        DisplayableProfileData profileData = mProfileDataCache.getProfileDataOrDefault(accountName);

        setLayoutResource(R.layout.account_management_account_row);
        setTitle(profileData.getFullNameOrEmail());
        setSummary(SyncPreference.getSyncStatusSummary(getContext()));
        setFragment(AccountManagementFragment.class.getName());
        setIcon(profileData.getImage());
        setWidgetLayoutResource(
                SyncPreference.showSyncErrorIcon(getContext()) ? R.layout.sync_error_widget : 0);
        setViewEnabled(true);

        mSigninPromoController = null;
        mWasGenericSigninPromoDisplayed = false;
    }

    // This just changes visual representation. Actual enabled flag in preference stays
    // always true to receive clicks (necessary to show "Managed by administrator" toast).
    private void setViewEnabled(boolean enabled) {
        if (mViewEnabled == enabled) {
            return;
        }
        mViewEnabled = enabled;
        notifyChanged();
    }

    @Override
    protected void onBindView(final View view) {
        super.onBindView(view);
        ViewUtils.setEnabledRecursive(view, mViewEnabled);

        if (mSigninPromoController == null) {
            return;
        }

        DisplayableProfileData profileData = null;
        Account[] accounts = AccountManagerFacade.get().tryGetGoogleAccounts();
        if (accounts.length > 0) {
            String defaultAccountName = accounts[0].name;
            mProfileDataCache.update(Collections.singletonList(defaultAccountName));
            profileData = mProfileDataCache.getProfileDataOrDefault(defaultAccountName);
        }
        PersonalizedSigninPromoView signinPromoView =
                view.findViewById(R.id.signin_promo_view_container);
        mSigninPromoController.detach();
        mSigninPromoController.setupPromoView(getContext(), signinPromoView, profileData, () -> {
            ChromePreferenceManager.getInstance().setSettingsPersonalizedSigninPromoDismissed(true);
            update();
        });
    }

    // ProfileSyncServiceListener implementation.
    @Override
    public void syncStateChanged() {
        update();
    }

    // SignInAllowedObserver implementation.
    @Override
    public void onSignInAllowedChanged() {
        update();
    }

    // ProfileDataCache.Observer implementation.
    @Override
    public void onProfileDataUpdated(String accountId) {
        update();
    }

    // AndroidSyncSettings.AndroidSyncSettingsObserver implementation.
    @Override
    public void androidSyncSettingsChanged() {
        update();
    }

    // AccountsChangeObserver implementation.
    @Override
    public void onAccountsChanged() {
        update();
    }
}
