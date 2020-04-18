// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.util;

import android.accounts.Account;
import android.accounts.AuthenticatorDescription;
import android.content.Intent;
import android.content.pm.ResolveInfo;
import android.speech.RecognizerIntent;
import android.support.test.InstrumentationRegistry;
import android.support.test.filters.SmallTest;
import android.test.mock.MockContext;
import android.test.mock.MockPackageManager;

import org.junit.After;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.ThreadUtils;
import org.chromium.base.test.util.AdvancedMockContext;
import org.chromium.base.test.util.Feature;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.components.signin.AccountManagerFacade;
import org.chromium.components.signin.test.util.AccountHolder;
import org.chromium.components.signin.test.util.FakeAccountManagerDelegate;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.Callable;

/**
 * Unit Test for FeatureUtilities.
 */
@RunWith(ChromeJUnit4ClassRunner.class)
public class FeatureUtilitiesTest {
    private IntentTestMockContext mContextWithSpeech;
    private IntentTestMockContext mContextWithoutSpeech;
    private FakeAuthenticationAccountManager mAccountManager;
    private AdvancedMockContext mAccountTestingContext;
    private Account mTestAccount;

    public FeatureUtilitiesTest() {
        mContextWithSpeech = new IntentTestMockContext(
                RecognizerIntent.ACTION_RECOGNIZE_SPEECH);

        mContextWithoutSpeech = new IntentTestMockContext(
                RecognizerIntent.ACTION_WEB_SEARCH);

        mTestAccount = AccountManagerFacade.createAccountFromName("Dummy");
    }

    @Before
    public void setUp() {
        // GetInstrumentation().getTargetContext() cannot be called in
        // constructor due to external dependencies.
        mAccountTestingContext =
                new AdvancedMockContext(InstrumentationRegistry.getTargetContext());
    }

    @After
    public void tearDown() {
        AccountManagerFacade.resetAccountManagerFacadeForTests();
    }

    private static class IntentTestPackageManager extends MockPackageManager {

        private final String mAction;

        public IntentTestPackageManager(String recognizesAction) {
            super();
            mAction = recognizesAction;
        }

        @Override
        public List<ResolveInfo> queryIntentActivities(Intent intent, int flags) {
            List<ResolveInfo> resolveInfoList = new ArrayList<ResolveInfo>();

            if (intent.getAction().equals(mAction)) {
                // Add an entry to the returned list as the action
                // being queried exists.
                ResolveInfo resolveInfo = new ResolveInfo();
                resolveInfoList.add(resolveInfo);
            }

            return resolveInfoList;
        }
    }

    private static class IntentTestMockContext extends MockContext {

        private final String mAction;

        public IntentTestMockContext(String recognizesAction) {
            super();
            mAction = recognizesAction;
        }

        @Override
        public IntentTestPackageManager getPackageManager() {
            return new IntentTestPackageManager(mAction);
        }
    }

    private static class FakeAuthenticationAccountManager extends FakeAccountManagerDelegate {
        private final String mAccountType;

        public FakeAuthenticationAccountManager(String accountType) {
            super(FakeAccountManagerDelegate.DISABLE_PROFILE_DATA_SOURCE);
            mAccountType = accountType;
        }

        @Override
        public AuthenticatorDescription[] getAuthenticatorTypes() {
            AuthenticatorDescription googleAuthenticator =
                    new AuthenticatorDescription(mAccountType, "p1", 0, 0, 0, 0);

            return new AuthenticatorDescription[] { googleAuthenticator };
        }
    }

    private static boolean isRecognitionIntentPresent(
            final IntentTestMockContext context, final boolean useCachedResult) {
        // Context can only be queried on a UI Thread.
        return ThreadUtils.runOnUiThreadBlockingNoException(
            new Callable<Boolean>() {
                @Override
                public Boolean call() {
                    return FeatureUtilities.isRecognitionIntentPresent(
                            context,
                            useCachedResult);
                }
            });
    }

