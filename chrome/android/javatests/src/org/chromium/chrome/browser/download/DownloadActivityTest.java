// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.download;

import android.content.Intent;
import android.content.SharedPreferences.Editor;
import android.os.Handler;
import android.os.Looper;
import android.support.annotation.StringRes;
import android.support.test.InstrumentationRegistry;
import android.support.test.filters.MediumTest;
import android.support.test.rule.ActivityTestRule;
import android.support.v7.widget.RecyclerView;
import android.support.v7.widget.RecyclerView.ViewHolder;
import android.text.TextUtils;
import android.view.View;
import android.widget.Spinner;
import android.widget.TextView;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.ContextUtils;
import org.chromium.base.ThreadUtils;
import org.chromium.base.test.util.CallbackHelper;
import org.chromium.base.test.util.Restriction;
import org.chromium.base.test.util.RetryOnFailure;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ChromeFeatureList;
import org.chromium.chrome.browser.download.ui.DownloadHistoryAdapter;
import org.chromium.chrome.browser.download.ui.DownloadHistoryItemViewHolder;
import org.chromium.chrome.browser.download.ui.DownloadHistoryItemWrapper;
import org.chromium.chrome.browser.download.ui.DownloadHistoryItemWrapper.OfflineItemWrapper;
import org.chromium.chrome.browser.download.ui.DownloadItemView;
import org.chromium.chrome.browser.download.ui.DownloadManagerToolbar;
import org.chromium.chrome.browser.download.ui.DownloadManagerUi;
import org.chromium.chrome.browser.download.ui.SpaceDisplay;
import org.chromium.chrome.browser.download.ui.StubbedProvider;
import org.chromium.chrome.browser.preferences.PrefServiceBridge;
import org.chromium.chrome.browser.snackbar.SnackbarManager;
import org.chromium.chrome.browser.util.IntentUtils;
import org.chromium.chrome.browser.widget.ListMenuButton.Item;
import org.chromium.chrome.browser.widget.selection.SelectionDelegate.SelectionObserver;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.chrome.test.util.browser.Features.DisableFeatures;
import org.chromium.chrome.test.util.browser.Features.EnableFeatures;
import org.chromium.components.offline_items_collection.OfflineItem;
import org.chromium.content.browser.test.util.Criteria;
import org.chromium.content.browser.test.util.CriteriaHelper;
import org.chromium.ui.test.util.UiRestriction;

import java.util.HashMap;
import java.util.List;

/**
 * Tests the DownloadActivity and the DownloadManagerUi.
 */
@RunWith(ChromeJUnit4ClassRunner.class)
@Restriction(UiRestriction.RESTRICTION_TYPE_PHONE)
public class DownloadActivityTest {
    @Rule
    public ActivityTestRule<DownloadActivity> mActivityTestRule =
            new ActivityTestRule<>(DownloadActivity.class);

    private static class TestObserver extends RecyclerView.AdapterDataObserver
            implements SelectionObserver<DownloadHistoryItemWrapper>,
                    DownloadManagerUi.DownloadUiObserver, SpaceDisplay.Observer {
        public final CallbackHelper onChangedCallback = new CallbackHelper();
        public final CallbackHelper onSelectionCallback = new CallbackHelper();
        public final CallbackHelper onFilterCallback = new CallbackHelper();
        public final CallbackHelper onSpaceDisplayUpdatedCallback = new CallbackHelper();

        private List<DownloadHistoryItemWrapper> mOnSelectionItems;
        private Handler mHandler;

        public TestObserver() {
            mHandler = new Handler(Looper.getMainLooper());
        }

        @Override
        public void onChanged() {
            // To guarantee that all real Observers have had a chance to react to the event, post
            // the CallbackHelper.notifyCalled() call.
            mHandler.post(() -> onChangedCallback.notifyCalled());
        }

        @Override
        public void onSelectionStateChange(List<DownloadHistoryItemWrapper> selectedItems) {
            mOnSelectionItems = selectedItems;
            mHandler.post(() -> onSelectionCallback.notifyCalled());
        }

        @Override
        public void onFilterChanged(int filter) {
            mHandler.post(() -> onFilterCallback.notifyCalled());
        }

        @Override
        public void onSpaceDisplayUpdated(SpaceDisplay display) {
            mHandler.post(() -> onSpaceDisplayUpdatedCallback.notifyCalled());
        }

        @Override
        public void onManagerDestroyed() {
        }
    }

    private static final String PREF_SHOW_STORAGE_INFO_HEADER =
            "download_home_show_storage_info_header";

    private StubbedProvider mStubbedProvider;
    private TestObserver mAdapterObserver;
    private DownloadManagerUi mUi;
    private DownloadHistoryAdapter mAdapter;

    private RecyclerView mRecyclerView;
    private TextView mSpaceUsedDisplay;

