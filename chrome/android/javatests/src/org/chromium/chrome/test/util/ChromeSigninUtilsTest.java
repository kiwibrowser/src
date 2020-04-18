// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.test.util;

import android.support.test.InstrumentationRegistry;
import android.support.test.filters.SmallTest;

import org.junit.After;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.test.util.EnormousTest;
import org.chromium.base.test.util.FlakyTest;
import org.chromium.base.test.util.Restriction;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.components.signin.ChromeSigninController;

/**
 * Tests for {@link ChromeSigninUtils}.
 */
@RunWith(ChromeJUnit4ClassRunner.class)
public class ChromeSigninUtilsTest {
    private static final String FAKE_ACCOUNT_USERNAME = "test@google.com";
    private static final String FAKE_ACCOUNT_PASSWORD = "$3cr3t";
    private static final String GOOGLE_ACCOUNT_USERNAME = "chromiumforandroid01@gmail.com";
    private static final String GOOGLE_ACCOUNT_PASSWORD = "chromeforandroid";
    private static final String GOOGLE_ACCOUNT_TYPE = "mail";

    private ChromeSigninUtils mSigninUtil;
    private ChromeSigninController mSigninController;

    @Before
    public void setUp() throws Exception {
        mSigninUtil = new ChromeSigninUtils(InstrumentationRegistry.getInstrumentation());
        mSigninController = ChromeSigninController.get();
        mSigninController.setSignedInAccountName(null);
        mSigninUtil.removeAllFakeAccountsFromOs();
        mSigninUtil.removeAllGoogleAccountsFromOs();
    }

    @Test
    @SmallTest
    public void testActivityIsNotSignedInOnAppOrFakeOSorGoogleOS() {
        Assert.assertFalse("Should not be signed into app.", mSigninController.isSignedIn());
        Assert.assertFalse("Should not be signed into OS with fake account.",
                mSigninUtil.isExistingFakeAccountOnOs(FAKE_ACCOUNT_USERNAME));
        Assert.assertFalse("Should not be signed in on OS with Google account.",
                mSigninUtil.isExistingGoogleAccountOnOs(GOOGLE_ACCOUNT_USERNAME));
    }

    @Test
    @SmallTest
    public void testIsSignedInOnApp() {
        mSigninUtil.addAccountToApp(FAKE_ACCOUNT_USERNAME);
        Assert.assertTrue("Should be signed on app.", mSigninController.isSignedIn());
        Assert.assertFalse("Should not be signed on OS with fake account.",
                mSigninUtil.isExistingFakeAccountOnOs(FAKE_ACCOUNT_USERNAME));
        Assert.assertFalse("Should not be signed in on OS with Google account.",
                mSigninUtil.isExistingGoogleAccountOnOs(GOOGLE_ACCOUNT_USERNAME));
    }

    @Test
    @SmallTest
    public void testIsSignedInOnFakeOS() {
        mSigninUtil.addFakeAccountToOs(FAKE_ACCOUNT_USERNAME, FAKE_ACCOUNT_PASSWORD);
        Assert.assertFalse("Should not be signed in on app.", mSigninController.isSignedIn());
        Assert.assertTrue("Should be signed in on OS with fake account.",
                mSigninUtil.isExistingFakeAccountOnOs(FAKE_ACCOUNT_USERNAME));
        Assert.assertFalse("Should not be signed in on OS with Google account.",
                mSigninUtil.isExistingGoogleAccountOnOs(GOOGLE_ACCOUNT_USERNAME));
    }

    @Test
    @FlakyTest(message = "https://crbug.com/517849")
    @EnormousTest
    @Restriction(Restriction.RESTRICTION_TYPE_INTERNET)
    public void testIsSignedInOnGoogleOS() {
        mSigninUtil.addGoogleAccountToOs(GOOGLE_ACCOUNT_USERNAME, GOOGLE_ACCOUNT_PASSWORD,
                GOOGLE_ACCOUNT_TYPE);
        Assert.assertFalse("Should not be signed into app.", mSigninController.isSignedIn());
        Assert.assertFalse("Should not be signed into OS with fake account.",
                mSigninUtil.isExistingFakeAccountOnOs(FAKE_ACCOUNT_USERNAME));
        Assert.assertTrue("Should be signed in on OS with Google account.",
                mSigninUtil.isExistingGoogleAccountOnOs(GOOGLE_ACCOUNT_USERNAME));
    }

