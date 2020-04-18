// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.ntp.cards;

import android.accounts.Account;
import android.content.Context;
import android.support.annotation.DrawableRes;
import android.support.annotation.Nullable;
import android.support.annotation.StringRes;

import org.chromium.base.Callback;
import org.chromium.base.ContextUtils;
import org.chromium.base.VisibleForTesting;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ntp.ContextMenuManager;
import org.chromium.chrome.browser.ntp.snippets.CategoryInt;
import org.chromium.chrome.browser.ntp.snippets.CategoryStatus;
import org.chromium.chrome.browser.ntp.snippets.SnippetsBridge;
import org.chromium.chrome.browser.ntp.snippets.SuggestionsSource;
import org.chromium.chrome.browser.preferences.ChromePreferenceManager;
import org.chromium.chrome.browser.signin.DisplayableProfileData;
import org.chromium.chrome.browser.signin.PersonalizedSigninPromoView;
import org.chromium.chrome.browser.signin.ProfileDataCache;
import org.chromium.chrome.browser.signin.SigninAccessPoint;
import org.chromium.chrome.browser.signin.SigninManager;
import org.chromium.chrome.browser.signin.SigninManager.SignInAllowedObserver;
import org.chromium.chrome.browser.signin.SigninManager.SignInStateObserver;
import org.chromium.chrome.browser.signin.SigninPromoController;
import org.chromium.chrome.browser.suggestions.DestructionObserver;
import org.chromium.chrome.browser.suggestions.SuggestionsConfig;
import org.chromium.chrome.browser.suggestions.SuggestionsRecyclerView;
import org.chromium.chrome.browser.suggestions.SuggestionsUiDelegate;
import org.chromium.chrome.browser.util.FeatureUtilities;
import org.chromium.chrome.browser.widget.displaystyle.UiConfig;
import org.chromium.components.signin.AccountManagerFacade;
import org.chromium.components.signin.AccountsChangeObserver;

import java.util.Collections;
import java.util.concurrent.TimeUnit;

/**
 * Shows a card prompting the user to sign in. This item is also an {@link OptionalLeaf}, and sign
 * in state changes control its visibility.
 */
public class SignInPromo extends OptionalLeaf {
    /**
     * Period for which promos are suppressed if signin is refused in FRE.
     */
    @VisibleForTesting
    static final long SUPPRESSION_PERIOD_MS = TimeUnit.DAYS.toMillis(1);

    private static boolean sDisablePromoForTests;

    /**
     * Whether the promo has been dismissed by the user.
     */
    private boolean mDismissed;

    /**
     * Whether the signin status means that the user has the possibility to sign in.
     */
    private boolean mCanSignIn;

    /**
     * Whether personalized suggestions can be shown. If it's not the case, we have no reason to
     * offer the user to sign in.
     */
    private boolean mCanShowPersonalizedSuggestions;

    private final @Nullable SigninObserver mSigninObserver;
    private final @Nullable SigninPromoController mSigninPromoController;
    private final @Nullable ProfileDataCache mProfileDataCache;

    private SignInPromo(SuggestionsUiDelegate uiDelegate) {
        Context context = ContextUtils.getApplicationContext();
        SuggestionsSource suggestionsSource = uiDelegate.getSuggestionsSource();
        SigninManager signinManager = SigninManager.get();

        mCanSignIn = signinManager.isSignInAllowed() && !signinManager.isSignedInOnNative();
        mCanShowPersonalizedSuggestions = suggestionsSource.areRemoteSuggestionsEnabled();

        updateVisibility();

        int imageSize = context.getResources().getDimensionPixelSize(R.dimen.user_picture_size);
        mProfileDataCache = new ProfileDataCache(context, imageSize);
        mSigninPromoController =
                new SigninPromoController(SigninAccessPoint.NTP_CONTENT_SUGGESTIONS);

        mSigninObserver = new SigninObserver(signinManager, suggestionsSource);
        uiDelegate.addDestructionObserver(mSigninObserver);
    }

    /**
     * Suppress signin promos in New Tab Page for {@link SignInPromo#SUPPRESSION_PERIOD_MS}. This
     * will not affect promos that were created before this call.
     */
    public static void temporarilySuppressPromos() {
        ChromePreferenceManager.getInstance().setNewTabPageSigninPromoSuppressionPeriodStart(
                System.currentTimeMillis());
    }