    @Before
    public void setUp() throws Exception {
        Editor editor = ContextUtils.getAppSharedPreferences().edit();
        editor.putBoolean(PREF_SHOW_STORAGE_INFO_HEADER, true).apply();

        ThreadUtils.runOnUiThreadBlocking(() -> {
            PrefServiceBridge.getInstance().setPromptForDownloadAndroid(
                    DownloadPromptStatus.DONT_SHOW);
        });

        HashMap<String, Boolean> features = new HashMap<String, Boolean>();
        features.put(ChromeFeatureList.DOWNLOADS_LOCATION_CHANGE, false);
        features.put(ChromeFeatureList.DOWNLOAD_HOME_SHOW_STORAGE_INFO, false);
        ChromeFeatureList.setTestFeatures(features);

        mStubbedProvider = new StubbedProvider();
        DownloadManagerUi.setProviderForTests(mStubbedProvider);

        mAdapterObserver = new TestObserver();
        mStubbedProvider.getSelectionDelegate().addObserver(mAdapterObserver);

        startDownloadActivity();
        mUi = mActivityTestRule.getActivity().getDownloadManagerUiForTests();
        mStubbedProvider.setUIDelegate(mUi);
        mAdapter = mUi.getDownloadHistoryAdapterForTests();
        mAdapter.registerAdapterDataObserver(mAdapterObserver);

        mSpaceUsedDisplay =
                (TextView) mActivityTestRule.getActivity().findViewById(R.id.size_downloaded);
        mRecyclerView =
                ((RecyclerView) mActivityTestRule.getActivity().findViewById(R.id.recycler_view));

        mAdapter.getSpaceDisplayForTests().addObserverForTests(mAdapterObserver);
    }

    @Test
    @MediumTest
    public void testSpaceDisplay() throws Exception {
        // This first check is a Criteria because initialization of the Adapter is asynchronous.
        CriteriaHelper.pollUiThread(new Criteria() {
            @Override
            public boolean isSatisfied() {
                return TextUtils.equals("6.00 GB downloaded", mSpaceUsedDisplay.getText());
            }
        });

        // Add a new item.
        int callCount = mAdapterObserver.onChangedCallback.getCallCount();
        int spaceDisplayCallCount = mAdapterObserver.onSpaceDisplayUpdatedCallback.getCallCount();
        final DownloadItem updateItem = StubbedProvider.createDownloadItem(7, "20151021 07:28");
        ThreadUtils.runOnUiThread(() -> mAdapter.onDownloadItemCreated(updateItem));
        mAdapterObserver.onChangedCallback.waitForCallback(callCount, 2);
        mAdapterObserver.onSpaceDisplayUpdatedCallback.waitForCallback(spaceDisplayCallCount);
        // Use Criteria here because the text for SpaceDisplay is updated through an AsyncTask.
        // Same for the checks below.
        CriteriaHelper.pollUiThread(new Criteria() {
            @Override
            public boolean isSatisfied() {
                return TextUtils.equals("6.50 GB downloaded", mSpaceUsedDisplay.getText());
            }
        });

        // Mark one download as deleted on disk, which should prevent it from being counted.
        callCount = mAdapterObserver.onChangedCallback.getCallCount();
        spaceDisplayCallCount = mAdapterObserver.onSpaceDisplayUpdatedCallback.getCallCount();
        final DownloadItem deletedItem = StubbedProvider.createDownloadItem(6, "20151021 07:28");
        deletedItem.setHasBeenExternallyRemoved(true);
        ThreadUtils.runOnUiThread(() -> mAdapter.onDownloadItemUpdated(deletedItem));
        mAdapterObserver.onChangedCallback.waitForCallback(callCount, 2);
        mAdapterObserver.onSpaceDisplayUpdatedCallback.waitForCallback(spaceDisplayCallCount);
        CriteriaHelper.pollUiThread(new Criteria() {
            @Override
            public boolean isSatisfied() {
                return TextUtils.equals("5.50 GB downloaded", mSpaceUsedDisplay.getText());
            }
        });

        // Say that the offline page has been deleted.
        callCount = mAdapterObserver.onChangedCallback.getCallCount();
        spaceDisplayCallCount = mAdapterObserver.onSpaceDisplayUpdatedCallback.getCallCount();
        final OfflineItem deletedPage = StubbedProvider.createOfflineItem(3, "20151021 07:28");
        ThreadUtils.runOnUiThread(
                ()
                        -> mStubbedProvider.getOfflineContentProvider().observer.onItemRemoved(
                                deletedPage.id));
        mAdapterObserver.onChangedCallback.waitForCallback(callCount, 2);
        mAdapterObserver.onSpaceDisplayUpdatedCallback.waitForCallback(spaceDisplayCallCount);
        CriteriaHelper.pollUiThread(new Criteria() {
            @Override
            public boolean isSatisfied() {
                return TextUtils.equals("512.00 MB downloaded", mSpaceUsedDisplay.getText());
            }
        });
    }