    private void setUpAccountManager(String accountType) {
        mAccountManager = new FakeAuthenticationAccountManager(accountType);
        AccountManagerFacade.overrideAccountManagerFacadeForTests(mAccountManager);
    }

    private void addTestAccount() {
        mAccountManager.addAccountHolderBlocking(
                AccountHolder.builder(mTestAccount).alwaysAccept(true).build());
    }

    @Test
    @SmallTest
    @Feature({"FeatureUtilities", "Speech"})
    public void testSpeechFeatureAvailable() {
        final boolean doNotUseCachedResult = false;
        final boolean recognizesSpeech = isRecognitionIntentPresent(
                mContextWithSpeech,
                doNotUseCachedResult);

        Assert.assertTrue(recognizesSpeech);
    }

    @Test
    @SmallTest
    @Feature({"FeatureUtilities", "Speech"})
    public void testSpeechFeatureUnavailable() {
        final boolean doNotUseCachedResult = false;
        final boolean recognizesSpeech = isRecognitionIntentPresent(
                mContextWithoutSpeech,
                doNotUseCachedResult);

        Assert.assertFalse(recognizesSpeech);
    }

    @Test
    @SmallTest
    @Feature({"FeatureUtilities", "Speech"})
    public void testCachedSpeechFeatureAvailability() {
        // Initial call will cache the fact that speech is recognized.
        final boolean doNotUseCachedResult = false;
        isRecognitionIntentPresent(
                mContextWithSpeech,
                doNotUseCachedResult);

        // Pass a context that does not recognize speech, but use cached result
        // which does recognize speech.
        final boolean useCachedResult = true;
        final boolean recognizesSpeech = isRecognitionIntentPresent(
                mContextWithoutSpeech,
                useCachedResult);

        // Check that we still recognize speech as we're using cached result.
        Assert.assertTrue(recognizesSpeech);

        // Check if we can turn cached result off again.
        final boolean RecognizesSpeechUncached = isRecognitionIntentPresent(
                mContextWithoutSpeech,
                doNotUseCachedResult);

        Assert.assertFalse(RecognizesSpeechUncached);
    }

    // This test previously flaked on the try bot: http://crbug.com/543160.
    // Re-enabling this test since there has been related cleanup/refactoring
    // during the time the test was disabled. If the test starts flaking again,
    // re-open the bug.
    // TODO(nyquist): Remove this if the test is not flaky anymore.
    @Test
    @SmallTest
    @Feature({"FeatureUtilities", "GoogleAccounts"})
    public void testHasGoogleAccountCorrectlyDetected() {
        // Set up an account manager mock that returns Google account types
        // when queried.
        setUpAccountManager(AccountManagerFacade.GOOGLE_ACCOUNT_TYPE);
        addTestAccount();

        boolean hasAccounts = FeatureUtilities.hasGoogleAccounts(
                mAccountTestingContext);

        Assert.assertTrue(hasAccounts);

        boolean hasAuthenticator = FeatureUtilities.hasGoogleAccountAuthenticator(
                mAccountTestingContext);

        Assert.assertTrue(hasAuthenticator);
    }

    // This test previously flaked on the try bot: http://crbug.com/543160.
    // Re-enabling this test since there has been related cleanup/refactoring
    // during the time the test was disabled. If the test starts flaking again,
    // re-open the bug.
    // TODO(nyquist): Remove this if the test is not flaky anymore.
    @Test
    @SmallTest
    @Feature({"FeatureUtilities", "GoogleAccounts"})
    public void testHasNoGoogleAccountCorrectlyDetected() {
        // Set up an account manager mock that doesn't have any accounts and doesn't have Google
        // account authenticator.
        setUpAccountManager("Not A Google Account");

        boolean hasAccounts = FeatureUtilities.hasGoogleAccounts(
                mAccountTestingContext);

        Assert.assertFalse(hasAccounts);

        boolean hasAuthenticator = FeatureUtilities.hasGoogleAccountAuthenticator(
                mAccountTestingContext);

        Assert.assertFalse(hasAuthenticator);
    }
}