    public static SignInPromo maybeCreatePromo(SuggestionsUiDelegate uiDelegate) {
        if (sDisablePromoForTests) return null;
        if (ChromePreferenceManager.getInstance().getNewTabPageSigninPromoDismissed()) return null;
        if (getSuppressionStatus()) return null;
        return new SignInPromo(uiDelegate);
    }

    private static boolean getSuppressionStatus() {
        long suppressedFrom = ChromePreferenceManager.getInstance()
                                      .getNewTabPageSigninPromoSuppressionPeriodStart();
        if (suppressedFrom == 0) return false;
        long currentTime = System.currentTimeMillis();
        long suppressedTo = suppressedFrom + SUPPRESSION_PERIOD_MS;
        if (suppressedFrom <= currentTime && currentTime < suppressedTo) {
            return true;
        }
        ChromePreferenceManager.getInstance().clearNewTabPageSigninPromoSuppressionPeriodStart();
        return false;
    }

    @Override
    @ItemViewType
    protected int getItemViewType() {
        return ItemViewType.PROMO;
    }

    /**
     * @return a {@link DestructionObserver} observer that updates the visibility of the signin
     * promo and unregisters itself when the New Tab Page is destroyed.
     */
    @Nullable
    public DestructionObserver getObserver() {
        return mSigninObserver;
    }

    /**
     * @return a {@link NewTabPageViewHolder} which will contain the view for the signin promo.
     */
    public NewTabPageViewHolder createViewHolder(SuggestionsRecyclerView parent,
            ContextMenuManager contextMenuManager, UiConfig config) {
        return new PersonalizedPromoViewHolder(
                parent, config, contextMenuManager, mProfileDataCache, mSigninPromoController);
    }

    @Override
    protected void onBindViewHolder(NewTabPageViewHolder holder) {
        ((PersonalizedPromoViewHolder) holder).onBindViewHolder();
    }

    @Override
    protected void visitOptionalItem(NodeVisitor visitor) {
        visitor.visitSignInPromo();
    }

    private void updateVisibility() {
        setVisibilityInternal(!mDismissed && mCanSignIn && mCanShowPersonalizedSuggestions);
    }

    @Override
    protected boolean canBeDismissed() {
        return true;
    }

    /** Hides the sign in promo and sets a preference to make sure it is not shown again. */
    @Override
    public void dismiss(Callback<String> itemRemovedCallback) {
        mDismissed = true;
        updateVisibility();

        final @StringRes int promoHeader;
        ChromePreferenceManager.getInstance().setNewTabPageSigninPromoDismissed(true);
        promoHeader = mSigninPromoController.getDescriptionStringId();

        mSigninObserver.unregister();
        itemRemovedCallback.onResult(ContextUtils.getApplicationContext().getString(promoHeader));
    }

    @VisibleForTesting
    public static void setDisablePromoForTests(boolean disable) {
        sDisablePromoForTests = disable;
    }

    @VisibleForTesting
    class SigninObserver extends SuggestionsSource.EmptyObserver
            implements SignInStateObserver, SignInAllowedObserver, DestructionObserver,
                       ProfileDataCache.Observer, AccountsChangeObserver {
        private final SigninManager mSigninManager;
        private final SuggestionsSource mSuggestionsSource;

        /** Guards {@link #unregister()}, which can be called multiple times. */
        private boolean mUnregistered;

        private SigninObserver(SigninManager signinManager, SuggestionsSource suggestionsSource) {
            mSigninManager = signinManager;
            mSigninManager.addSignInAllowedObserver(this);
            mSigninManager.addSignInStateObserver(this);

            mSuggestionsSource = suggestionsSource;
            mSuggestionsSource.addObserver(this);

            mProfileDataCache.addObserver(this);
            AccountManagerFacade.get().addObserver(this);
        }

        private void unregister() {
            if (mUnregistered) return;
            mUnregistered = true;

            mSigninManager.removeSignInAllowedObserver(this);
            mSigninManager.removeSignInStateObserver(this);

            mSuggestionsSource.removeObserver(this);

            mProfileDataCache.removeObserver(this);
            AccountManagerFacade.get().removeObserver(this);
        }

        // DestructionObserver implementation.
        @Override
        public void onDestroy() {
            unregister();
        }

        // SignInAllowedObserver implementation.
        @Override
        public void onSignInAllowedChanged() {
            // Listening to onSignInAllowedChanged is important for the FRE. Sign in is not allowed
            // until it is completed, but the NTP is initialised before the FRE is even shown. By
            // implementing this we can show the promo if the user did not sign in during the FRE.
            mCanSignIn = mSigninManager.isSignInAllowed();
            updateVisibility();
        }

        // SignInStateObserver implementation.
        @Override
        public void onSignedIn() {
            mCanSignIn = false;
            updateVisibility();
        }

        @Override
        public void onSignedOut() {
            mCanSignIn = mSigninManager.isSignInAllowed();
            updateVisibility();
        }

        @Override
        public void onCategoryStatusChanged(
                @CategoryInt int category, @CategoryStatus int newStatus) {
            if (!SnippetsBridge.isCategoryRemote(category)) return;

            // Checks whether the category is enabled first to avoid unnecessary calls across JNI.
            mCanShowPersonalizedSuggestions = SnippetsBridge.isCategoryEnabled(category)
                    || mSuggestionsSource.areRemoteSuggestionsEnabled();
            updateVisibility();
        }

        // AccountsChangeObserver implementation.
        @Override
        public void onAccountsChanged() {
            notifyPersonalizedPromoIfVisible();
        }

        // ProfileDataCache.Observer implementation.
        @Override
        public void onProfileDataUpdated(String accountId) {
            notifyPersonalizedPromoIfVisible();
        }

        private void notifyPersonalizedPromoIfVisible() {
            if (isVisible()) notifyItemChanged(0, PersonalizedPromoViewHolder::update);
        }
    }

