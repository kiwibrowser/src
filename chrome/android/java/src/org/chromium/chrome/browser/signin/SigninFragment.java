// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.signin;

import android.content.Intent;
import android.os.Bundle;
import android.support.annotation.IntDef;
import android.support.annotation.Nullable;

import org.chromium.base.metrics.RecordHistogram;
import org.chromium.base.metrics.RecordUserAction;
import org.chromium.chrome.browser.preferences.PrefServiceBridge;
import org.chromium.chrome.browser.preferences.PreferencesLauncher;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

/** This fragment implements sign-in screen for {@link SigninActivity}. */
public class SigninFragment extends SigninFragmentBase {
    private static final String TAG = "SigninFragment";

    private static final String ARGUMENT_PERSONALIZED_PROMO_ACTION =
            "SigninFragment.PersonalizedPromoAction";

    @IntDef({PROMO_ACTION_NONE, PROMO_ACTION_WITH_DEFAULT, PROMO_ACTION_NOT_DEFAULT,
            PROMO_ACTION_NEW_ACCOUNT})
    @Retention(RetentionPolicy.SOURCE)
    public @interface PromoAction {}

    public static final int PROMO_ACTION_NONE = 0;
    public static final int PROMO_ACTION_WITH_DEFAULT = 1;
    public static final int PROMO_ACTION_NOT_DEFAULT = 2;
    public static final int PROMO_ACTION_NEW_ACCOUNT = 3;

    private @PromoAction int mPromoAction;

    /**
     * Creates an argument bundle to start sign-in.
     * @param accessPoint The access point for starting sign-in flow.
     */
    public static Bundle createArguments(@SigninAccessPoint int accessPoint) {
        return SigninFragmentBase.createArguments(accessPoint, null);
    }

    /**
     * Creates an argument bundle to start sign-in from personalized sign-in promo.
     * @param accessPoint The access point for starting sign-in flow.
     * @param accountName The account to preselect or null to preselect the default account.
     */
    public static Bundle createArgumentsForPromoDefaultFlow(
            @SigninAccessPoint int accessPoint, String accountName) {
        Bundle result = SigninFragmentBase.createArguments(accessPoint, accountName);
        result.putInt(ARGUMENT_PERSONALIZED_PROMO_ACTION, PROMO_ACTION_WITH_DEFAULT);
        return result;
    }

    /**
     * Creates an argument bundle to start "Choose account" sign-in flow from personalized sign-in
     * promo.
     * @param accessPoint The access point for starting sign-in flow.
     * @param accountName The account to preselect or null to preselect the default account.
     */
    public static Bundle createArgumentsForPromoChooseAccountFlow(
            @SigninAccessPoint int accessPoint, String accountName) {
        Bundle result =
                SigninFragmentBase.createArgumentsForChooseAccountFlow(accessPoint, accountName);
        result.putInt(ARGUMENT_PERSONALIZED_PROMO_ACTION, PROMO_ACTION_NOT_DEFAULT);
        return result;
    }

    /**
     * Creates an argument bundle to start "New account" sign-in flow from personalized sign-in
     * promo.
     * @param accessPoint The access point for starting sign-in flow.
     */
    public static Bundle createArgumentsForPromoAddAccountFlow(@SigninAccessPoint int accessPoint) {
        Bundle result = SigninFragmentBase.createArgumentsForAddAccountFlow(accessPoint);
        result.putInt(ARGUMENT_PERSONALIZED_PROMO_ACTION, PROMO_ACTION_NEW_ACCOUNT);
        return result;
    }

    // Every fragment must have a public default constructor.
    public SigninFragment() {}

    @Override
    public void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        mPromoAction =
                getSigninArguments().getInt(ARGUMENT_PERSONALIZED_PROMO_ACTION, PROMO_ACTION_NONE);

