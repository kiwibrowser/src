// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.bookmarks;

import static android.support.test.espresso.Espresso.onView;
import static android.support.test.espresso.Espresso.pressBack;
import static android.support.test.espresso.action.ViewActions.click;
import static android.support.test.espresso.assertion.ViewAssertions.doesNotExist;
import static android.support.test.espresso.assertion.ViewAssertions.matches;
import static android.support.test.espresso.matcher.ViewMatchers.isDisplayed;
import static android.support.test.espresso.matcher.ViewMatchers.withId;

import static junit.framework.Assert.assertEquals;
import static junit.framework.Assert.assertFalse;
import static junit.framework.Assert.assertTrue;

import android.accounts.Account;
import android.content.Intent;
import android.support.test.filters.LargeTest;
import android.support.test.filters.MediumTest;
import android.support.test.runner.intent.IntentCallback;
import android.support.test.runner.intent.IntentMonitorRegistry;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.ThreadUtils;
import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.DisableIf;
import org.chromium.base.test.util.RetryOnFailure;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ChromeActivity;
import org.chromium.chrome.browser.ChromeSwitches;
import org.chromium.chrome.browser.signin.AccountSigninActivity;
import org.chromium.chrome.browser.signin.SigninAccessPoint;
import org.chromium.chrome.browser.signin.SigninPromoController;
import org.chromium.chrome.test.ChromeActivityTestRule;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.components.signin.AccountManagerFacade;
import org.chromium.components.signin.ProfileDataSource;
import org.chromium.components.signin.test.util.AccountHolder;
import org.chromium.components.signin.test.util.FakeAccountManagerDelegate;
import org.chromium.ui.test.util.UiDisableIf;

import java.io.Closeable;
import java.util.ArrayList;
import java.util.List;

/**
 * Tests for the personalized signin promo on the Bookmarks page.
 */
@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags.Add({ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE})
@RetryOnFailure(message = "crbug.com/789531")
public class BookmarkPersonalizedSigninPromoTest {
    private static final String TEST_ACCOUNT_NAME = "test@gmail.com";
    private static final String TEST_FULL_NAME = "Test Account";

    @Rule
    public final ChromeActivityTestRule<ChromeActivity> mActivityTestRule =
            new ChromeActivityTestRule<>(ChromeActivity.class);

    private final FakeAccountManagerDelegate mAccountManagerDelegate =
            new FakeAccountManagerDelegate(FakeAccountManagerDelegate.ENABLE_PROFILE_DATA_SOURCE);

    @Before
    public void setUp() throws Exception {
        AccountManagerFacade.overrideAccountManagerFacadeForTests(mAccountManagerDelegate);
        mActivityTestRule.startMainActivityFromLauncher();
    }

    @Test
    @MediumTest
    public void testManualDismissPromo() throws Exception {
        openBookmarkManager();
        onView(withId(R.id.signin_promo_view_container)).check(matches(isDisplayed()));
        onView(withId(R.id.signin_promo_close_button)).perform(click());
        onView(withId(R.id.signin_promo_view_container)).check(doesNotExist());
    }

    // If this starts flaking again, disable the test. See crbug.com/789531.
    @Test
    @LargeTest
    @DisableIf.Device(type = {UiDisableIf.TABLET}) // https://crbug.com/776405.
    public void testAutoDismissPromo() throws Exception {
        int impressionCap = SigninPromoController.getMaxImpressionsBookmarksForTests();
        for (int impression = 0; impression < impressionCap; impression++) {
            openBookmarkManager();
            onView(withId(R.id.signin_promo_view_container)).check(matches(isDisplayed()));
            pressBack();
        }
        openBookmarkManager();
        onView(withId(R.id.signin_promo_view_container)).check(doesNotExist());
    }

