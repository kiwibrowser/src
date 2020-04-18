// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.bookmarks;

import android.accounts.Account;
import android.content.Context;
import android.content.SharedPreferences;
import android.support.annotation.IntDef;
import android.support.annotation.Nullable;
import android.support.v7.widget.RecyclerView;
import android.support.v7.widget.RecyclerView.ViewHolder;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import org.chromium.base.ContextUtils;
import org.chromium.base.VisibleForTesting;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.signin.DisplayableProfileData;
import org.chromium.chrome.browser.signin.PersonalizedSigninPromoView;
import org.chromium.chrome.browser.signin.ProfileDataCache;
import org.chromium.chrome.browser.signin.SigninAccessPoint;
import org.chromium.chrome.browser.signin.SigninManager;
import org.chromium.chrome.browser.signin.SigninManager.SignInStateObserver;
import org.chromium.chrome.browser.signin.SigninPromoController;
import org.chromium.chrome.browser.signin.SyncPromoView;
import org.chromium.components.signin.AccountManagerFacade;
import org.chromium.components.signin.AccountsChangeObserver;
import org.chromium.components.signin.ChromeSigninController;
import org.chromium.components.sync.AndroidSyncSettings;
import org.chromium.components.sync.AndroidSyncSettings.AndroidSyncSettingsObserver;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.util.Collections;

/**
 * Class that manages all the logic and UI behind the signin promo header in the bookmark
 * content UI. The header is shown only on certain situations, (e.g., not signed in).
 */