    /** Clicking on filters affects various things in the UI. */
    @Test
    @MediumTest
    public void testFilters() throws Exception {
        // This first check is a Criteria because initialization of the Adapter is asynchronous.
        CriteriaHelper.pollUiThread(new Criteria() {
            @Override
            public boolean isSatisfied() {
                return TextUtils.equals("6.00 GB downloaded", mSpaceUsedDisplay.getText());
            }
        });

        // Change the filter to Pages. Only the space display, offline page and the date header
        // should stay.
        int spaceDisplayCallCount = mAdapterObserver.onSpaceDisplayUpdatedCallback.getCallCount();
        clickOnFilter(mUi, 1);
        Assert.assertEquals(3, mAdapter.getItemCount());

        // Check that the number of items displayed is correct.
        // We need to poll because RecyclerView doesn't animate changes immediately.
        CriteriaHelper.pollUiThread(new Criteria() {
            @Override
            public boolean isSatisfied() {
                return mRecyclerView.getChildCount() == 3;
            }
        });

        // Filtering doesn't affect the total download size.
        mAdapterObserver.onSpaceDisplayUpdatedCallback.waitForCallback(spaceDisplayCallCount);
        Assert.assertEquals("6.00 GB downloaded", mSpaceUsedDisplay.getText());
    }

    @Test
    @MediumTest
    @RetryOnFailure
    public void testDeleteFiles() throws Exception {
        SnackbarManager.setDurationForTesting(1);

        // This first check is a Criteria because initialization of the Adapter is asynchronous.
        CriteriaHelper.pollUiThread(new Criteria() {
            @Override
            public boolean isSatisfied() {
                return TextUtils.equals("6.00 GB downloaded", mSpaceUsedDisplay.getText());
            }
        });

        // Select the first two items.
        toggleItemSelection(2);
        toggleItemSelection(3);

        // Click the delete button, which should delete the items and reset the toolbar.
        Assert.assertEquals(12, mAdapter.getItemCount());
        // checkForExternallyRemovedFiles() should have been called once already in onResume().
        Assert.assertEquals(
                1, mStubbedProvider.getDownloadDelegate().checkExternalCallback.getCallCount());
        Assert.assertEquals(
                0, mStubbedProvider.getDownloadDelegate().removeDownloadCallback.getCallCount());
        Assert.assertEquals(
                0, mStubbedProvider.getOfflineContentProvider().deleteItemCallback.getCallCount());
        int callCount = mAdapterObserver.onSpaceDisplayUpdatedCallback.getCallCount();
        ThreadUtils.runOnUiThread(() -> Assert.assertTrue(mUi.getDownloadManagerToolbarForTests()
                    .getMenu().performIdentifierAction(R.id.selection_mode_delete_menu_id, 0)));

        mAdapterObserver.onSpaceDisplayUpdatedCallback.waitForCallback(callCount);
        Assert.assertEquals(
                1, mStubbedProvider.getDownloadDelegate().checkExternalCallback.getCallCount());
        Assert.assertFalse(mStubbedProvider.getSelectionDelegate().isSelectionEnabled());
        Assert.assertEquals(9, mAdapter.getItemCount());
        Assert.assertEquals("0.65 KB downloaded", mSpaceUsedDisplay.getText());
    }

    @Test
    @MediumTest
    @RetryOnFailure
    public void testDeleteFileFromMenu() throws Exception {
        SnackbarManager.setDurationForTesting(1);

        // This first check is a Criteria because initialization of the Adapter is asynchronous.
        CriteriaHelper.pollUiThread(new Criteria() {
            @Override
            public boolean isSatisfied() {
                return TextUtils.equals("6.00 GB downloaded", mSpaceUsedDisplay.getText());
            }
        });

        Assert.assertEquals(12, mAdapter.getItemCount());
        // checkForExternallyRemovedFiles() should have been called once already in onResume().
        Assert.assertEquals(
                1, mStubbedProvider.getDownloadDelegate().checkExternalCallback.getCallCount());
        Assert.assertEquals(
                0, mStubbedProvider.getDownloadDelegate().removeDownloadCallback.getCallCount());
        Assert.assertEquals(
                0, mStubbedProvider.getOfflineContentProvider().deleteItemCallback.getCallCount());
        int callCount = mAdapterObserver.onSpaceDisplayUpdatedCallback.getCallCount();

        // Simulate a delete context menu action on the item.
        simulateContextMenu(2, R.string.delete);
        mAdapterObserver.onSpaceDisplayUpdatedCallback.waitForCallback(callCount);
        Assert.assertEquals(
                1, mStubbedProvider.getDownloadDelegate().checkExternalCallback.getCallCount());
        Assert.assertFalse(mStubbedProvider.getSelectionDelegate().isSelectionEnabled());
        Assert.assertEquals(11, mAdapter.getItemCount());
        Assert.assertEquals("5.00 GB downloaded", mSpaceUsedDisplay.getText());
    }

