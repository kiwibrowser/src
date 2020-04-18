// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.superviseduser;

import android.accounts.Account;
import android.content.ContentProviderClient;
import android.content.ContentResolver;
import android.database.Cursor;
import android.net.Uri;
import android.os.RemoteException;
import android.support.test.InstrumentationRegistry;
import android.support.test.filters.SmallTest;

import org.junit.After;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.ThreadUtils;
import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.RetryOnFailure;
import org.chromium.chrome.browser.ChromeActivity;
import org.chromium.chrome.browser.ChromeSwitches;
import org.chromium.chrome.browser.preferences.PrefServiceBridge;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.test.ChromeActivityTestRule;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.chrome.test.util.browser.signin.SigninTestUtil;
import org.chromium.components.webrestrictions.browser.WebRestrictionsContentProvider;

import java.util.concurrent.Callable;
import java.util.concurrent.ExecutionException;

/**
 * Instrumentation test for SupervisedUserContentProvider.
 */
@RunWith(ChromeJUnit4ClassRunner.class)
@RetryOnFailure
@CommandLineFlags.Add({ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE})
public class SupervisedUserContentProviderTest {
    @Rule
    public ChromeActivityTestRule<ChromeActivity> mActivityTestRule =
            new ChromeActivityTestRule<>(ChromeActivity.class);

    private static final String DEFAULT_ACCOUNT = "test@gmail.com";
    private static final String AUTHORITY_SUFFIX = ".SupervisedUserProvider";
    private ContentResolver mResolver;
    private String mAuthority;
    private Uri mUri;

    @Before
    public void setUp() throws Exception {
        SigninTestUtil.setUpAuthForTest(InstrumentationRegistry.getInstrumentation());

        // In principle the SupervisedUserContentProvider should work whenever Chrome is installed
        // (even if it isn't running), but to test it we need to set up a dummy child, and to do
        // this within a test we need to start Chrome.
        mActivityTestRule.startMainActivityOnBlankPage();
        mResolver = InstrumentationRegistry.getInstrumentation()
                            .getTargetContext()
                            .getContentResolver();
        Assert.assertNotNull(mResolver);
        mAuthority = InstrumentationRegistry.getTargetContext().getPackageName() + AUTHORITY_SUFFIX;
        mUri = new Uri.Builder()
                       .scheme(ContentResolver.SCHEME_CONTENT)
                       .authority(mAuthority)
                       .path("authorized")
                       .build();
        SigninTestUtil.resetSigninState();
    }

    @After
    public void tearDown() throws Exception {
        SigninTestUtil.resetSigninState();
        SigninTestUtil.tearDownAuthForTest();
    }

    @Test
    @SmallTest
    public void testSupervisedUserDisabled() throws RemoteException, ExecutionException {
        ContentProviderClient client = mResolver.acquireContentProviderClient(mAuthority);
        Assert.assertNotNull(client);
        Cursor cursor = client.query(mUri, null, "url = 'http://google.com'", null, null);
        Assert.assertNull(cursor);
    }

    @Test
    @SmallTest
    public void testNoSupervisedUser() throws RemoteException, ExecutionException {
        Assert.assertFalse(ThreadUtils.runOnUiThreadBlocking(new Callable<Boolean>() {

            @Override
            public Boolean call() throws Exception {
                PrefServiceBridge.getInstance().setSupervisedUserId("");
                return Profile.getLastUsedProfile().isChild();
            }

        }));
        ContentProviderClient client = mResolver.acquireContentProviderClient(mAuthority);
        Assert.assertNotNull(client);
        SupervisedUserContentProvider.enableContentProviderForTesting();
        Cursor cursor = client.query(mUri, null, "url = 'http://google.com'", null, null);
        Assert.assertNotNull(cursor);
        Assert.assertEquals(WebRestrictionsContentProvider.BLOCKED, cursor.getInt(0));
        cursor = client.query(mUri, null, "url = 'http://www.notgoogle.com'", null, null);
        Assert.assertNotNull(cursor);
        Assert.assertEquals(WebRestrictionsContentProvider.BLOCKED, cursor.getInt(0));
    }

    @Test
    @SmallTest
    public void testWithSupervisedUser() throws RemoteException, ExecutionException {
        final Account account = SigninTestUtil.addAndSignInTestAccount();
        Assert.assertNotNull(account);
        Assert.assertTrue(ThreadUtils.runOnUiThreadBlocking(new Callable<Boolean>() {

            @Override
            public Boolean call() throws Exception {
                PrefServiceBridge.getInstance().setSupervisedUserId("ChildAccountSUID");
                return Profile.getLastUsedProfile().isChild();
            }

        }));
        ContentProviderClient client = mResolver.acquireContentProviderClient(mAuthority);
        Assert.assertNotNull(client);
        SupervisedUserContentProvider.enableContentProviderForTesting();
        // setFilter for testing sets a default filter that blocks by default.
        mResolver.call(mUri, "setFilterForTesting", null, null);
        Cursor cursor = client.query(mUri, null, "url = 'http://www.google.com'", null, null);
        Assert.assertNotNull(cursor);
        Assert.assertEquals(WebRestrictionsContentProvider.BLOCKED, cursor.getInt(0));
    }
}
