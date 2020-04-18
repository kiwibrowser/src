// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.bookmarks;

import android.support.annotation.Nullable;
import android.support.test.InstrumentationRegistry;
import android.support.test.filters.MediumTest;
import android.support.test.filters.SmallTest;
import android.support.v7.widget.RecyclerView;
import android.text.TextUtils;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import org.junit.After;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TestRule;
import org.junit.runner.RunWith;

import org.chromium.base.ActivityState;
import org.chromium.base.ApplicationStatus;
import org.chromium.base.ThreadUtils;
import org.chromium.base.test.util.CallbackHelper;
import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.DisableIf;
import org.chromium.base.test.util.Feature;
import org.chromium.base.test.util.Restriction;
import org.chromium.base.test.util.RetryOnFailure;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ChromeActivity;
import org.chromium.chrome.browser.ChromeSwitches;
import org.chromium.chrome.browser.UrlConstants;
import org.chromium.chrome.browser.bookmarks.BookmarkBridge.BookmarkItem;
import org.chromium.chrome.browser.partnerbookmarks.PartnerBookmarksShim;
import org.chromium.chrome.browser.widget.selection.SelectableListToolbar;
import org.chromium.chrome.test.ChromeActivityTestRule;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.chrome.test.util.ActivityUtils;
import org.chromium.chrome.test.util.BookmarkTestUtil;
import org.chromium.chrome.test.util.ChromeTabUtils;
import org.chromium.chrome.test.util.MenuUtils;
import org.chromium.chrome.test.util.RenderTestRule;
import org.chromium.chrome.test.util.browser.ChromeModernDesign;
import org.chromium.components.bookmarks.BookmarkId;
import org.chromium.components.bookmarks.BookmarkType;
import org.chromium.content.browser.test.util.TouchCommon;
import org.chromium.net.test.EmbeddedTestServer;
import org.chromium.ui.test.util.UiRestriction;

import java.util.ArrayList;
import java.util.concurrent.Callable;
import java.util.concurrent.ExecutionException;

/**
 * Tests for the bookmark manager.
 */
@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags.Add({ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE})
@RetryOnFailure
public class BookmarkTest {
    @Rule
    public ChromeActivityTestRule<ChromeActivity> mActivityTestRule =
            new ChromeActivityTestRule<>(ChromeActivity.class);

    @Rule
    public RenderTestRule mRenderTestRule = new RenderTestRule();

    @Rule
    public TestRule mChromeModernDesignStateRule = new ChromeModernDesign.Processor();

    private static final String TEST_PAGE_URL_GOOGLE = "/chrome/test/data/android/google.html";
    private static final String TEST_PAGE_TITLE_GOOGLE = "The Google";
    private static final String TEST_PAGE_TITLE_GOOGLE2 = "Google";
    private static final String TEST_PAGE_URL_FOO = "/chrome/test/data/android/test.html";
    private static final String TEST_PAGE_TITLE_FOO = "Foo";
    private static final String TEST_FOLDER_TITLE = "Test folder";

    private BookmarkManager mManager;
    private BookmarkModel mBookmarkModel;
    private RecyclerView mItemsContainer;
    private String mTestPage;
    private String mTestPageFoo;
    private EmbeddedTestServer mTestServer;
    private @Nullable BookmarkActivity mBookmarkActivity;