    @Test
    @MediumTest
    @RetryOnFailure
    public void testUndoDelete() throws Exception {
        // Adapter positions:
        // 0 = space display
        // 1 = date
        // 2 = download item #7
        // 3 = download item #8
        // 4 = date
        // 5 = download item #6
        // 6 = offline page #3

        SnackbarManager.setDurationForTesting(5000);

        // Add duplicate items.
        int callCount = mAdapterObserver.onSpaceDisplayUpdatedCallback.getCallCount();
        final DownloadItem item7 = StubbedProvider.createDownloadItem(7, "20161021 07:28");
        final DownloadItem item8 = StubbedProvider.createDownloadItem(8, "20161021 17:28");
        ThreadUtils.runOnUiThread(() -> {
            mAdapter.onDownloadItemCreated(item7);
            mAdapter.onDownloadItemCreated(item8);
        });

        // The criteria is needed because an AsyncTask is fired to update the space display, which
        // can result in either 1 or 2 updates.
        mAdapterObserver.onSpaceDisplayUpdatedCallback.waitForCallback(callCount);
        CriteriaHelper.pollUiThread(new Criteria() {
            @Override
            public boolean isSatisfied() {
                return TextUtils.equals("6.50 GB downloaded", mSpaceUsedDisplay.getText());
            }
        });

        // Select download item #7 and offline page #3.
        toggleItemSelection(2);
        toggleItemSelection(6);

        Assert.assertEquals(15, mAdapter.getItemCount());

        // Click the delete button.
        callCount = mAdapterObserver.onSpaceDisplayUpdatedCallback.getCallCount();
        ThreadUtils.runOnUiThread(() -> Assert.assertTrue(mUi.getDownloadManagerToolbarForTests()
                    .getMenu().performIdentifierAction(R.id.selection_mode_delete_menu_id, 0)));
        mAdapterObserver.onSpaceDisplayUpdatedCallback.waitForCallback(callCount);

        // Assert that items are temporarily removed from the adapter. The two selected items,
        // one duplicate item, and one date bucket should be removed.
        Assert.assertEquals(11, mAdapter.getItemCount());
        Assert.assertEquals("1.00 GB downloaded", mSpaceUsedDisplay.getText());

        // Click "Undo" on the snackbar.
        callCount = mAdapterObserver.onSpaceDisplayUpdatedCallback.getCallCount();
        final View rootView = mUi.getView().getRootView();
        Assert.assertNotNull(rootView.findViewById(R.id.snackbar));
        ThreadUtils.runOnUiThread(
                (Runnable) () -> rootView.findViewById(R.id.snackbar_button).callOnClick());

        mAdapterObserver.onSpaceDisplayUpdatedCallback.waitForCallback(callCount);

        // Assert that items are restored.
        Assert.assertEquals(
                0, mStubbedProvider.getDownloadDelegate().removeDownloadCallback.getCallCount());
        Assert.assertEquals(
                0, mStubbedProvider.getOfflineContentProvider().deleteItemCallback.getCallCount());
        Assert.assertFalse(mStubbedProvider.getSelectionDelegate().isSelectionEnabled());
        Assert.assertEquals(15, mAdapter.getItemCount());
        Assert.assertEquals("6.50 GB downloaded", mSpaceUsedDisplay.getText());
    }

