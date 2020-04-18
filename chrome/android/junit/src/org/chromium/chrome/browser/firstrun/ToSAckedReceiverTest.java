// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.firstrun;

import android.accounts.Account;
import android.content.Intent;
import android.os.Build;

import org.hamcrest.Matchers;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mockito;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.annotation.Config;
import org.robolectric.util.ReflectionHelpers;

import org.chromium.base.ContextUtils;
import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.components.signin.AccountManagerDelegate;
import org.chromium.components.signin.AccountManagerDelegateException;
import org.chromium.components.signin.AccountManagerFacade;

import java.util.HashSet;
import java.util.Set;

@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class ToSAckedReceiverTest {
    private static final String GOOGLE_ACCOUNT = "TestyMcTesterson@gmail.com";

    private ToSAckedReceiver mReceiver;

    @Before
    public void setUp() {
        ReflectionHelpers.setStaticField(
                Build.VERSION.class, "SDK_INT", Build.VERSION_CODES.KITKAT);
        mReceiver = new ToSAckedReceiver();
    }

    @Test
    public void testNoToSAccounts() {
        Assert.assertFalse(ToSAckedReceiver.checkAnyUserHasSeenToS());
    }

    @Test
    public void testReceivedToSPing() throws AccountManagerDelegateException {
        Intent intent = new Intent();
        intent.putExtra(ToSAckedReceiver.EXTRA_ACCOUNT_NAME, GOOGLE_ACCOUNT);

        mReceiver.onReceive(RuntimeEnvironment.application, intent);
        Assert.assertFalse(ToSAckedReceiver.checkAnyUserHasSeenToS());
        Set<String> toSAckedAccounts = ContextUtils.getAppSharedPreferences().getStringSet(
                ToSAckedReceiver.TOS_ACKED_ACCOUNTS, new HashSet<>());
        Assert.assertThat(toSAckedAccounts, Matchers.contains(GOOGLE_ACCOUNT));

        AccountManagerDelegate accountManagerDelegate = Mockito.mock(AccountManagerDelegate.class);
        Account[] accounts = new Account[1];
        accounts[0] = new Account(GOOGLE_ACCOUNT, "LegitAccount");
        Mockito.doReturn(accounts).when(accountManagerDelegate).getAccountsSync();
        AccountManagerFacade.overrideAccountManagerFacadeForTests(accountManagerDelegate);
        Assert.assertTrue(ToSAckedReceiver.checkAnyUserHasSeenToS());
    }
}
