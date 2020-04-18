// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.firstrun;

import android.app.Activity;
import android.app.Fragment;
import android.os.Bundle;

import org.chromium.base.metrics.RecordUserAction;
import org.chromium.chrome.browser.ntp.cards.SignInPromo;
import org.chromium.chrome.browser.signin.SigninAccessPoint;
import org.chromium.chrome.browser.signin.SigninFragmentBase;
import org.chromium.chrome.browser.signin.SigninManager;
import org.chromium.components.signin.ChildAccountStatus;

/** A {@link Fragment} to handle sign-in within the first run experience. */
public class SigninFirstRunFragment extends SigninFragmentBase implements FirstRunFragment {
    private Bundle mArguments;

    // Every fragment must have a public default constructor.
    public SigninFirstRunFragment() {}

    @Override
    public void onAttach(Activity activity) {
        super.onAttach(activity);

        Bundle freProperties = getPageDelegate().getProperties();
        String forceAccountTo =
                freProperties.getString(AccountFirstRunFragment.FORCE_SIGNIN_ACCOUNT_TO);
        if (forceAccountTo == null) {
            mArguments = createArguments(SigninAccessPoint.START_PAGE, null);
        } else {
            @ChildAccountStatus.Status int childAccountStatus =
                    freProperties.getInt(AccountFirstRunFragment.CHILD_ACCOUNT_STATUS);
            mArguments = createArgumentsForForcedSigninFlow(
                    SigninAccessPoint.START_PAGE, forceAccountTo, childAccountStatus);
        }

        RecordUserAction.record("MobileFre.SignInShown");
        RecordUserAction.record("Signin_Signin_FromStartPage");
        SigninManager.logSigninStartAccessPoint(SigninAccessPoint.START_PAGE);
    }

    @Override
    protected Bundle getSigninArguments() {
        return mArguments;
    }

    @Override
    protected void onSigninRefused() {
        if (isForcedSignin()) {
            // Somehow the forced account disappeared while we were in the FRE.
            // The user would have to go through the FRE again.
            getPageDelegate().abortFirstRunExperience();
        } else {
            SignInPromo.temporarilySuppressPromos();
            getPageDelegate().refuseSignIn();
            getPageDelegate().advanceToNextPage();
        }
    }

    @Override
    protected void onSigninAccepted(String accountName, boolean isDefaultAccount,
            boolean settingsClicked, Runnable callback) {
        getPageDelegate().acceptSignIn(accountName, isDefaultAccount);
        if (settingsClicked) {
            getPageDelegate().askToOpenSignInSettings();
        }
        getPageDelegate().advanceToNextPage();
        callback.run();
    }
}