    @Test
    @MediumTest
    @RetryOnFailure
    public void testUndoDeleteFromMenu() throws Exception {
        // Adapter positions:
        // 0 = space display
        // 1 = date
        // 2 = download item #7
        // 3 = download item #8
        // 4 = date
        // 5 = download item #6
        // 6 = offline page #3

        SnackbarManager.setDurationForTesting(5000);

        // Add duplicate items.
        int callCount = mAdapterObserver.onSpaceDisplayUpdatedCallback.getCallCount();
        final DownloadItem item7 = StubbedProvider.createDownloadItem(7, "20161021 07:28");
        final DownloadItem item8 = StubbedProvider.createDownloadItem(8, "20161021 17:28");
        ThreadUtils.runOnUiThread(() -> {
            mAdapter.onDownloadItemCreated(item7);
            mAdapter.onDownloadItemCreated(item8);
        });

        // The criteria is needed because an AsyncTask is fired to update the space display, which
        // can result in either 1 or 2 updates.
        mAdapterObserver.onSpaceDisplayUpdatedCallback.waitForCallback(callCount);
        CriteriaHelper.pollUiThread(new Criteria() {
            @Override
            public boolean isSatisfied() {
                return TextUtils.equals("6.50 GB downloaded", mSpaceUsedDisplay.getText());
            }
        });

        Assert.assertEquals(15, mAdapter.getItemCount());
        callCount = mAdapterObserver.onSpaceDisplayUpdatedCallback.getCallCount();

        // Simulate a delete context menu action on the item.
        simulateContextMenu(2, R.string.delete);
        mAdapterObserver.onSpaceDisplayUpdatedCallback.waitForCallback(callCount);

        // Assert that items are temporarily removed from the adapter. The two selected items,
        // one duplicate item, and one date bucket should be removed.
        Assert.assertEquals(12, mAdapter.getItemCount());
        Assert.assertEquals("6.00 GB downloaded", mSpaceUsedDisplay.getText());

        // Click "Undo" on the snackbar.
        callCount = mAdapterObserver.onSpaceDisplayUpdatedCallback.getCallCount();
        final View rootView = mUi.getView().getRootView();
        Assert.assertNotNull(rootView.findViewById(R.id.snackbar));
        ThreadUtils.runOnUiThread(
                (Runnable) () -> rootView.findViewById(R.id.snackbar_button).callOnClick());

        mAdapterObserver.onSpaceDisplayUpdatedCallback.waitForCallback(callCount);

        // Assert that items are restored.
        Assert.assertEquals(
                0, mStubbedProvider.getDownloadDelegate().removeDownloadCallback.getCallCount());
        Assert.assertEquals(
                0, mStubbedProvider.getOfflineContentProvider().deleteItemCallback.getCallCount());
        Assert.assertFalse(mStubbedProvider.getSelectionDelegate().isSelectionEnabled());
        Assert.assertEquals(15, mAdapter.getItemCount());
        Assert.assertEquals("6.50 GB downloaded", mSpaceUsedDisplay.getText());
    }

    @Test
    @MediumTest
    @RetryOnFailure
    public void testUndoDeleteDuplicatesSelected() throws Exception {
        // Adapter positions:
        // 0 = space display
        // 1 = date
        // 2 = download item #7
        // 3 = download item #8
        // ....

        SnackbarManager.setDurationForTesting(5000);

        // Add duplicate items.
        int callCount = mAdapterObserver.onSpaceDisplayUpdatedCallback.getCallCount();
        final DownloadItem item7 = StubbedProvider.createDownloadItem(7, "20161021 07:28");
        final DownloadItem item8 = StubbedProvider.createDownloadItem(8, "20161021 17:28");
        ThreadUtils.runOnUiThread(() -> {
            mAdapter.onDownloadItemCreated(item7);
            mAdapter.onDownloadItemCreated(item8);
        });

        // The criteria is needed because an AsyncTask is fired to update the space display, which
        // can result in either 1 or 2 updates.
        mAdapterObserver.onSpaceDisplayUpdatedCallback.waitForCallback(callCount);
        CriteriaHelper.pollUiThread(new Criteria() {
            @Override
            public boolean isSatisfied() {
                return TextUtils.equals("6.50 GB downloaded", mSpaceUsedDisplay.getText());
            }
        });

        // Select download item #7 and download item #8.
        toggleItemSelection(2);
        toggleItemSelection(3);

        Assert.assertEquals(15, mAdapter.getItemCount());

        // Click the delete button.
        callCount = mAdapterObserver.onSpaceDisplayUpdatedCallback.getCallCount();
        ThreadUtils.runOnUiThread(() -> Assert.assertTrue(mUi.getDownloadManagerToolbarForTests()
                    .getMenu().performIdentifierAction(R.id.selection_mode_delete_menu_id, 0)));
        mAdapterObserver.onSpaceDisplayUpdatedCallback.waitForCallback(callCount);

        // Assert that the two items and their date bucket are temporarily removed from the adapter.
        Assert.assertEquals(12, mAdapter.getItemCount());
        Assert.assertEquals("6.00 GB downloaded", mSpaceUsedDisplay.getText());

        // Click "Undo" on the snackbar.
        callCount = mAdapterObserver.onSpaceDisplayUpdatedCallback.getCallCount();
        final View rootView = mUi.getView().getRootView();
        Assert.assertNotNull(rootView.findViewById(R.id.snackbar));
        ThreadUtils.runOnUiThread(
                (Runnable) () -> rootView.findViewById(R.id.snackbar_button).callOnClick());

        mAdapterObserver.onSpaceDisplayUpdatedCallback.waitForCallback(callCount);

        // Assert that items are restored.
        Assert.assertEquals(15, mAdapter.getItemCount());
        Assert.assertEquals("6.50 GB downloaded", mSpaceUsedDisplay.getText());
    }

