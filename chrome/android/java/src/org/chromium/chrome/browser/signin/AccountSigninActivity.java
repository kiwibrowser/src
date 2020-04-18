// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.signin;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.support.annotation.IntDef;

import org.chromium.base.metrics.RecordHistogram;
import org.chromium.base.metrics.RecordUserAction;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ChromeFeatureList;
import org.chromium.chrome.browser.SynchronousInitializationActivity;
import org.chromium.chrome.browser.preferences.ManagedPreferencesUtils;
import org.chromium.chrome.browser.preferences.PrefServiceBridge;
import org.chromium.chrome.browser.preferences.PreferencesLauncher;
import org.chromium.chrome.browser.signin.SigninManager.SignInCallback;
import org.chromium.components.signin.ChildAccountStatus;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

/**
 * An Activity displayed from the MainPreferences to allow the user to pick an account to
 * sign in to. The AccountSigninView.Delegate interface is fulfilled by the AppCompatActivity.
 */
public class AccountSigninActivity extends SynchronousInitializationActivity
        implements AccountSigninView.Listener, AccountSigninView.Delegate {
    private static final String TAG = "AccountSigninActivity";
    private static final String INTENT_IS_FROM_PERSONALIZED_PROMO =
            "AccountSigninActivity.IsFromPersonalizedPromo";

    @IntDef({SigninAccessPoint.SETTINGS, SigninAccessPoint.BOOKMARK_MANAGER,
            SigninAccessPoint.RECENT_TABS, SigninAccessPoint.SIGNIN_PROMO,
            SigninAccessPoint.NTP_CONTENT_SUGGESTIONS, SigninAccessPoint.AUTOFILL_DROPDOWN})
    @Retention(RetentionPolicy.SOURCE)
    public @interface AccessPoint {}

    @IntDef({SwitchAccountSource.SIGNOUT_SIGNIN, SwitchAccountSource.SYNC_ACCOUNT_SWITCHER})
    @Retention(RetentionPolicy.SOURCE)
    public @interface SwitchAccountSource {
        int SIGNOUT_SIGNIN = 0;
        int SYNC_ACCOUNT_SWITCHER = 1;
        int MAX = 2;
    }

    private @AccessPoint int mAccessPoint;
    private @AccountSigninView.SigninFlowType int mSigninFlowType;
    private boolean mIsFromPersonalizedPromo;

    /**
     * A convenience method to create a AccountSigninActivity passing the access point as an
     * intent. Checks if the sign in flow can be started before showing the activity.
     * @param accessPoint {@link AccessPoint} for starting signin flow. Used in metrics.
     * @return {@code true} if sign in has been allowed.
     */
    public static boolean startIfAllowed(Context context, @AccessPoint int accessPoint) {
        if (!SigninManager.get().isSignInAllowed()) {
            if (SigninManager.get().isSigninDisabledByPolicy()) {
                ManagedPreferencesUtils.showManagedByAdministratorToast(context);
            }
            return false;
        }

        final Intent intent;
        if (ChromeFeatureList.isEnabled(ChromeFeatureList.UNIFIED_CONSENT)) {
            intent = SigninActivity.createIntent(context, accessPoint);
        } else {
            intent = createIntentForDefaultSigninFlow(context, accessPoint, false);
        }
        context.startActivity(intent);
        return true;
    }

    /**
     * Creates an {@link Intent} which can be used to start the default signin flow.
     * @param accessPoint {@link AccessPoint} for starting signin flow. Used in metrics.
     * @param isFromPersonalizedPromo Whether the signin activity is started from a personalized
     *         promo.
     */
    public static Intent createIntentForDefaultSigninFlow(
            Context context, @AccessPoint int accessPoint, boolean isFromPersonalizedPromo) {
        Intent intent = new Intent(context, AccountSigninActivity.class);
        Bundle viewArguments = AccountSigninView.createArgumentsForDefaultFlow(
                accessPoint, ChildAccountStatus.NOT_CHILD);
        intent.putExtras(viewArguments);
        intent.putExtra(INTENT_IS_FROM_PERSONALIZED_PROMO, isFromPersonalizedPromo);
        return intent;
    }

    /**
     * Creates an {@link Intent} which can be used to start the signin flow from the confirmation
     * screen.
     * @param accessPoint {@link AccessPoint} for starting signin flow. Used in metrics.
     * @param selectAccount Account for which signin confirmation page should be shown.
     * @param isDefaultAccount Whether {@param selectedAccount} is the default account on
     *         the device. Used in metrics.
     * @param isFromPersonalizedPromo Whether the signin activity is started from a personalized
     *         promo.
     */
    public static Intent createIntentForConfirmationOnlySigninFlow(Context context,
            @AccessPoint int accessPoint, String selectAccount, boolean isDefaultAccount,
            boolean isFromPersonalizedPromo) {
        Intent intent = new Intent(context, AccountSigninActivity.class);
        Bundle viewArguments = AccountSigninView.createArgumentsForConfirmationFlow(accessPoint,
                ChildAccountStatus.NOT_CHILD, selectAccount, isDefaultAccount,
                AccountSigninView.UNDO_ABORT);
        intent.putExtras(viewArguments);
        intent.putExtra(INTENT_IS_FROM_PERSONALIZED_PROMO, isFromPersonalizedPromo);
        return intent;
    }

    /**
     * Creates an {@link Intent} which can be used to start the signin flow from the "Add Account"
     * page.
     * @param accessPoint {@link AccessPoint} for starting signin flow. Used in metrics.
     * @param isFromPersonalizedPromo Whether the signin activity is started from a personalized
     *         promo.
     */
    public static Intent createIntentForAddAccountSigninFlow(
            Context context, @AccessPoint int accessPoint, boolean isFromPersonalizedPromo) {
        Intent intent = new Intent(context, AccountSigninActivity.class);
        Bundle viewArguments = AccountSigninView.createArgumentsForAddAccountFlow(accessPoint);
        intent.putExtras(viewArguments);
        intent.putExtra(INTENT_IS_FROM_PERSONALIZED_PROMO, isFromPersonalizedPromo);
        return intent;
    }

    /**
     * Records the flow that was used to switch sync accounts.
     * @param source {@link SwitchAccountSource} that was used for switching accounts.
     */
    public static void recordSwitchAccountSourceHistogram(@SwitchAccountSource int source) {
        RecordHistogram.recordEnumeratedHistogram(
                "Signin.SwitchSyncAccount.Source", source, SwitchAccountSource.MAX);
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        // We don't trust android to restore the saved state correctly, so pass null.
        super.onCreate(null);

        AccountSigninView view =
                (AccountSigninView) getLayoutInflater().inflate(R.layout.account_signin_view, null);

        view.init(getIntent().getExtras(), this, this);

        mAccessPoint = view.getSigninAccessPoint();
        assert mAccessPoint == SigninAccessPoint.BOOKMARK_MANAGER
                || mAccessPoint == SigninAccessPoint.RECENT_TABS
                || mAccessPoint == SigninAccessPoint.SETTINGS
                || mAccessPoint == SigninAccessPoint.SIGNIN_PROMO
                || mAccessPoint == SigninAccessPoint.NTP_CONTENT_SUGGESTIONS
                || mAccessPoint == SigninAccessPoint.AUTOFILL_DROPDOWN
                : "invalid access point: " + mAccessPoint;

        mSigninFlowType = view.getSigninFlowType();
        mIsFromPersonalizedPromo =
                getIntent().getBooleanExtra(INTENT_IS_FROM_PERSONALIZED_PROMO, false);

        setContentView(view);

        SigninManager.logSigninStartAccessPoint(mAccessPoint);
        recordSigninStartedHistogramAccountInfo();
        recordSigninStartedUserAction();
    }

    @Override
    public void onAccountSelectionCanceled() {
        finish();
    }

    @Override
    public void onNewAccount() {
        AccountAdder.getInstance().addAccount(this, AccountAdder.ADD_ACCOUNT_RESULT);
    }

    @Override
    public void onAccountSelected(
            final String accountName, boolean isDefaultAccount, final boolean settingsClicked) {
        if (PrefServiceBridge.getInstance().getSyncLastAccountName() != null) {
            recordSwitchAccountSourceHistogram(SwitchAccountSource.SIGNOUT_SIGNIN);
        }

        final Context context = this;
        SigninManager.get().signIn(accountName, this, new SignInCallback() {
            @Override
            public void onSignInComplete() {
                if (settingsClicked) {
                    Intent intent = PreferencesLauncher.createIntentForSettingsPage(
                            context, AccountManagementFragment.class.getName());
                    startActivity(intent);
                }

                recordSigninCompletedHistogramAccountInfo();
                finish();
            }

            @Override
            public void onSignInAborted() {}
        });
    }

    @Override
    public void onFailedToSetForcedAccount(String forcedAccountName) {}

    private void recordSigninCompletedHistogramAccountInfo() {
        if (!mIsFromPersonalizedPromo) {
            return;
        }

        final String histogram;
        switch (mSigninFlowType) {
            case AccountSigninView.SIGNIN_FLOW_ADD_NEW_ACCOUNT:
                histogram = "Signin.SigninCompletedAccessPoint.NewAccount";
                break;
            case AccountSigninView.SIGNIN_FLOW_CONFIRMATION_ONLY:
                histogram = "Signin.SigninCompletedAccessPoint.WithDefault";
                break;
            case AccountSigninView.SIGNIN_FLOW_DEFAULT:
                histogram = "Signin.SigninCompletedAccessPoint.NotDefault";
                break;
            default:
                assert false : "Unexpected signin flow type!";
                return;
        }

        RecordHistogram.recordEnumeratedHistogram(histogram, mAccessPoint, SigninAccessPoint.MAX);
    }

    private void recordSigninStartedHistogramAccountInfo() {
        if (!mIsFromPersonalizedPromo) {
            return;
        }

        final String histogram;
        switch (mSigninFlowType) {
            case AccountSigninView.SIGNIN_FLOW_ADD_NEW_ACCOUNT:
                histogram = "Signin.SigninStartedAccessPoint.NewAccount";
                break;
            case AccountSigninView.SIGNIN_FLOW_CONFIRMATION_ONLY:
                histogram = "Signin.SigninStartedAccessPoint.WithDefault";
                break;
            case AccountSigninView.SIGNIN_FLOW_DEFAULT:
                histogram = "Signin.SigninStartedAccessPoint.NotDefault";
                break;
            default:
                assert false : "Unexpected signin flow type!";
                return;
        }

        RecordHistogram.recordEnumeratedHistogram(histogram, mAccessPoint, SigninAccessPoint.MAX);
    }

    private void recordSigninStartedUserAction() {
        switch (mAccessPoint) {
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

    // AccountSigninView.Delegate implementation.
    @Override
    public Activity getActivity() {
        return this;
    }
}
