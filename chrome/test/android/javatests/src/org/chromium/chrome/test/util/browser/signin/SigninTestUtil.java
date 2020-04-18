// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.test.util.browser.signin;

import android.accounts.Account;
import android.annotation.SuppressLint;
import android.app.Instrumentation;
import android.content.Context;

import org.chromium.base.ContextUtils;
import org.chromium.base.ThreadUtils;
import org.chromium.chrome.browser.init.ProcessInitializationHandler;
import org.chromium.chrome.browser.signin.AccountTrackerService;
import org.chromium.chrome.browser.signin.OAuth2TokenService;
import org.chromium.components.signin.AccountIdProvider;
import org.chromium.components.signin.AccountManagerFacade;
import org.chromium.components.signin.ChromeSigninController;
import org.chromium.components.signin.test.util.AccountHolder;
import org.chromium.components.signin.test.util.FakeAccountManagerDelegate;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;

/**
 * Utility class for test signin functionality.
 */
public final class SigninTestUtil {
    private static final String TAG = "Signin";

    private static final String DEFAULT_ACCOUNT = "test@gmail.com";

    @SuppressLint("StaticFieldLeak")
    private static Context sContext;
    @SuppressLint("StaticFieldLeak")
    private static FakeAccountManagerDelegate sAccountManager;
    @SuppressLint("StaticFieldLeak")
    private static List<AccountHolder> sAddedAccounts = new ArrayList<>();

    /**
     * Sets up the test authentication environment.
     *
     * This must be called before native is loaded.
     */
    public static void setUpAuthForTest(Instrumentation instrumentation) {
        assert sContext == null;
        sContext = instrumentation.getTargetContext();
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                ProcessInitializationHandler.getInstance().initializePreNative();
            }
        });
        sAccountManager = new FakeAccountManagerDelegate(
                FakeAccountManagerDelegate.DISABLE_PROFILE_DATA_SOURCE);
        AccountManagerFacade.overrideAccountManagerFacadeForTests(sAccountManager);
        overrideAccountIdProvider();
        resetSigninState();
    }

    /**
     * Tears down the test authentication environment.
     */
    public static void tearDownAuthForTest() {
        ThreadUtils.runOnUiThreadBlocking(() -> {
            for (AccountHolder accountHolder : sAddedAccounts) {
                sAccountManager.removeAccountHolderExplicitly(accountHolder);
            }
        });
        sAddedAccounts.clear();
        sContext = null;
    }

    /**
     * Returns the currently signed in account.
     */
    public static Account getCurrentAccount() {
        assert sContext != null;
        return ChromeSigninController.get().getSignedInUser();
    }

    /**
     * Add an account with the default name.
     */
    public static Account addTestAccount() {
        return addTestAccount(DEFAULT_ACCOUNT);
    }

    /**
     * Add an account with a given name.
     */
    public static Account addTestAccount(String name) {
        Account account = AccountManagerFacade.createAccountFromName(name);
        AccountHolder accountHolder = AccountHolder.builder(account).alwaysAccept(true).build();
        sAccountManager.addAccountHolderBlocking(accountHolder);
        sAddedAccounts.add(accountHolder);
        ThreadUtils.runOnUiThreadBlocking(
                () -> AccountTrackerService.get().invalidateAccountSeedStatus(true));
        return account;
    }

    /**
     * Add and sign in an account with the default name.
     */
    public static Account addAndSignInTestAccount() {
        Account account = addTestAccount();
        ThreadUtils.runOnUiThreadBlocking(() -> {
            ChromeSigninController.get().setSignedInAccountName(DEFAULT_ACCOUNT);
            AccountTrackerService.get().invalidateAccountSeedStatus(true);
        });
        return account;
    }

    private static void overrideAccountIdProvider() {
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                AccountIdProvider.setInstanceForTest(new AccountIdProvider() {
                    @Override
                    public String getAccountId(String accountName) {
                        return "gaia-id-" + accountName;
                    }

                    @Override
                    public boolean canBeUsed() {
                        return true;
                    }
                });
            }
        });
    }

    /**
     * Should be called at setUp and tearDown so that the signin state is not leaked across tests.
     * The setUp call is implicit inside the constructor.
     */
    public static void resetSigninState() {
        // Clear cached signed account name and accounts list.
        ChromeSigninController.get().setSignedInAccountName(null);
        ContextUtils.getAppSharedPreferences()
                .edit()
                .putStringSet(OAuth2TokenService.STORED_ACCOUNTS_KEY, new HashSet<String>())
                .apply();
    }

    private SigninTestUtil() {}
}