    @Test
    @MediumTest
    @DisableFeatures("OfflinePagesSharing")
    public void testShareFiles() throws Exception {
        // Adapter positions:
        // 0 = space display
        // 1 = date
        // 2 = download item #6
        // 3 = offline page #3
        // 4 = date
        // 5 = download item #3
        // 6 = download item #4
        // 7 = download item #5
        // 8 = date
        // 9 = download item #0
        // 10 = download item #1
        // 11 = download item #2

        // Select an image, download item #6.
        toggleItemSelection(2);
        Intent shareIntent = DownloadUtils.createShareIntent(
                mUi.getBackendProvider().getSelectionDelegate().getSelectedItems(), null);
        Assert.assertEquals("Incorrect intent action", Intent.ACTION_SEND, shareIntent.getAction());
        Assert.assertEquals("Incorrect intent mime type", "image/png", shareIntent.getType());
        Assert.assertNotNull(
                "Intent expected to have stream", shareIntent.getExtras().get(Intent.EXTRA_STREAM));
        Assert.assertNull("Intent not expected to have parcelable ArrayList",
                shareIntent.getParcelableArrayListExtra(Intent.EXTRA_STREAM));

        // Scroll to ensure the item at position 8 is visible.
        ThreadUtils.runOnUiThread(() -> mRecyclerView.scrollToPosition(9));
        InstrumentationRegistry.getInstrumentation().waitForIdleSync();

        // Select another image, download item #0.
        toggleItemSelection(9);
        shareIntent = DownloadUtils.createShareIntent(
                mUi.getBackendProvider().getSelectionDelegate().getSelectedItems(), null);
        Assert.assertEquals(
                "Incorrect intent action", Intent.ACTION_SEND_MULTIPLE, shareIntent.getAction());
        Assert.assertEquals("Incorrect intent mime type", "image/*", shareIntent.getType());
        Assert.assertEquals("Intent expected to have parcelable ArrayList", 2,
                shareIntent.getParcelableArrayListExtra(Intent.EXTRA_STREAM).size());

        // Scroll to ensure the item at position 5 is visible.
        ThreadUtils.runOnUiThread(() -> mRecyclerView.scrollToPosition(6));
        InstrumentationRegistry.getInstrumentation().waitForIdleSync();

        // Select non-image item, download item #4.
        toggleItemSelection(6);
        shareIntent = DownloadUtils.createShareIntent(
                mUi.getBackendProvider().getSelectionDelegate().getSelectedItems(), null);
        Assert.assertEquals(
                "Incorrect intent action", Intent.ACTION_SEND_MULTIPLE, shareIntent.getAction());
        Assert.assertEquals("Incorrect intent mime type", "*/*", shareIntent.getType());
        Assert.assertEquals("Intent expected to have parcelable ArrayList", 3,
                shareIntent.getParcelableArrayListExtra(Intent.EXTRA_STREAM).size());

        // Scroll to ensure the item at position 2 is visible.
        ThreadUtils.runOnUiThread(() -> mRecyclerView.scrollToPosition(3));
        InstrumentationRegistry.getInstrumentation().waitForIdleSync();

        // Select an offline page #3.
        toggleItemSelection(3);
        shareIntent = DownloadUtils.createShareIntent(
                mUi.getBackendProvider().getSelectionDelegate().getSelectedItems(), null);
        Assert.assertEquals(
                "Incorrect intent action", Intent.ACTION_SEND_MULTIPLE, shareIntent.getAction());
        Assert.assertEquals("Incorrect intent mime type", "*/*", shareIntent.getType());
        Assert.assertEquals("Intent expected to have parcelable ArrayList", 3,
                shareIntent.getParcelableArrayListExtra(Intent.EXTRA_STREAM).size());
        Assert.assertEquals("Intent expected to have plain text for offline page URL",
                "https://thangs.com",
                IntentUtils.safeGetStringExtra(shareIntent, Intent.EXTRA_TEXT));
    }