        SigninManager.logSigninStartAccessPoint(getSigninAccessPoint());
        recordSigninStartedHistogramAccountInfo();
        recordSigninStartedUserAction();
    }

    @Override
    protected Bundle getSigninArguments() {
        return getArguments();
    }

    @Override
    protected void onSigninRefused() {
        getActivity().finish();
    }

    @Override
    protected void onSigninAccepted(String accountName, boolean isDefaultAccount,
            boolean settingsClicked, Runnable callback) {
        if (PrefServiceBridge.getInstance().getSyncLastAccountName() != null) {
            AccountSigninActivity.recordSwitchAccountSourceHistogram(
                    AccountSigninActivity.SwitchAccountSource.SIGNOUT_SIGNIN);
        }

        SigninManager.get().signIn(accountName, getActivity(), new SigninManager.SignInCallback() {
            @Override
            public void onSignInComplete() {
                if (settingsClicked) {
                    Intent intent = PreferencesLauncher.createIntentForSettingsPage(
                            getActivity(), AccountManagementFragment.class.getName());
                    startActivity(intent);
                }

                recordSigninCompletedHistogramAccountInfo();
                getActivity().finish();
                callback.run();
            }

            @Override
            public void onSignInAborted() {
                callback.run();
            }
        });
    }

    private void recordSigninCompletedHistogramAccountInfo() {
        final String histogram;
        switch (mPromoAction) {
            case PROMO_ACTION_NONE:
                return;
            case PROMO_ACTION_WITH_DEFAULT:
                histogram = "Signin.SigninCompletedAccessPoint.WithDefault";
                break;
            case PROMO_ACTION_NOT_DEFAULT:
                histogram = "Signin.SigninCompletedAccessPoint.NotDefault";
                break;
            case PROMO_ACTION_NEW_ACCOUNT:
                histogram = "Signin.SigninCompletedAccessPoint.NewAccount";
                break;
            default:
                assert false : "Unexpected sign-in flow type!";
                return;
        }

        RecordHistogram.recordEnumeratedHistogram(
                histogram, getSigninAccessPoint(), SigninAccessPoint.MAX);
    }

    private void recordSigninStartedHistogramAccountInfo() {
        final String histogram;
        switch (mPromoAction) {
            case PROMO_ACTION_NONE:
                return;
            case PROMO_ACTION_WITH_DEFAULT:
                histogram = "Signin.SigninStartedAccessPoint.WithDefault";
                break;
            case PROMO_ACTION_NOT_DEFAULT:
                histogram = "Signin.SigninStartedAccessPoint.NotDefault";
                break;
            case PROMO_ACTION_NEW_ACCOUNT:
                histogram = "Signin.SigninStartedAccessPoint.NewAccount";
                break;
            default:
                assert false : "Unexpected sign-in flow type!";
                return;
        }

        RecordHistogram.recordEnumeratedHistogram(
                histogram, getSigninAccessPoint(), SigninAccessPoint.MAX);
    }

    private void recordSigninStartedUserAction() {
        switch (getSigninAccessPoint()) {
            case SigninAccessPoint.AUTOFILL_DROPDOWN:
                RecordUserAction.record("Signin_Signin_FromAutofillDropdown");
                break;
            case SigninAccessPoint.BOOKMARK_MANAGER:
                RecordUserAction.record("Signin_Signin_FromBookmarkManager");
                break;
            case SigninAccessPoint.RECENT_TABS:
                RecordUserAction.record("Signin_Signin_FromRecentTabs");
                break;
            case SigninAccessPoint.SETTINGS:
                RecordUserAction.record("Signin_Signin_FromSettings");
                break;
            case SigninAccessPoint.SIGNIN_PROMO:
                RecordUserAction.record("Signin_Signin_FromSigninPromo");
                break;
            case SigninAccessPoint.NTP_CONTENT_SUGGESTIONS:
                RecordUserAction.record("Signin_Signin_FromNTPContentSuggestions");
                break;
            default:
                assert false : "Invalid access point.";
        }
    }
}