    @Before
    public void setUp() throws Exception {
        mActivityTestRule.startMainActivityFromLauncher();
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                mBookmarkModel = new BookmarkModel(
                        mActivityTestRule.getActivity().getActivityTab().getProfile());
            }
        });
        mTestServer = EmbeddedTestServer.createAndStartServer(InstrumentationRegistry.getContext());
        mTestPage = mTestServer.getURL(TEST_PAGE_URL_GOOGLE);
        mTestPageFoo = mTestServer.getURL(TEST_PAGE_URL_FOO);
    }

    private void readPartnerBookmarks() throws InterruptedException {
        // Do not read partner bookmarks in setUp(), so that the lazy reading is covered.
        ThreadUtils.runOnUiThreadBlocking(
                () -> PartnerBookmarksShim.kickOffReading(mActivityTestRule.getActivity()));
        BookmarkTestUtil.waitForBookmarkModelLoaded();
    }

    @After
    public void tearDown() throws Exception {
        mTestServer.stopAndDestroyServer();
    }

    private void openBookmarkManager() throws InterruptedException {
        if (mActivityTestRule.getActivity().isTablet()) {
            mActivityTestRule.loadUrl(UrlConstants.BOOKMARKS_URL);
            mItemsContainer =
                    (RecyclerView) mActivityTestRule.getActivity().findViewById(R.id.recycler_view);
            mManager = ((BookmarkPage) mActivityTestRule.getActivity()
                                .getActivityTab()
                                .getNativePage())
                               .getManagerForTesting();
        } else {
            // phone
            mBookmarkActivity = ActivityUtils.waitForActivity(
                    InstrumentationRegistry.getInstrumentation(), BookmarkActivity.class,
                    new MenuUtils.MenuActivityTrigger(InstrumentationRegistry.getInstrumentation(),
                            mActivityTestRule.getActivity(), R.id.all_bookmarks_menu_id));
            mItemsContainer = (RecyclerView) mBookmarkActivity.findViewById(R.id.recycler_view);
            mManager = mBookmarkActivity.getManagerForTesting();
        }
    }

    private boolean isItemPresentInBookmarkList(final String expectedTitle) {
        return ThreadUtils.runOnUiThreadBlockingNoException(new Callable<Boolean>() {
            @Override
            public Boolean call() throws Exception {
                for (int i = 0; i < mItemsContainer.getAdapter().getItemCount(); i++) {
                    BookmarkId item =
                            ((BookmarkItemsAdapter) mItemsContainer.getAdapter()).getItem(i);
                    if (item == null) continue;

                    String actualTitle = mBookmarkModel.getBookmarkTitle(item);
                    if (TextUtils.equals(actualTitle, expectedTitle)) {
                        return true;
                    }
                }
                return false;
            }
        });
    }

    @Test
    @SmallTest
    @DisableIf.Build(sdk_is_less_than = 21, message = "crbug.com/807807")
    public void testAddBookmark() throws InterruptedException {
        mActivityTestRule.loadUrl(mTestPage);
        // Check partner bookmarks are lazily loaded.
        Assert.assertFalse(mBookmarkModel.isBookmarkModelLoaded());
        // Click star button to bookmark the current tab.
        MenuUtils.invokeCustomMenuActionSync(InstrumentationRegistry.getInstrumentation(),
                mActivityTestRule.getActivity(), R.id.bookmark_this_page_id);
        BookmarkTestUtil.waitForBookmarkModelLoaded();
        // All actions with BookmarkModel needs to run on UI thread.
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                long bookmarkIdLong =
                        mActivityTestRule.getActivity().getActivityTab().getUserBookmarkId();
                BookmarkId id = new BookmarkId(bookmarkIdLong, BookmarkType.NORMAL);
                Assert.assertTrue("The test page is not added as bookmark: ",
                        mBookmarkModel.doesBookmarkExist(id));
                BookmarkItem item = mBookmarkModel.getBookmarkById(id);
                Assert.assertEquals(mBookmarkModel.getDefaultFolder(), item.getParentId());
                Assert.assertEquals(mTestPage, item.getUrl());
                Assert.assertEquals(TEST_PAGE_TITLE_GOOGLE, item.getTitle());
            }
        });
    }

    @Test
    @SmallTest
    public void testOpenBookmark() throws InterruptedException, ExecutionException {
        addBookmark(TEST_PAGE_TITLE_GOOGLE, mTestPage);
        openBookmarkManager();
        Assert.assertTrue("Grid view does not contain added bookmark: ",
                isItemPresentInBookmarkList(TEST_PAGE_TITLE_GOOGLE));
        final View tile = getViewWithText(mItemsContainer, TEST_PAGE_TITLE_GOOGLE);
        ChromeTabUtils.waitForTabPageLoaded(mActivityTestRule.getActivity().getActivityTab(),
                () -> TouchCommon.singleClickView(tile));
        Assert.assertEquals(TEST_PAGE_TITLE_GOOGLE,
                mActivityTestRule.getActivity().getActivityTab().getTitle());
    }

    @Test
    @SmallTest
    public void testUrlComposition() throws InterruptedException {
        readPartnerBookmarks();
        BookmarkId mobileId = mBookmarkModel.getMobileFolderId();
        BookmarkId bookmarkBarId = mBookmarkModel.getDesktopFolderId();
        BookmarkId otherId = mBookmarkModel.getOtherFolderId();
        Assert.assertEquals("chrome-native://bookmarks/folder/" + mobileId,
                BookmarkUIState.createFolderUrl(mobileId).toString());
        Assert.assertEquals("chrome-native://bookmarks/folder/" + bookmarkBarId,
                BookmarkUIState.createFolderUrl(bookmarkBarId).toString());
        Assert.assertEquals("chrome-native://bookmarks/folder/" + otherId,
                BookmarkUIState.createFolderUrl(otherId).toString());
    }

    @Test
    @SmallTest
    public void testOpenBookmarkManager() throws InterruptedException {
        openBookmarkManager();
        BookmarkDelegate delegate =
                ((BookmarkItemsAdapter) mItemsContainer.getAdapter()).getDelegateForTesting();

        Assert.assertEquals(BookmarkUIState.STATE_FOLDER, delegate.getCurrentState());
        Assert.assertEquals("chrome-native://bookmarks/folder/3",
                BookmarkUtils.getLastUsedUrl(mActivityTestRule.getActivity()));
    }

    @Test
    @MediumTest
    @Restriction({UiRestriction.RESTRICTION_TYPE_PHONE})
    public void testFolderNavigation_Phone() throws InterruptedException, ExecutionException {
        BookmarkId testFolder = addFolder(TEST_FOLDER_TITLE);
        openBookmarkManager();
        final BookmarkDelegate delegate =
                ((BookmarkItemsAdapter) mItemsContainer.getAdapter()).getDelegateForTesting();
        final BookmarkActionBar toolbar = ((BookmarkManager) delegate).getToolbarForTests();

        // Open the "Mobile bookmarks" folder.
        ThreadUtils.runOnUiThreadBlocking(
                () -> delegate.openFolder(mBookmarkModel.getMobileFolderId()));

        // Check that we are in the mobile bookmarks folder.
        Assert.assertEquals("Mobile bookmarks", toolbar.getTitle());
        Assert.assertEquals(SelectableListToolbar.NAVIGATION_BUTTON_BACK,
                toolbar.getNavigationButtonForTests());
        Assert.assertFalse(toolbar.getMenu().findItem(R.id.edit_menu_id).isVisible());

        // Open the new test folder.
        ThreadUtils.runOnUiThreadBlocking(() -> delegate.openFolder(testFolder));

        // Check that we are in the editable test folder.
        Assert.assertEquals(TEST_FOLDER_TITLE, toolbar.getTitle());
        Assert.assertEquals(SelectableListToolbar.NAVIGATION_BUTTON_BACK,
                toolbar.getNavigationButtonForTests());
        Assert.assertTrue(toolbar.getMenu().findItem(R.id.edit_menu_id).isVisible());

        // Call BookmarkActionBar#onClick() to activate the navigation button.
        ThreadUtils.runOnUiThreadBlocking(() -> toolbar.onClick(toolbar));

        // Check that we are back in the mobile folder
        Assert.assertEquals("Mobile bookmarks", toolbar.getTitle());
        Assert.assertEquals(SelectableListToolbar.NAVIGATION_BUTTON_BACK,
                toolbar.getNavigationButtonForTests());
        Assert.assertFalse(toolbar.getMenu().findItem(R.id.edit_menu_id).isVisible());

        // Call BookmarkActionBar#onClick() to activate the navigation button.
        ThreadUtils.runOnUiThreadBlocking(() -> toolbar.onClick(toolbar));

        // Check that we are in the root folder.
        Assert.assertEquals("Bookmarks", toolbar.getTitle());
        Assert.assertEquals(SelectableListToolbar.NAVIGATION_BUTTON_NONE,
                toolbar.getNavigationButtonForTests());
        Assert.assertFalse(toolbar.getMenu().findItem(R.id.edit_menu_id).isVisible());
    }

    // TODO(twellington): Write a folder navigation test for tablets that waits for the Tab hosting
    //                    the native page to update its url after navigations.

    @Test
    @MediumTest
    public void testSearchBookmarks() throws Exception {
        BookmarkPromoHeader.forcePromoStateForTests(BookmarkPromoHeader.PromoState.PROMO_SYNC);
        addBookmark(TEST_PAGE_TITLE_GOOGLE, mTestPage);
        addBookmark(TEST_PAGE_TITLE_FOO, mTestPageFoo);
        openBookmarkManager();

        BookmarkItemsAdapter adapter = ((BookmarkItemsAdapter) mItemsContainer.getAdapter());
        final BookmarkDelegate delegate = adapter.getDelegateForTesting();

        Assert.assertEquals(BookmarkUIState.STATE_FOLDER, delegate.getCurrentState());
        Assert.assertEquals(
                "Wrong number of items before starting search.", 3, adapter.getItemCount());

        ThreadUtils.runOnUiThreadBlocking(delegate::openSearchUI);

        Assert.assertEquals(BookmarkUIState.STATE_SEARCHING, delegate.getCurrentState());
        Assert.assertEquals(
                "Wrong number of items after showing search UI. The promo should be hidden.", 2,
                adapter.getItemCount());

        searchBookmarks("Google");
        Assert.assertEquals("Wrong number of items after searching.", 1,
                mItemsContainer.getAdapter().getItemCount());

        BookmarkId newBookmark = addBookmark(TEST_PAGE_TITLE_GOOGLE2, mTestPage);
        Assert.assertEquals("Wrong number of items after bookmark added while searching.", 2,
                mItemsContainer.getAdapter().getItemCount());

        removeBookmark(newBookmark);
        Assert.assertEquals("Wrong number of items after bookmark removed while searching.", 1,
                mItemsContainer.getAdapter().getItemCount());

        searchBookmarks("Non-existent page");
        Assert.assertEquals("Wrong number of items after searching for non-existent item.", 0,
                mItemsContainer.getAdapter().getItemCount());

        ThreadUtils.runOnUiThreadBlocking(
                () -> ((BookmarkManager) delegate).getToolbarForTests().hideSearchView());
        Assert.assertEquals("Wrong number of items after closing search UI.", 3,
                mItemsContainer.getAdapter().getItemCount());
        Assert.assertEquals(BookmarkUIState.STATE_FOLDER, delegate.getCurrentState());
    }

    @Test
    @MediumTest
    public void testSearchBookmarks_Delete() throws Exception {
        BookmarkPromoHeader.forcePromoStateForTests(BookmarkPromoHeader.PromoState.PROMO_NONE);
        BookmarkId testFolder = addFolder(TEST_FOLDER_TITLE);
        BookmarkId testBookmark = addBookmark(TEST_PAGE_TITLE_GOOGLE, mTestPage);
        addBookmark(TEST_PAGE_TITLE_FOO, mTestPageFoo);
        openBookmarkManager();

        BookmarkItemsAdapter adapter = ((BookmarkItemsAdapter) mItemsContainer.getAdapter());
        BookmarkManager manager = (BookmarkManager) adapter.getDelegateForTesting();

        Assert.assertEquals("Wrong state, should be in folder", BookmarkUIState.STATE_FOLDER,
                manager.getCurrentState());
        Assert.assertEquals(
                "Wrong number of items before starting search.", 3, adapter.getItemCount());

        // Start searching without entering a query.
        ThreadUtils.runOnUiThreadBlocking(manager::openSearchUI);
        Assert.assertEquals("Wrong state, should be searching", BookmarkUIState.STATE_SEARCHING,
                manager.getCurrentState());

        // Select the folder and delete it.
        ThreadUtils.runOnUiThreadBlocking(
                () -> manager.getSelectionDelegate().toggleSelectionForItem(adapter.getItem(0)));
        ThreadUtils.runOnUiThreadBlocking(
                ()
                        -> manager.getToolbarForTests().onMenuItemClick(
                                manager.getToolbarForTests().getMenu().findItem(
                                        R.id.selection_mode_delete_menu_id)));

        // Search should be exited and the folder should be gone.
        Assert.assertEquals("Wrong state, should be in folder", BookmarkUIState.STATE_FOLDER,
                manager.getCurrentState());
        Assert.assertEquals(
                "Wrong number of items before starting search.", 2, adapter.getItemCount());

        // Start searching, enter a query.
        ThreadUtils.runOnUiThreadBlocking(manager::openSearchUI);
        Assert.assertEquals("Wrong state, should be searching", BookmarkUIState.STATE_SEARCHING,
                manager.getCurrentState());
        searchBookmarks("Google");
        Assert.assertEquals("Wrong number of items after searching.", 1,
                mItemsContainer.getAdapter().getItemCount());

        // Remove the bookmark.
        removeBookmark(testBookmark);

        // The user should still be searching, and the bookmark should be gone.
        Assert.assertEquals("Wrong state, should be searching", BookmarkUIState.STATE_SEARCHING,
                manager.getCurrentState());
        Assert.assertEquals("Wrong number of items after searching.", 0,
                mItemsContainer.getAdapter().getItemCount());

        // Undo the deletion.
        ThreadUtils.runOnUiThreadBlocking(() -> manager.getUndoControllerForTests().onAction(null));

        // The user should still be searching, and the bookmark should reappear.
        Assert.assertEquals("Wrong state, should be searching", BookmarkUIState.STATE_SEARCHING,
                manager.getCurrentState());
        Assert.assertEquals("Wrong number of items after searching.", 1,
                mItemsContainer.getAdapter().getItemCount());
    }

    @Test
    @MediumTest
    @Feature({"RenderTest"})
    @ChromeModernDesign.Enable
    public void testBookmarkFolderIcon() throws Exception {
        BookmarkPromoHeader.forcePromoStateForTests(BookmarkPromoHeader.PromoState.PROMO_NONE);
        addFolder(TEST_FOLDER_TITLE);
        openBookmarkManager();

        BookmarkItemsAdapter adapter = ((BookmarkItemsAdapter) mItemsContainer.getAdapter());
        BookmarkManager manager = (BookmarkManager) adapter.getDelegateForTesting();

        mRenderTestRule.render(manager.getView(), "bookmark_manager_one_folder");

        ThreadUtils.runOnUiThreadBlocking(() -> {
            BookmarkRow itemView = (BookmarkRow) manager.getRecyclerViewForTests()
                                           .findViewHolderForAdapterPosition(0)
                                           .itemView;
            itemView.performLongClick();
            itemView.endAnimationsForTests();
            manager.getToolbarForTests().endAnimationsForTesting();
        });

        mRenderTestRule.render(manager.getView(), "bookmark_manager_folder_selected");

        ThreadUtils.runOnUiThreadBlocking(
                () -> manager.getSelectionDelegate().toggleSelectionForItem(adapter.getItem(0)));

        mRenderTestRule.render(manager.getView(), "bookmark_manager_one_folder");
    }

    @Test
    @MediumTest
    @Restriction({UiRestriction.RESTRICTION_TYPE_PHONE}) // Tablets don't have a close button.
    public void testCloseBookmarksWhileStillLoading() throws Exception {
        BookmarkManager.preventLoadingForTesting(true);

        openBookmarkManager();

        CallbackHelper activityDestroyedCallback = new CallbackHelper();
        ApplicationStatus.registerStateListenerForActivity((activity, newState) -> {
            if (newState == ActivityState.DESTROYED) activityDestroyedCallback.notifyCalled();
        }, mBookmarkActivity);

        final BookmarkActionBar toolbar = mManager.getToolbarForTests();

        ThreadUtils.runOnUiThreadBlocking(
                () -> toolbar.onMenuItemClick(toolbar.getMenu().findItem(R.id.close_menu_id)));

        activityDestroyedCallback.waitForCallback(0);

        BookmarkManager.preventLoadingForTesting(false);
    }

    @Test
    @MediumTest
    public void testEditHiddenWhileStillLoading() throws Exception {
        BookmarkManager.preventLoadingForTesting(true);

        openBookmarkManager();

        BookmarkActionBar toolbar = mManager.getToolbarForTests();
        Assert.assertFalse(toolbar.getMenu().findItem(R.id.edit_menu_id).isVisible());

        BookmarkManager.preventLoadingForTesting(false);
    }

    /**
     * Returns the View that has the given text.
     *
     * @param viewGroup    The group to which the view belongs.
     * @param expectedText The expected description text.
     * @return The unique view, if one exists. Throws an exception if one doesn't exist.
     */
    private static View getViewWithText(final ViewGroup viewGroup, final String expectedText) {
        return ThreadUtils.runOnUiThreadBlockingNoException(new Callable<View>() {
            @Override
            public View call() throws Exception {
                ArrayList<View> outViews = new ArrayList<>();
                ArrayList<View> matchingViews = new ArrayList<>();
                viewGroup.findViewsWithText(outViews, expectedText, View.FIND_VIEWS_WITH_TEXT);
                // outViews includes all views whose text contains expectedText as a
                // case-insensitive substring. Filter these views to find only exact string matches.
                for (View v : outViews) {
                    if (TextUtils.equals(((TextView) v).getText().toString(), expectedText)) {
                        matchingViews.add(v);
                    }
                }
                Assert.assertEquals("Exactly one item should be present.", 1, matchingViews.size());
                return matchingViews.get(0);
            }
        });
    }

    private BookmarkId addBookmark(final String title, final String url)
            throws InterruptedException, ExecutionException {
        readPartnerBookmarks();
        return ThreadUtils.runOnUiThreadBlocking(
                () -> mBookmarkModel.addBookmark(mBookmarkModel.getDefaultFolder(), 0, title, url));
    }

    private BookmarkId addFolder(final String title)
            throws InterruptedException, ExecutionException {
        readPartnerBookmarks();
        return ThreadUtils.runOnUiThreadBlocking(
                () -> mBookmarkModel.addFolder(mBookmarkModel.getDefaultFolder(), 0, title));
    }

    private void removeBookmark(final BookmarkId bookmarkId) {
        ThreadUtils.runOnUiThreadBlocking(() -> mBookmarkModel.deleteBookmark(bookmarkId));
    }

    private void searchBookmarks(final String query) {
        ThreadUtils.runOnUiThreadBlocking(
                () -> ((BookmarkItemsAdapter) mItemsContainer.getAdapter()).search(query));
    }
}
