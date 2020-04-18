// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.suggestions;

import static android.support.test.espresso.Espresso.onView;
import static android.support.test.espresso.action.ViewActions.longClick;
import static android.support.test.espresso.contrib.RecyclerViewActions.actionOnItemAtPosition;
import static android.support.test.espresso.matcher.ViewMatchers.withId;

import static org.chromium.chrome.test.BottomSheetTestRule.waitForWindowUpdates;

import android.support.test.filters.MediumTest;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.test.util.DisabledTest;
import org.chromium.base.test.util.Feature;
import org.chromium.base.test.util.Restriction;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ntp.NtpUiCaptureTestData;
import org.chromium.chrome.browser.ntp.cards.ItemViewType;
import org.chromium.chrome.browser.ntp.snippets.CategoryStatus;
import org.chromium.chrome.browser.ntp.snippets.KnownCategories;
import org.chromium.chrome.browser.preferences.ChromePreferenceManager;
import org.chromium.chrome.browser.test.ScreenShooter;
import org.chromium.chrome.browser.widget.bottomsheet.BottomSheet;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.chrome.test.util.browser.suggestions.FakeSuggestionsSource;
import org.chromium.chrome.test.util.browser.suggestions.SuggestionsDependenciesRule;
import org.chromium.chrome.test.util.browser.suggestions.SuggestionsDependenciesRule.TestFactory;
import org.chromium.ui.test.util.UiRestriction;

import java.util.Collections;

/**
 * Tests for the appearance of the card suggestions in the home sheet.
 */
@DisabledTest(message = "https://crbug.com/805160")
@RunWith(ChromeJUnit4ClassRunner.class)
@Restriction(UiRestriction.RESTRICTION_TYPE_PHONE) // ChromeHome is only enabled on phones
public class HomeSheetCardsUiCaptureTest {
    @Rule
    public SuggestionsBottomSheetTestRule mActivityRule = new SuggestionsBottomSheetTestRule();

    private final TestFactory mDepsFactory = NtpUiCaptureTestData.createFactory();

    @Rule
    public SuggestionsDependenciesRule setupSuggestions() {
        return new SuggestionsDependenciesRule(mDepsFactory);
    }

    @Rule
    public ScreenShooter mScreenShooter = new ScreenShooter();

    @Before
    public void setup() throws InterruptedException {
        ChromePreferenceManager.getInstance().setNewTabPageSigninPromoDismissed(true);
        mActivityRule.startMainActivityOnBottomSheet(BottomSheet.SHEET_STATE_PEEK);
    }

    @Test
    @MediumTest
    @Feature({"UiCatalogue"})
    public void testContextMenu() throws Exception {
        mActivityRule.setSheetState(BottomSheet.SHEET_STATE_FULL, false);
        waitForWindowUpdates();

        int position = mActivityRule.getFirstPositionForType(ItemViewType.SNIPPET);
        onView(withId(R.id.recycler_view)).perform(actionOnItemAtPosition(position, longClick()));
        mScreenShooter.shoot("ContextMenu");
    }

    @Test
    @MediumTest
    @Feature({"UiCatalogue"})
    public void testScrolling() throws Exception {
        mActivityRule.setSheetState(BottomSheet.SHEET_STATE_FULL, false);
        waitForWindowUpdates();

        // When scrolling to a View, we wait until the View is no longer updating - when it is no
        // longer dirty. If scroll to load is triggered, the animated progress spinner will keep
        // the RecyclerView dirty as it is constantly updating. In addition for the UiCaptureTest
        // we would like to get to the bottom of the CardsUI.

        // We do not want to disable the Scroll to Load feature entirely because its presence
        // effects other elements of the UI - it moves the Learn More link into the Context Menu.
        // Removing the ScrollToLoad listener from the RecyclerView allows us to prevent scroll to
        // load triggering while maintaining the UI otherwise.
        mActivityRule.getRecyclerView().clearScrollToLoadListener();

        mActivityRule.scrollToFirstItemOfType(ItemViewType.ACTION);
        waitForWindowUpdates();
        mScreenShooter.shoot("ScrolledToMoreButton");

        mActivityRule.scrollToFirstItemOfType(ItemViewType.SNIPPET);
        waitForWindowUpdates();
        mScreenShooter.shoot("ScrolledToFirstCard");
    }

    @Test
    @MediumTest
    @Feature({"UiCatalogue"})
    public void testContentSuggestionPlaceholder() throws Exception {
        FakeSuggestionsSource source = (FakeSuggestionsSource) mDepsFactory.suggestionsSource;
        source.setSuggestionsForCategory(KnownCategories.ARTICLES, Collections.emptyList());
        source.setStatusForCategory(KnownCategories.ARTICLES, CategoryStatus.AVAILABLE_LOADING);

        mActivityRule.setSheetState(BottomSheet.SHEET_STATE_FULL, false);
        waitForWindowUpdates();

        mScreenShooter.shoot("ContentSuggestionsPlaceholder");
    }
}