    @Test
    @SmallTest
    public void testIsSignedInOnFakeOSandApp() {
        mSigninUtil.addAccountToApp(FAKE_ACCOUNT_USERNAME);
        mSigninUtil.addFakeAccountToOs(FAKE_ACCOUNT_USERNAME, FAKE_ACCOUNT_PASSWORD);
        Assert.assertTrue("Should be signed in on app.", mSigninController.isSignedIn());
        Assert.assertTrue("Should be signed in on OS with fake account.",
                mSigninUtil.isExistingFakeAccountOnOs(FAKE_ACCOUNT_USERNAME));
        Assert.assertFalse("Should not be signed in on OS with Google account.",
                mSigninUtil.isExistingGoogleAccountOnOs(GOOGLE_ACCOUNT_USERNAME));
    }

    @Test
    @FlakyTest(message = "https://crbug.com/517849")
    @EnormousTest
    @Restriction(Restriction.RESTRICTION_TYPE_INTERNET)
    public void testIsSignedInOnAppAndGoogleOS() {
        mSigninUtil.addAccountToApp(FAKE_ACCOUNT_USERNAME);
        mSigninUtil.addGoogleAccountToOs(GOOGLE_ACCOUNT_USERNAME, GOOGLE_ACCOUNT_PASSWORD,
                GOOGLE_ACCOUNT_TYPE);
        Assert.assertTrue("Should be signed into app.", mSigninController.isSignedIn());
        Assert.assertFalse("Should not be signed into OS with fake account.",
                mSigninUtil.isExistingFakeAccountOnOs(FAKE_ACCOUNT_USERNAME));
        Assert.assertTrue("Should be signed in on OS with Google account.",
                mSigninUtil.isExistingGoogleAccountOnOs(GOOGLE_ACCOUNT_USERNAME));
    }

    @Test
    @FlakyTest(message = "https://crbug.com/517849")
    @EnormousTest
    @Restriction(Restriction.RESTRICTION_TYPE_INTERNET)
    public void testIsSignedInOnFakeOSandGoogleOS() {
        mSigninUtil.addFakeAccountToOs(FAKE_ACCOUNT_USERNAME, FAKE_ACCOUNT_PASSWORD);
        mSigninUtil.addGoogleAccountToOs(GOOGLE_ACCOUNT_USERNAME, GOOGLE_ACCOUNT_PASSWORD,
                GOOGLE_ACCOUNT_TYPE);
        Assert.assertFalse("Should not be signed into app.", mSigninController.isSignedIn());
        Assert.assertTrue("Should be signed into OS with fake account.",
                mSigninUtil.isExistingFakeAccountOnOs(FAKE_ACCOUNT_USERNAME));
        Assert.assertTrue("Should be signed in on OS with Google account.",
                mSigninUtil.isExistingGoogleAccountOnOs(GOOGLE_ACCOUNT_USERNAME));
    }

    @Test
    @FlakyTest(message = "https://crbug.com/517849")
    @EnormousTest
    @Restriction(Restriction.RESTRICTION_TYPE_INTERNET)
    public void testIsSignedInOnAppAndFakeOSandGoogleOS() {
        mSigninUtil.addAccountToApp(FAKE_ACCOUNT_USERNAME);
        mSigninUtil.addFakeAccountToOs(FAKE_ACCOUNT_USERNAME, FAKE_ACCOUNT_PASSWORD);
        mSigninUtil.addGoogleAccountToOs(GOOGLE_ACCOUNT_USERNAME, GOOGLE_ACCOUNT_PASSWORD,
                GOOGLE_ACCOUNT_TYPE);
        Assert.assertTrue("Should be signed into app.", mSigninController.isSignedIn());
        Assert.assertTrue("Should be signed into OS with fake account.",
                mSigninUtil.isExistingFakeAccountOnOs(FAKE_ACCOUNT_USERNAME));
        Assert.assertTrue("Should be signed in on OS with Google account.",
                mSigninUtil.isExistingGoogleAccountOnOs(GOOGLE_ACCOUNT_USERNAME));
    }

    @After
    public void tearDown() throws Exception {
        mSigninController.setSignedInAccountName(null);
        mSigninUtil.removeAllFakeAccountsFromOs();
        mSigninUtil.removeAllGoogleAccountsFromOs();
    }
}
