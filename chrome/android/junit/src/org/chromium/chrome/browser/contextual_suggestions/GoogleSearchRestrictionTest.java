// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.contextual_suggestions;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.spy;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.annotation.Config;

import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.content_public.browser.LoadUrlParams;
import org.chromium.content_public.browser.NavigationController;
import org.chromium.content_public.browser.NavigationEntry;
import org.chromium.content_public.browser.NavigationHistory;
import org.chromium.ui.base.PageTransition;

/** Unit tests for FetchHelper#isFromGoogleSearch. */
@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class GoogleSearchRestrictionTest {
    private static final String GOOGLE_SEARCH_URL = "https://www.google.com/search?q=foo";
    private static final String FOO_URL = "www.foo.com";
    private static final String BAR_URL = "www.bar.com";
    private static final String BAZ_URL = "www.baz.com";
    private static final String QUX_URL = "www.qux.com";
    private static final String QUUX_URL = "www.quux.com";

    /**
     * PageTransition types that are considered "accepted" when considering all navigation history.
     */
    private static final int[] ACCEPTED_TRANSITIONS = new int[] {
            PageTransition.LINK, PageTransition.MANUAL_SUBFRAME, PageTransition.FORM_SUBMIT};

    /**
     * PageTransition types that are rejected when considering all navigation history.
     */
    private static final int[] REJECTED_TRANSITIONS = new int[] {PageTransition.TYPED,
            PageTransition.AUTO_BOOKMARK, PageTransition.AUTO_SUBFRAME, PageTransition.GENERATED,
            PageTransition.AUTO_TOPLEVEL, PageTransition.RELOAD, PageTransition.KEYWORD,
            PageTransition.KEYWORD_GENERATED};

    private FetchHelper mFetchHelper;

    @Before
    public void setUp() {
        mFetchHelper = spy(new FetchHelper(null, null) {
            @Override
            public void initialize() {
                // Intentionally do nothing.
            }
        });

        doReturn(true).when(mFetchHelper).isGoogleSearchUrl(GOOGLE_SEARCH_URL);
        doReturn(false).when(mFetchHelper).isGoogleSearchUrl(FOO_URL);
        doReturn(false).when(mFetchHelper).isGoogleSearchUrl(BAR_URL);
        doReturn(false).when(mFetchHelper).isGoogleSearchUrl(BAZ_URL);
        doReturn(false).when(mFetchHelper).isGoogleSearchUrl(QUX_URL);
        doReturn(false).when(mFetchHelper).isGoogleSearchUrl(QUUX_URL);
    }

    @Test
    public void testOneNavEntry_Link() {
        NavigationController navController = createOneEntryNavController(PageTransition.LINK);
        assertFalse(mFetchHelper.isFromGoogleSearch(navController, true));
    }

    @Test
    public void testTwoNavEntries_Accepted_FromSRP() {
        for (int i = 0; i < ACCEPTED_TRANSITIONS.length; i++) {
            NavigationController navController =
                    createTwoEntryNavController(ACCEPTED_TRANSITIONS[i], true);
            assertTrue("From SRP using " + ACCEPTED_TRANSITIONS[i] + " expected to be true.",
                    mFetchHelper.isFromGoogleSearch(navController, false));
        }
    }

    @Test
    public void testTwoNavEntries_Accepted_Masked_FromSRP() {
        for (int i = 0; i < ACCEPTED_TRANSITIONS.length; i++) {
            int transition = ACCEPTED_TRANSITIONS[i] | PageTransition.FORWARD_BACK
                    | PageTransition.IS_REDIRECT_MASK;
            NavigationController navController = createTwoEntryNavController(transition, true);
            assertTrue("From SRP using " + ACCEPTED_TRANSITIONS[i] + " expected to be true.",
                    mFetchHelper.isFromGoogleSearch(navController, false));
        }
    }

    @Test
    public void testTwoNavEntries_Rejected_FromSRP() {
        for (int i = 0; i < REJECTED_TRANSITIONS.length; i++) {
            NavigationController navController =
                    createTwoEntryNavController(REJECTED_TRANSITIONS[i], false);
            assertFalse("From SRP using " + REJECTED_TRANSITIONS[i] + " expected to be false.",
                    mFetchHelper.isFromGoogleSearch(navController, false));
        }
    }

    @Test
    public void testTwoNavEntries_Rejected_Masked_FromSRP() {
        for (int i = 0; i < REJECTED_TRANSITIONS.length; i++) {
            int transition = REJECTED_TRANSITIONS[i] | PageTransition.FORWARD_BACK
                    | PageTransition.IS_REDIRECT_MASK;
            NavigationController navController = createTwoEntryNavController(transition, false);
            assertFalse("From SRP using " + REJECTED_TRANSITIONS[i] + " expected to be false.",
                    mFetchHelper.isFromGoogleSearch(navController, false));
        }
    }

    @Test
    public void testTwoNavEntries_Link_FromBar() {
        NavigationController navController =
                createTwoEntryNavController(PageTransition.LINK, false);
        assertFalse(mFetchHelper.isFromGoogleSearch(navController, true));
    }

    @Test
    public void testLongNavHistory_Link_FromGoogle_OnlyConsiderCurrent() {
        NavigationController navController =
                createLongNavHistory(PageTransition.LINK, PageTransition.LINK, true);
        assertFalse(mFetchHelper.isFromGoogleSearch(navController, true));
    }

    @Test
    public void testLongNavHistory_Link_FromBar_OnlyConsiderCurrent() {
        NavigationController navController =
                createLongNavHistory(PageTransition.LINK, PageTransition.LINK, false);
        assertFalse(mFetchHelper.isFromGoogleSearch(navController, true));
    }

    @Test
    public void testLongNavHistory_Accepted_Accepted_FromSRP_ConsiderAllHistory() {
        for (int i = 0; i < ACCEPTED_TRANSITIONS.length; i++) {
            NavigationController navController =
                    createLongNavHistory(PageTransition.LINK, ACCEPTED_TRANSITIONS[i], true);
            assertTrue("From SRP using " + ACCEPTED_TRANSITIONS[i] + " expected to be true.",
                    mFetchHelper.isFromGoogleSearch(navController, false));
        }
    }

    @Test
    public void testLongNavHistory_Accepted_Rejected_FromSRP_ConsiderAllHistory() {
        for (int i = 0; i < REJECTED_TRANSITIONS.length; i++) {
            NavigationController navController =
                    createLongNavHistory(PageTransition.LINK, REJECTED_TRANSITIONS[i], true);
            assertFalse("From SRP using " + REJECTED_TRANSITIONS[i] + " expected to be false.",
                    mFetchHelper.isFromGoogleSearch(navController, false));
        }
    }

    @Test
    public void testLongNavHistory_Link_FromBar_ConsiderAllHistory() {
        NavigationController navController =
                createLongNavHistory(PageTransition.LINK, PageTransition.LINK, false);
        assertFalse(mFetchHelper.isFromGoogleSearch(navController, false));
    }

    /**
     * Creates a {@link NavigationController} for a navigation history with a single entry.
     *
     * @param pageTransition The {@link PageTransition} type for the entry.
     * @return A {@link NavigationController} with one entry.
     */
    private NavigationController createOneEntryNavController(int pageTransition) {
        NavigationEntry entry =
                new NavigationEntry(1, FOO_URL, FOO_URL, FOO_URL, "Foo", null, pageTransition);

        NavigationHistory navHistory = new NavigationHistory();
        navHistory.addEntry(entry);
        navHistory.setCurrentEntryIndex(0);

        return new FakeNavigationController(navHistory);
    }

    /**
     * Creates a {@link NavigationController} for a navigation history with two entries.
     *
     * @param pageTransition The {@link PageTransition} type for the current nav entry.
     * @param fromSRP Whether the first entry should be a Google search results page.
     * @return A {@link NavigationController} with two entries.
     */
    private NavigationController createTwoEntryNavController(int pageTransition, boolean fromSRP) {
        NavigationEntry firstEntry;
        if (fromSRP) {
            firstEntry = new NavigationEntry(0, GOOGLE_SEARCH_URL, GOOGLE_SEARCH_URL,
                    GOOGLE_SEARCH_URL, "foo - Google Search", null, PageTransition.TYPED);
        } else {
            firstEntry = new NavigationEntry(
                    0, BAR_URL, BAR_URL, BAR_URL, "bar", null, PageTransition.LINK);
        }

        NavigationEntry currentEntry =
                new NavigationEntry(1, FOO_URL, FOO_URL, FOO_URL, "Foo", null, pageTransition);

        NavigationHistory navHistory = new NavigationHistory();
        navHistory.addEntry(firstEntry);
        navHistory.addEntry(currentEntry);
        navHistory.setCurrentEntryIndex(1);

        return new FakeNavigationController(navHistory);
    }

    /**
     * Creates {@link NavigationController} with a longer navigation history.
     *
     * @param previousPageTransition The {@link PageTransition} type for the nav entry before the
     *                               current entry.
     * @param currentPageTransition The {@link PageTransition} type for the current nav entry.
     * @param fromSRP Whether the navigation stack should originate from a Google search results
     * page.
     * @return A {@link NavigationController} with multiple entries.
     */
    private NavigationController createLongNavHistory(
            int previousPageTransition, int currentPageTransition, boolean fromSRP) {
        NavigationEntry firstEntry =
                new NavigationEntry(0, BAZ_URL, BAZ_URL, BAZ_URL, "baz", null, PageTransition.LINK);
        NavigationEntry secondEntry;
        if (fromSRP) {
            secondEntry = new NavigationEntry(1, GOOGLE_SEARCH_URL, GOOGLE_SEARCH_URL,
                    GOOGLE_SEARCH_URL, "foo - Google Search", null, PageTransition.LINK);
        } else {
            secondEntry = new NavigationEntry(
                    1, BAR_URL, BAR_URL, BAR_URL, "bar", null, PageTransition.LINK);
        }

        NavigationEntry thirdEntry = new NavigationEntry(
                2, FOO_URL, FOO_URL, FOO_URL, "Foo", null, previousPageTransition);

        NavigationEntry fourthEntry = new NavigationEntry(3, QUX_URL, QUX_URL, QUX_URL, "qux", null,
                currentPageTransition | PageTransition.FORWARD_BACK);

        NavigationEntry fifthEntry = new NavigationEntry(
                4, QUUX_URL, QUUX_URL, QUUX_URL, "quux", null, PageTransition.TYPED);

        NavigationHistory navHistory = new NavigationHistory();
        navHistory.addEntry(firstEntry);
        navHistory.addEntry(secondEntry);
        navHistory.addEntry(thirdEntry);
        navHistory.addEntry(fourthEntry);
        navHistory.addEntry(fifthEntry);
        navHistory.setCurrentEntryIndex(3);

        return new FakeNavigationController(navHistory);
    }

    private class FakeNavigationController implements NavigationController {
        private NavigationHistory mNavHistory;

        public FakeNavigationController(NavigationHistory navHistory) {
            mNavHistory = navHistory;
        }

        @Override
        public NavigationEntry getEntryAtIndex(int index) {
            return mNavHistory.getEntryAtIndex(index);
        }

        @Override
        public int getLastCommittedEntryIndex() {
            return mNavHistory.getCurrentEntryIndex();
        }

        // Dummy implementations.

        @Override
        public boolean canGoBack() {
            return false;
        }

        @Override
        public boolean canGoForward() {
            return false;
        }

        @Override
        public boolean canGoToOffset(int offset) {
            return false;
        }

        @Override
        public void goToOffset(int offset) {}

        @Override
        public void goToNavigationIndex(int index) {}

        @Override
        public void goBack() {}

        @Override
        public void goForward() {}

        @Override
        public boolean isInitialNavigation() {
            return false;
        }

        @Override
        public void loadIfNecessary() {}

        @Override
        public boolean needsReload() {
            return false;
        }

        @Override
        public void setNeedsReload() {}

        @Override
        public void reload(boolean checkForRepost) {}

        @Override
        public void reloadBypassingCache(boolean checkForRepost) {}

        @Override
        public void cancelPendingReload() {}

        @Override
        public void continuePendingReload() {}

        @Override
        public void loadUrl(LoadUrlParams params) {}

        @Override
        public void clearHistory() {}

        @Override
        public NavigationHistory getNavigationHistory() {
            return null;
        }

        @Override
        public NavigationHistory getDirectedNavigationHistory(boolean isForward, int itemLimit) {
            return null;
        }

        @Override
        public String getOriginalUrlForVisibleNavigationEntry() {
            return null;
        }

        @Override
        public void clearSslPreferences() {}

        @Override
        public boolean getUseDesktopUserAgent() {
            return false;
        }

        @Override
        public void setUseDesktopUserAgent(boolean override, boolean reloadOnChange) {}

        @Override
        public NavigationEntry getPendingEntry() {
            return null;
        }

        @Override
        public boolean removeEntryAtIndex(int index) {
            return false;
        }

        @Override
        public boolean canCopyStateOver() {
            return false;
        }

        @Override
        public boolean canPruneAllButLastCommitted() {
            return false;
        }

        @Override
        public void copyStateFrom(NavigationController source, boolean needsReload) {}

        @Override
        public void copyStateFromAndPrune(NavigationController source, boolean replaceEntry) {}

        @Override
        public String getEntryExtraData(int index, String key) {
            return null;
        }

        @Override
        public void setEntryExtraData(int index, String key, String value) {}
    }
}
