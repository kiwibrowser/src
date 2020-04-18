// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.signin;

import android.accounts.Account;
import android.support.test.InstrumentationRegistry;
import android.support.test.filters.SmallTest;

import org.junit.After;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.test.util.AdvancedMockContext;
import org.chromium.base.test.util.DisabledTest;
import org.chromium.base.test.util.Feature;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.components.signin.AccountManagerFacade;
import org.chromium.components.signin.test.util.AccountHolder;
import org.chromium.components.signin.test.util.FakeAccountManagerDelegate;

import java.util.Arrays;
import java.util.concurrent.TimeUnit;

/** Tests for OAuth2TokenService. */
@RunWith(ChromeJUnit4ClassRunner.class)
public class OAuth2TokenServiceTest {
    private AdvancedMockContext mContext;
    private FakeAccountManagerDelegate mAccountManager;

    @Before
    public void setUp() throws Exception {
        mContext = new AdvancedMockContext(InstrumentationRegistry.getTargetContext());
        mAccountManager = new FakeAccountManagerDelegate(
                FakeAccountManagerDelegate.DISABLE_PROFILE_DATA_SOURCE);
        AccountManagerFacade.overrideAccountManagerFacadeForTests(mAccountManager);
    }

    @After
    public void tearDown() {
        AccountManagerFacade.resetAccountManagerFacadeForTests();
    }

    /*
     *  @SmallTest
     *  @Feature({"Sync"})
     */
    @Test
    @DisabledTest(message = "crbug.com/533417")
    public void testGetAccountsNoAccountsRegistered() {
        String[] accounts = OAuth2TokenService.getAccounts();
        Assert.assertEquals("There should be no accounts registered", 0, accounts.length);
    }

    /*@SmallTest
    @Feature({"Sync"})*/
    @Test
    @DisabledTest(message = "crbug.com/527852")
    public void testGetAccountsOneAccountRegistered() {
        Account account1 = AccountManagerFacade.createAccountFromName("foo@gmail.com");
        AccountHolder accountHolder1 = AccountHolder.builder(account1).build();
        mAccountManager.addAccountHolderBlocking(accountHolder1);

        String[] sysAccounts = OAuth2TokenService.getSystemAccountNames();
        Assert.assertEquals("There should be one registered account", 1, sysAccounts.length);
        Assert.assertEquals("The account should be " + account1, account1.name, sysAccounts[0]);

        String[] accounts = OAuth2TokenService.getAccounts();
        Assert.assertEquals("There should be zero registered account", 0, accounts.length);
    }

    /*@SmallTest
    @Feature({"Sync"})*/
    @Test
    @DisabledTest(message = "crbug.com/527852")
    public void testGetAccountsTwoAccountsRegistered() {
        Account account1 = AccountManagerFacade.createAccountFromName("foo@gmail.com");
        AccountHolder accountHolder1 = AccountHolder.builder(account1).build();
        mAccountManager.addAccountHolderBlocking(accountHolder1);
        Account account2 = AccountManagerFacade.createAccountFromName("bar@gmail.com");
        AccountHolder accountHolder2 = AccountHolder.builder(account2).build();
        mAccountManager.addAccountHolderBlocking(accountHolder2);

        String[] sysAccounts = OAuth2TokenService.getSystemAccountNames();
        Assert.assertEquals("There should be one registered account", 2, sysAccounts.length);
        Assert.assertTrue("The list should contain " + account1,
                Arrays.asList(sysAccounts).contains(account1.name));
        Assert.assertTrue("The list should contain " + account2,
                Arrays.asList(sysAccounts).contains(account2.name));

        String[] accounts = OAuth2TokenService.getAccounts();
        Assert.assertEquals("There should be zero registered account", 0, accounts.length);
    }

    @Test
    @DisabledTest(message = "crbug.com/568620")
    @SmallTest
    @Feature({"Sync"})
    public void testGetOAuth2AccessTokenWithTimeoutOnSuccess() {
        String authToken = "someToken";
        // Auth token should be successfully received.
        runTestOfGetOAuth2AccessTokenWithTimeout(authToken);
    }

    /*@SmallTest
    @Feature({"Sync"})*/
    @Test
    @DisabledTest(message = "crbug.com/527852")
    public void testGetOAuth2AccessTokenWithTimeoutOnError() {
        String authToken = null;
        // Should not crash when auth token is null.
        runTestOfGetOAuth2AccessTokenWithTimeout(authToken);
    }

    private void runTestOfGetOAuth2AccessTokenWithTimeout(String expectedToken) {
        String scope = "http://example.com/scope";
        Account account = AccountManagerFacade.createAccountFromName("test@gmail.com");
        String oauth2Scope = "oauth2:" + scope;

        // Add an account with given auth token for the given scope, already accepted auth popup.
        AccountHolder accountHolder = AccountHolder.builder(account)
                                              .hasBeenAccepted(oauth2Scope, true)
                                              .authToken(oauth2Scope, expectedToken)
                                              .build();
        mAccountManager.addAccountHolderBlocking(accountHolder);

        String accessToken = OAuth2TokenService.getOAuth2AccessTokenWithTimeout(
                mContext, account, scope, 5, TimeUnit.SECONDS);
        Assert.assertEquals(expectedToken, accessToken);
    }
}
