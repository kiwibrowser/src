// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.test.util;

import android.accounts.Account;
import android.accounts.AccountManager;
import android.accounts.AccountManagerCallback;
import android.accounts.AccountManagerFuture;
import android.accounts.AuthenticatorException;
import android.accounts.OperationCanceledException;
import android.app.Instrumentation;
import android.content.Context;
import android.os.Bundle;
import android.text.TextUtils;

import org.chromium.components.signin.AccountManagerFacade;
import org.chromium.components.signin.ChromeSigninController;
import org.chromium.components.signin.test.util.AccountHolder;
import org.chromium.components.signin.test.util.FakeAccountManagerDelegate;

import java.io.IOException;

/**
 *  A tool used for running tests as signed in.
 */
public class ChromeSigninUtils {
    /** Bundle tags */
    private static final String USERNAME = "username";
    private static final String PASSWORD = "password";
    private static final String ALLOW_SKIP = "allowSkip";

    /** Information for Google OS account */
    private static final String GOOGLE_ACCOUNT_TYPE = "com.google";

    private AccountManager mAccountManager;
    private FakeAccountManagerDelegate mFakeAccountManagerDelegate;
    private Context mTargetContext;

    /**
     * The constructor for SigninUtils for a given test case.
     *
     * @param instrumentation the {@link android.app.Instrumentation} to perform signin operations
     * with.
     */
    public ChromeSigninUtils(Instrumentation instrumentation) {
        mTargetContext = instrumentation.getTargetContext();
        mAccountManager = AccountManager.get(mTargetContext);
        mFakeAccountManagerDelegate = new FakeAccountManagerDelegate(
                FakeAccountManagerDelegate.DISABLE_PROFILE_DATA_SOURCE);
        AccountManagerFacade.overrideAccountManagerFacadeForTests(mFakeAccountManagerDelegate);
    }

    /**
     * Adds an account to the app.
     *
     * @param username the username for the logged in account; must be a Google account.
     */
    public void addAccountToApp(String username) {
        if (TextUtils.isEmpty(username)) {
            throw new IllegalArgumentException("ERROR: must specify account");
        }

        if (ChromeSigninController.get().isSignedIn()) {
            ChromeSigninController.get().setSignedInAccountName(null);
        }
        ChromeSigninController.get().setSignedInAccountName(username);
    }

    /**
     * Adds a fake account to the OS.
     */
    public void addFakeAccountToOs(String username, String password) {
        if (TextUtils.isEmpty(username)) {
            throw new IllegalArgumentException("ERROR: must specify account");
        }

        Account account = new Account(username, GOOGLE_ACCOUNT_TYPE);
        AccountHolder accountHolder = AccountHolder.builder(account).password(password).build();
        mFakeAccountManagerDelegate.addAccountHolderBlocking(accountHolder);
    }

    /**
     * Removes all fake accounts from the OS.
     */
    public void removeAllFakeAccountsFromOs() throws Exception {
        for (Account acct : mFakeAccountManagerDelegate.getAccountsSyncNoThrow()) {
            mFakeAccountManagerDelegate.removeAccountHolderBlocking(
                    AccountHolder.builder(acct).build());
        }
    }

    /**
     * Checks if an existing fake account is on the OS.
     *
     * @param username the username of the account you want to check if signed in
     * @return {@code true} if fake account is on OS, false otherwise.
     */
    public boolean isExistingFakeAccountOnOs(String username) {
        for (Account acct : mFakeAccountManagerDelegate.getAccountsSyncNoThrow()) {
            if (username.equals(acct.name)) {
                return true;
            }
        }
        return false;
    }

    /**
     * Adds a Google account to the OS.
     *
     * @param username the username for the logged in account; must be a Google account.
     * @param password the password for the username specified
     * @param googleAccountType the account type to log into (e.g. @gmail.com accounts use "mail")
     */
    public void addGoogleAccountToOs(String username, String password, String googleAccountType) {
        final Bundle options = new Bundle();
        options.putString(USERNAME, username);
        options.putString(PASSWORD, password);
        options.putBoolean(ALLOW_SKIP, true);

        AuthenticationCallback authCallback = new AuthenticationCallback();
        mAccountManager.addAccount(GOOGLE_ACCOUNT_TYPE, googleAccountType, null, options, null,
                authCallback, null);

        Bundle authResult = authCallback.waitForAuthCompletion();

        if (authResult.containsKey(AccountManager.KEY_ERROR_CODE)) {
            throw new IllegalStateException(String.format("AddAccount failed. Reason: %s",
                    authResult.get(AccountManager.KEY_ERROR_MESSAGE)));
        }
    }

    /**
     * Checks to see if an existing Google account is on OS.
     *
     * @param username the username of the account you want to check if signed in
     * @return {@code true} if one or more Google accounts exists on OS, {@code false} otherwise
     */
    public boolean isExistingGoogleAccountOnOs(String username) {
        if (mAccountManager.getAccountsByType(GOOGLE_ACCOUNT_TYPE).length == 0) {
            return false;
        }

        for (Account acct : mAccountManager.getAccountsByType(GOOGLE_ACCOUNT_TYPE)) {
            if (username.equals(acct.name)) {
                return true;
            }
        }
        return false;
    }

    /**
     * Removes all Google accounts from the OS.
     */
    // TODO(jbudorick): fix deprecation warnings crbug.com/537352
    @SuppressWarnings("deprecation")
    public void removeAllGoogleAccountsFromOs() {
        for (Account acct : mAccountManager.getAccountsByType(GOOGLE_ACCOUNT_TYPE)) {
            mAccountManager.removeAccount(acct, null, null);
        }
    }

    /**
     * Helper class for adding Google accounts to OS.
     *
     * Usage: Use this as the callback parameter when using {@link addAccount} in
     * {@link android.accounts.AccountManager}.
     */
    private static class AuthenticationCallback implements AccountManagerCallback<Bundle> {
        private final Object mAuthenticationCompletionLock = new Object();

        /** Stores the result of account authentication. Null means not finished. */
        private Bundle mResultBundle;

        /**
         * Block and wait for the authentication callback to complete.
         *
         * @return the {@link Bundle} result from the authentication.
         */
        public Bundle waitForAuthCompletion() {
            synchronized (mAuthenticationCompletionLock) {
                while (mResultBundle == null) {
                    try {
                        mAuthenticationCompletionLock.wait();
                    } catch (InterruptedException e) {
                        // ignore
                    }
                }
                return mResultBundle;
            }
        }

        @Override
        public void run(AccountManagerFuture<Bundle> future) {
            Bundle resultBundle;
            try {
                resultBundle = future.getResult();
            } catch (OperationCanceledException e) {
                resultBundle = buildExceptionBundle(e);
            } catch (IOException e) {
                resultBundle = buildExceptionBundle(e);
            } catch (AuthenticatorException e) {
                resultBundle = buildExceptionBundle(e);
            }
            synchronized (mAuthenticationCompletionLock) {
                mResultBundle = resultBundle;
                mAuthenticationCompletionLock.notify();
            }
        }

        /**
         * Create a result bundle for a given exception.
         *
         * @param e the exception to be made into a {@link Bundle}
         * @return the {@link Bundle} for the given exception e.
         */
        private Bundle buildExceptionBundle(Exception e) {
            Bundle bundle = new Bundle();
            bundle.putInt(AccountManager.KEY_ERROR_CODE,
                    AccountManager.ERROR_CODE_INVALID_RESPONSE);
            bundle.putString(AccountManager.KEY_ERROR_MESSAGE, e.toString());
            return bundle;
        }
    }
}