    // TODO(carlosk): OfflineItems used here come from StubbedProvider so this might not be the best
    // place to test peer-2-peer sharing.
    @Test
    @MediumTest
    @EnableFeatures("OfflinePagesSharing")
    public void testShareOfflinePageWithP2PSharingEnabled() throws Exception {
        // Scroll to ensure the item at position 2 is visible.
        ThreadUtils.runOnUiThread(() -> mRecyclerView.scrollToPosition(3));
        InstrumentationRegistry.getInstrumentation().waitForIdleSync();

        // Select the offline page located at position #3.
        toggleItemSelection(3);
        List<DownloadHistoryItemWrapper> selected_items =
                mUi.getBackendProvider().getSelectionDelegate().getSelectedItems();
        Assert.assertEquals("There should be only one item selected", 1, selected_items.size());
        Intent shareIntent = DownloadUtils.createShareIntent(selected_items, null);

        Assert.assertEquals("Incorrect intent action", Intent.ACTION_SEND, shareIntent.getAction());
        Assert.assertEquals("Incorrect intent mime type", "*/*", shareIntent.getType());
        Assert.assertNotNull("Intent expected to have parcelable ArrayList",
                shareIntent.getParcelableExtra(Intent.EXTRA_STREAM));
        Assert.assertEquals("Intent expected to have parcelable Uri",
                "file:///data/fake_path/Downloads/4",
                shareIntent.getParcelableExtra(Intent.EXTRA_STREAM).toString());
        Assert.assertNull("Intent expected to not have any text for offline page",
                IntentUtils.safeGetStringExtra(shareIntent, Intent.EXTRA_TEXT));

        // Pass a map that contains a new file path.
        HashMap<String, String> newFilePathMap = new HashMap<String, String>();
        newFilePathMap.put(((OfflineItemWrapper) selected_items.get(0)).getId(),
                "/data/new_fake_path/Downloads/4");
        shareIntent = DownloadUtils.createShareIntent(selected_items, newFilePathMap);

        Assert.assertEquals("Incorrect intent action", Intent.ACTION_SEND, shareIntent.getAction());
        Assert.assertEquals("Incorrect intent mime type", "*/*", shareIntent.getType());
        Assert.assertNotNull("Intent expected to have parcelable ArrayList",
                shareIntent.getParcelableExtra(Intent.EXTRA_STREAM));
        Assert.assertEquals("Intent expected to have parcelable Uri",
                "file:///data/new_fake_path/Downloads/4",
                shareIntent.getParcelableExtra(Intent.EXTRA_STREAM).toString());
        Assert.assertNull("Intent expected to not have any text for offline page",
                IntentUtils.safeGetStringExtra(shareIntent, Intent.EXTRA_TEXT));
    }

    @Test
    @MediumTest
    @DisableFeatures(ChromeFeatureList.DOWNLOADS_LOCATION_CHANGE)
    public void testToggleSelection() throws Exception {
        // The selection toolbar should not be showing.
        Assert.assertTrue(mAdapterObserver.mOnSelectionItems.isEmpty());
        Assert.assertEquals(View.VISIBLE,
                mActivityTestRule.getActivity().findViewById(R.id.close_menu_id).getVisibility());
        Assert.assertEquals(View.GONE,
                mActivityTestRule.getActivity()
                        .findViewById(R.id.selection_mode_number)
                        .getVisibility());
        Assert.assertNull(
                mActivityTestRule.getActivity().findViewById(R.id.selection_mode_share_menu_id));
        Assert.assertNull(
                mActivityTestRule.getActivity().findViewById(R.id.selection_mode_delete_menu_id));
        Assert.assertFalse(mStubbedProvider.getSelectionDelegate().isSelectionEnabled());

        // Select an item.
        toggleItemSelection(2);

        // The toolbar should flip states to allow doing things with the selected items.
        Assert.assertNull(mActivityTestRule.getActivity().findViewById(R.id.close_menu_id));
        Assert.assertEquals(View.VISIBLE,
                mActivityTestRule.getActivity()
                        .findViewById(R.id.selection_mode_number)
                        .getVisibility());
        Assert.assertEquals(View.VISIBLE,
                mActivityTestRule.getActivity()
                        .findViewById(R.id.selection_mode_share_menu_id)
                        .getVisibility());
        Assert.assertEquals(View.VISIBLE,
                mActivityTestRule.getActivity()
                        .findViewById(R.id.selection_mode_delete_menu_id)
                        .getVisibility());
        Assert.assertTrue(mStubbedProvider.getSelectionDelegate().isSelectionEnabled());

        // Deselect the same item.
        toggleItemSelection(2);

        // The toolbar should flip back.
        Assert.assertTrue(mAdapterObserver.mOnSelectionItems.isEmpty());
        Assert.assertEquals(View.VISIBLE,
                mActivityTestRule.getActivity().findViewById(R.id.close_menu_id).getVisibility());
        Assert.assertEquals(View.GONE,
                mActivityTestRule.getActivity()
                        .findViewById(R.id.selection_mode_number)
                        .getVisibility());
        Assert.assertNull(
                mActivityTestRule.getActivity().findViewById(R.id.selection_mode_share_menu_id));
        Assert.assertNull(
                mActivityTestRule.getActivity().findViewById(R.id.selection_mode_delete_menu_id));
        Assert.assertFalse(mStubbedProvider.getSelectionDelegate().isSelectionEnabled());
    }