class BookmarkPromoHeader implements AndroidSyncSettingsObserver, SignInStateObserver,
                                     ProfileDataCache.Observer, AccountsChangeObserver {
    /**
     * Specifies the various states in which the Bookmarks promo can be.
     */
    @Retention(RetentionPolicy.SOURCE)
    @IntDef({PromoState.PROMO_NONE, PromoState.PROMO_SIGNIN_PERSONALIZED, PromoState.PROMO_SYNC})
    @interface PromoState {
        int PROMO_NONE = 0;
        int PROMO_SIGNIN_PERSONALIZED = 1;
        int PROMO_SYNC = 2;
    }

    // Personalized signin promo preference.
    private static final String PREF_PERSONALIZED_SIGNIN_PROMO_DECLINED =
            "signin_promo_bookmarks_declined";
    // Generic signin and sync promo preferences.
    private static final String PREF_SIGNIN_AND_SYNC_PROMO_SHOW_COUNT =
            "enhanced_bookmark_signin_promo_show_count";
    // TODO(kkimlabs): Figure out the optimal number based on UMA data.
    private static final int MAX_SIGNIN_AND_SYNC_PROMO_SHOW_COUNT = 10;

    private static @Nullable @PromoState Integer sPromoStateForTests;

    private final Context mContext;
    private final SigninManager mSignInManager;
    private final Runnable mPromoHeaderChangeAction;

    private final @Nullable ProfileDataCache mProfileDataCache;
    private final @Nullable SigninPromoController mSigninPromoController;
    private @PromoState int mPromoState;

    /**
     * Initializes the class. Note that this will start listening to signin related events and
     * update itself if needed.
     */
    BookmarkPromoHeader(Context context, Runnable promoHeaderChangeAction) {
        mContext = context;
        mPromoHeaderChangeAction = promoHeaderChangeAction;

        AndroidSyncSettings.registerObserver(mContext, this);

        if (SigninPromoController.hasNotReachedImpressionLimit(
                    SigninAccessPoint.BOOKMARK_MANAGER)) {
            int imageSize =
                    mContext.getResources().getDimensionPixelSize(R.dimen.user_picture_size);
            mProfileDataCache = new ProfileDataCache(mContext, imageSize);
            mProfileDataCache.addObserver(this);
            mSigninPromoController = new SigninPromoController(SigninAccessPoint.BOOKMARK_MANAGER);
            AccountManagerFacade.get().addObserver(this);
        } else {
            mProfileDataCache = null;
            mSigninPromoController = null;
        }

        mSignInManager = SigninManager.get();
        mSignInManager.addSignInStateObserver(this);

        mPromoState = calculatePromoState();
        if (mPromoState == PromoState.PROMO_SYNC) {
            int promoShowCount = ContextUtils.getAppSharedPreferences().getInt(
                    PREF_SIGNIN_AND_SYNC_PROMO_SHOW_COUNT, 0);
            ContextUtils.getAppSharedPreferences()
                    .edit()
                    .putInt(PREF_SIGNIN_AND_SYNC_PROMO_SHOW_COUNT, promoShowCount + 1)
                    .apply();
        }
    }

    /**
     * Clean ups the class. Must be called once done using this class.
     */
    void destroy() {
        AndroidSyncSettings.unregisterObserver(mContext, this);

        if (mSigninPromoController != null) {
            AccountManagerFacade.get().removeObserver(this);
            mProfileDataCache.removeObserver(this);
            mSigninPromoController.onPromoDestroyed();
        }

        mSignInManager.removeSignInStateObserver(this);
    }

    /**
     * @return The current state of the promo.
     */
    @PromoState
    int getPromoState() {
        return mPromoState;
    }

    /**
     * @return Personalized signin promo header {@link ViewHolder} instance that can be used with
     *         {@link RecyclerView}.
     */
    ViewHolder createPersonalizedSigninPromoHolder(ViewGroup parent) {
        View view = LayoutInflater.from(mContext).inflate(
                R.layout.personalized_signin_promo_view_bookmarks, parent, false);

        // ViewHolder is abstract and it cannot be instantiated directly.
        return new ViewHolder(view) {};
    }

    /**
     * @return Sync promo header {@link ViewHolder} instance that can be used with
     *         {@link RecyclerView}.
     */
    ViewHolder createSyncPromoHolder(ViewGroup parent) {
        SyncPromoView view = SyncPromoView.create(parent, SigninAccessPoint.BOOKMARK_MANAGER);

        // ViewHolder is abstract and it cannot be instantiated directly.
        return new ViewHolder(view) {};
    }

    /**
     * Configures the personalized signin promo and records promo impressions.
     * @param view The view to be configured.
     */
    void setupPersonalizedSigninPromo(PersonalizedSigninPromoView view) {
        DisplayableProfileData profileData = null;
        Account[] accounts = AccountManagerFacade.get().tryGetGoogleAccounts();
        if (accounts.length > 0) {
            String defaultAccountName = accounts[0].name;
            mProfileDataCache.update(Collections.singletonList(defaultAccountName));
            profileData = mProfileDataCache.getProfileDataOrDefault(defaultAccountName);
        }
        SigninPromoController.OnDismissListener listener = this::setPersonalizedSigninPromoDeclined;
        mSigninPromoController.setupPromoView(mContext, view, profileData, listener);
    }

    /**
     * Detaches the previously configured {@link PersonalizedSigninPromoView}.
     */
    void detachPersonalizePromoView() {
        if (mSigninPromoController != null) mSigninPromoController.detach();
    }

    /**
     * Saves that the personalized signin promo was declined and updates the UI.
     */
    private void setPersonalizedSigninPromoDeclined() {
        SharedPreferences.Editor sharedPreferencesEditor =
                ContextUtils.getAppSharedPreferences().edit();
        sharedPreferencesEditor.putBoolean(PREF_PERSONALIZED_SIGNIN_PROMO_DECLINED, true);
        sharedPreferencesEditor.apply();
        mPromoState = calculatePromoState();
        triggerPromoUpdate();
    }

    /**
     * @return Whether the user declined the personalized signin promo.
     */
    private boolean wasPersonalizedSigninPromoDeclined() {
        return ContextUtils.getAppSharedPreferences().getBoolean(
                PREF_PERSONALIZED_SIGNIN_PROMO_DECLINED, false);
    }

    private @PromoState int calculatePromoState() {
        if (sPromoStateForTests != null) {
            return sPromoStateForTests;
        }

        if (!AndroidSyncSettings.isMasterSyncEnabled(mContext)) {
            return PromoState.PROMO_NONE;
        }

        if (!ChromeSigninController.get().isSignedIn()) {
            boolean impressionLimitReached = !SigninPromoController.hasNotReachedImpressionLimit(
                    SigninAccessPoint.BOOKMARK_MANAGER);
            if (!mSignInManager.isSignInAllowed() || impressionLimitReached
                    || wasPersonalizedSigninPromoDeclined()) {
                return PromoState.PROMO_NONE;
            }
            return PromoState.PROMO_SIGNIN_PERSONALIZED;
        }

        boolean impressionLimitNotReached = ContextUtils.getAppSharedPreferences().getInt(
                                                    PREF_SIGNIN_AND_SYNC_PROMO_SHOW_COUNT, 0)
                < MAX_SIGNIN_AND_SYNC_PROMO_SHOW_COUNT;
        if (!AndroidSyncSettings.isChromeSyncEnabled(mContext) && impressionLimitNotReached) {
            return PromoState.PROMO_SYNC;
        }
        return PromoState.PROMO_NONE;
    }

    // AndroidSyncSettingsObserver implementation.
    @Override
    public void androidSyncSettingsChanged() {
        mPromoState = calculatePromoState();
        triggerPromoUpdate();
    }

    // SignInStateObserver implementation.
    @Override
    public void onSignedIn() {
        mPromoState = calculatePromoState();
        triggerPromoUpdate();
    }

    @Override
    public void onSignedOut() {
        mPromoState = calculatePromoState();
        triggerPromoUpdate();
    }

    // ProfileDataCache.Observer implementation.
    @Override
    public void onProfileDataUpdated(String accountId) {
        triggerPromoUpdate();
    }

    // AccountsChangeObserver implementation.
    @Override
    public void onAccountsChanged() {
        triggerPromoUpdate();
    }

    private void triggerPromoUpdate() {
        detachPersonalizePromoView();
        mPromoHeaderChangeAction.run();
    }

    /**
     * Forces the promo state to a particular value for testing purposes.
     * @param promoState The promo state to which the header will be set to.
     */
    @VisibleForTesting
    public static void forcePromoStateForTests(@PromoState int promoState) {
        sPromoStateForTests = promoState;
    }
}