    /**
     * View Holder for {@link SignInPromo} if the personalized promo is to be shown.
     */
    @VisibleForTesting
    public static class PersonalizedPromoViewHolder extends CardViewHolder {
        private final ProfileDataCache mProfileDataCache;
        private final SigninPromoController mSigninPromoController;

        public PersonalizedPromoViewHolder(SuggestionsRecyclerView parent, UiConfig config,
                ContextMenuManager contextMenuManager, ProfileDataCache profileDataCache,
                SigninPromoController signinPromoController) {
            super(SuggestionsConfig.useModernLayout()
                            ? R.layout.personalized_signin_promo_view_modern_content_suggestions
                            : R.layout.personalized_signin_promo_view_ntp_content_suggestions,
                    parent, config, contextMenuManager);
            if (!FeatureUtilities.isChromeModernDesignEnabled()) {
                getParams().topMargin = parent.getResources().getDimensionPixelSize(
                        R.dimen.ntp_sign_in_promo_margin_top);
            }

            mProfileDataCache = profileDataCache;
            mSigninPromoController = signinPromoController;
        }

        @Override
        public void onBindViewHolder() {
            super.onBindViewHolder();
            bindPersonalizedSigninPromo();
        }

        @Override
        public void recycle() {
            mSigninPromoController.detach();
            super.recycle();
        }

        @DrawableRes
        @Override
        protected int selectBackground(boolean hasCardAbove, boolean hasCardBelow) {
            // Modern does not update the card background.
            assert !SuggestionsConfig.useModernLayout();
            return R.drawable.ntp_signin_promo_card_single;
        }

        /**
         * Triggers an update of the personalized signin promo. Intended to be used as
         * {@link NewTabPageViewHolder.PartialBindCallback}.
         */
        public static void update(NewTabPageViewHolder viewHolder) {
            ((PersonalizedPromoViewHolder) viewHolder).updatePersonalizedSigninPromo();
        }

        private void updatePersonalizedSigninPromo() {
            mSigninPromoController.detach();
            bindPersonalizedSigninPromo();
        }

        private void bindPersonalizedSigninPromo() {
            DisplayableProfileData profileData = null;
            Account[] accounts = AccountManagerFacade.get().tryGetGoogleAccounts();
            if (accounts.length > 0) {
                String defaultAccountName = accounts[0].name;
                mProfileDataCache.update(Collections.singletonList(defaultAccountName));
                profileData = mProfileDataCache.getProfileDataOrDefault(defaultAccountName);
            }
            PersonalizedSigninPromoView view = (PersonalizedSigninPromoView) itemView;
            mSigninPromoController.setupPromoView(view.getContext(), view, profileData, null);
        }

        /**
         * Binds the view and sets the profile data directly. Used for testing purposes.
         * @param profileData The profile data which will be used to configure the personalized
         *         signin promo.
         */
        @VisibleForTesting
        public void bindAndConfigureViewForTests(@Nullable DisplayableProfileData profileData) {
            super.onBindViewHolder();
            PersonalizedSigninPromoView view = (PersonalizedSigninPromoView) itemView;
            mSigninPromoController.setupPromoView(view.getContext(), view, profileData, null);
        }
    }
}