    @Test
    @MediumTest
    public void testSigninButtonDefaultAccount() throws Exception {
        addTestAccount();
        openBookmarkManager();
        onView(withId(R.id.signin_promo_view_container)).check(matches(isDisplayed()));

        final List<Intent> startedIntents;
        try (IntentCallbackHelper helper = new IntentCallbackHelper()) {
            onView(withId(R.id.signin_promo_signin_button)).perform(click());
            startedIntents = helper.getStartedIntents();
        }

        assertEquals("Choosing to sign in with the default account should fire an intent!", 1,
                startedIntents.size());
        Intent expectedIntent = AccountSigninActivity.createIntentForConfirmationOnlySigninFlow(
                mActivityTestRule.getActivity(), SigninAccessPoint.BOOKMARK_MANAGER,
                TEST_ACCOUNT_NAME, true, true);
        assertTrue(expectedIntent.filterEquals(startedIntents.get(0)));
    }

    @Test
    @MediumTest
    public void testSigninButtonNotDefaultAccount() throws Exception {
        addTestAccount();
        openBookmarkManager();
        onView(withId(R.id.signin_promo_view_container)).check(matches(isDisplayed()));

        final List<Intent> startedIntents;
        try (IntentCallbackHelper helper = new IntentCallbackHelper()) {
            onView(withId(R.id.signin_promo_choose_account_button)).perform(click());
            startedIntents = helper.getStartedIntents();
        }

        assertEquals("Choosing to sign in with another account should fire an intent!", 1,
                startedIntents.size());
        Intent expectedIntent = AccountSigninActivity.createIntentForDefaultSigninFlow(
                mActivityTestRule.getActivity(), SigninAccessPoint.BOOKMARK_MANAGER, true);
        assertTrue(expectedIntent.filterEquals(startedIntents.get(0)));
    }

    @Test
    @MediumTest
    public void testSigninButtonNewAccount() throws Exception {
        openBookmarkManager();
        onView(withId(R.id.signin_promo_view_container)).check(matches(isDisplayed()));

        final List<Intent> startedIntents;
        try (IntentCallbackHelper helper = new IntentCallbackHelper()) {
            onView(withId(R.id.signin_promo_signin_button)).perform(click());
            startedIntents = helper.getStartedIntents();
        }

        assertFalse(
                "Adding a new account should fire at least one intent!", startedIntents.isEmpty());
        Intent expectedIntent = AccountSigninActivity.createIntentForAddAccountSigninFlow(
                mActivityTestRule.getActivity(), SigninAccessPoint.BOOKMARK_MANAGER, true);
        // Comparing only the first intent as AccountSigninActivity will fire an intent after
        // starting the flow to add an account.
        assertTrue(expectedIntent.filterEquals(startedIntents.get(0)));
    }

    private void openBookmarkManager() throws InterruptedException {
        ThreadUtils.runOnUiThreadBlocking(
                () -> BookmarkUtils.showBookmarkManager(mActivityTestRule.getActivity()));
    }

    private void addTestAccount() {
        Account account = AccountManagerFacade.createAccountFromName(TEST_ACCOUNT_NAME);
        AccountHolder.Builder accountHolder = AccountHolder.builder(account).alwaysAccept(true);
        mAccountManagerDelegate.addAccountHolderBlocking(accountHolder.build());
        ProfileDataSource.ProfileData profileData =
                new ProfileDataSource.ProfileData(TEST_ACCOUNT_NAME, null, TEST_FULL_NAME, null);
        ThreadUtils.runOnUiThreadBlocking(
                () -> mAccountManagerDelegate.setProfileData(TEST_ACCOUNT_NAME, profileData));
    }

    private static class IntentCallbackHelper implements IntentCallback, Closeable {
        private final List<Intent> mStartedIntents = new ArrayList<>();

        public IntentCallbackHelper() {
            IntentMonitorRegistry.getInstance().addIntentCallback(this);
        }

        @Override
        public void onIntentSent(Intent intent) {
            mStartedIntents.add(intent);
        }

        @Override
        public void close() {
            IntentMonitorRegistry.getInstance().removeIntentCallback(this);
        }

        public List<Intent> getStartedIntents() {
            return mStartedIntents;
        }
    }
}