    @Test
    @MediumTest
    @DisableFeatures(ChromeFeatureList.DOWNLOADS_LOCATION_CHANGE)
    public void testSearchView() throws Exception {
        final DownloadManagerToolbar toolbar = mUi.getDownloadManagerToolbarForTests();
        View toolbarSearchView = toolbar.getSearchViewForTests();
        Assert.assertEquals(View.GONE, toolbarSearchView.getVisibility());

        toggleItemSelection(2);
        Assert.assertTrue(mStubbedProvider.getSelectionDelegate().isSelectionEnabled());

        int callCount = mAdapterObserver.onSelectionCallback.getCallCount();
        ThreadUtils.runOnUiThreadBlocking(
                (Runnable) () -> toolbar.getMenu().performIdentifierAction(R.id.search_menu_id, 0));

        // The selection should be cleared when a search is started.
        mAdapterObserver.onSelectionCallback.waitForCallback(callCount, 1);
        Assert.assertFalse(mStubbedProvider.getSelectionDelegate().isSelectionEnabled());
        Assert.assertEquals(View.VISIBLE, toolbarSearchView.getVisibility());

        // Select an item and assert that the search view is no longer showing.
        toggleItemSelection(2);
        Assert.assertTrue(mStubbedProvider.getSelectionDelegate().isSelectionEnabled());
        Assert.assertEquals(View.GONE, toolbarSearchView.getVisibility());

        // Clear the selection and assert that the search view is showing again.
        toggleItemSelection(2);
        Assert.assertFalse(mStubbedProvider.getSelectionDelegate().isSelectionEnabled());
        Assert.assertEquals(View.VISIBLE, toolbarSearchView.getVisibility());

        // Close the search view.
        ThreadUtils.runOnUiThreadBlocking(() -> toolbar.onNavigationBack());
        Assert.assertEquals(View.GONE, toolbarSearchView.getVisibility());
    }

    private DownloadActivity startDownloadActivity() throws Exception {
        // Load up the downloads lists.
        DownloadItem item0 = StubbedProvider.createDownloadItem(0, "19551112 06:38");
        DownloadItem item1 = StubbedProvider.createDownloadItem(1, "19551112 06:38");
        DownloadItem item2 = StubbedProvider.createDownloadItem(2, "19551112 06:38");
        DownloadItem item3 = StubbedProvider.createDownloadItem(3, "19851026 09:00");
        DownloadItem item4 = StubbedProvider.createDownloadItem(4, "19851026 09:00");
        DownloadItem item5 = StubbedProvider.createDownloadItem(5, "19851026 09:00");
        DownloadItem item6 = StubbedProvider.createDownloadItem(6, "20151021 07:28");
        OfflineItem item7 = StubbedProvider.createOfflineItem(3, "20151021 07:28");
        mStubbedProvider.getDownloadDelegate().regularItems.add(item0);
        mStubbedProvider.getDownloadDelegate().regularItems.add(item1);
        mStubbedProvider.getDownloadDelegate().regularItems.add(item2);
        mStubbedProvider.getDownloadDelegate().regularItems.add(item3);
        mStubbedProvider.getDownloadDelegate().regularItems.add(item4);
        mStubbedProvider.getDownloadDelegate().regularItems.add(item5);
        mStubbedProvider.getDownloadDelegate().regularItems.add(item6);
        mStubbedProvider.getOfflineContentProvider().items.add(item7);

        // Start the activity up.
        Intent intent = new Intent();
        intent.setClass(InstrumentationRegistry.getTargetContext(), DownloadActivity.class);
        intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        return mActivityTestRule.launchActivity(intent);
    }

    private void clickOnFilter(final DownloadManagerUi ui, final int position) throws Exception {
        int previousCount = mAdapterObserver.onChangedCallback.getCallCount();
        final Spinner spinner = mUi.getDownloadManagerToolbarForTests().getSpinnerForTests();
        ThreadUtils.runOnUiThread(() -> {
            spinner.performClick();
            spinner.setSelection(position);
        });
        mAdapterObserver.onChangedCallback.waitForCallback(previousCount);
    }

    private void toggleItemSelection(int position) throws Exception {
        int callCount = mAdapterObserver.onSelectionCallback.getCallCount();
        final DownloadItemView itemView = getView(position);
        ThreadUtils.runOnUiThread((Runnable) () -> itemView.performLongClick());
        mAdapterObserver.onSelectionCallback.waitForCallback(callCount, 1);
    }

    private void simulateContextMenu(int position, @StringRes int text) throws Exception {
        final DownloadItemView view = getView(position);
        ThreadUtils.runOnUiThread((Runnable) () -> {
            Item[] items = view.getItems();
            for (Item item : items) {
                if (item.getTextId() == text) {
                    view.onItemSelected(item);
                    return;
                }
            }
            throw new IllegalStateException("Context menu option not found " + text);
        });
    }

    private DownloadItemView getView(int position) throws Exception {
        int callCount = mAdapterObserver.onSelectionCallback.getCallCount();
        ViewHolder mostRecentHolder = mRecyclerView.findViewHolderForAdapterPosition(position);
        Assert.assertTrue(mostRecentHolder instanceof DownloadHistoryItemViewHolder);
        return ((DownloadHistoryItemViewHolder) mostRecentHolder).getItemView();
    }
}
