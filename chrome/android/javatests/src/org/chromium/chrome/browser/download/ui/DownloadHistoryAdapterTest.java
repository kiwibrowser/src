// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.download.ui;

import static org.chromium.chrome.browser.widget.DateDividedAdapter.TYPE_DATE;
import static org.chromium.chrome.browser.widget.DateDividedAdapter.TYPE_HEADER;
import static org.chromium.chrome.browser.widget.DateDividedAdapter.TYPE_NORMAL;

import android.content.SharedPreferences.Editor;
import android.support.test.filters.SmallTest;
import android.support.v7.widget.RecyclerView;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.ContextUtils;
import org.chromium.base.ThreadUtils;
import org.chromium.base.test.BaseJUnit4ClassRunner;
import org.chromium.base.test.util.CallbackHelper;
import org.chromium.chrome.browser.ChromeFeatureList;
import org.chromium.chrome.browser.download.DownloadItem;
import org.chromium.chrome.browser.download.ui.StubbedProvider.StubbedDownloadDelegate;
import org.chromium.chrome.browser.download.ui.StubbedProvider.StubbedOfflineContentProvider;
import org.chromium.chrome.browser.test.ChromeBrowserTestRule;
import org.chromium.components.download.DownloadState;
import org.chromium.components.offline_items_collection.ContentId;
import org.chromium.components.offline_items_collection.OfflineItem;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Set;

/**
 * Tests a DownloadHistoryAdapter that is isolated from the real bridges.
 */
@RunWith(BaseJUnit4ClassRunner.class)
public class DownloadHistoryAdapterTest {
    @Rule
    public final ChromeBrowserTestRule mBrowserTestRule = new ChromeBrowserTestRule();

    private static class Observer extends RecyclerView.AdapterDataObserver
            implements DownloadHistoryAdapter.TestObserver, SpaceDisplay.Observer {
        public CallbackHelper onChangedCallback = new CallbackHelper();
        public CallbackHelper onDownloadItemCreatedCallback = new CallbackHelper();
        public CallbackHelper onDownloadItemUpdatedCallback = new CallbackHelper();
        public CallbackHelper onOfflineItemCreatedCallback = new CallbackHelper();
        public CallbackHelper onOfflineItemUpdatedCallback = new CallbackHelper();
        public CallbackHelper onSpaceDisplayUpdatedCallback = new CallbackHelper();

        public DownloadItem createdItem;
        public DownloadItem updatedItem;

        @Override
        public void onChanged() {
            onChangedCallback.notifyCalled();
        }

        @Override
        public void onDownloadItemCreated(DownloadItem item) {
            createdItem = item;
            onDownloadItemCreatedCallback.notifyCalled();
        }

        @Override
        public void onDownloadItemUpdated(DownloadItem item) {
            updatedItem = item;
            onDownloadItemUpdatedCallback.notifyCalled();
        }

        @Override
        public void onOfflineItemCreated(OfflineItem item) {
            onOfflineItemCreatedCallback.notifyCalled();
        }

        @Override
        public void onOfflineItemUpdated(OfflineItem item) {
            onOfflineItemUpdatedCallback.notifyCalled();
        }

        @Override
        public void onSpaceDisplayUpdated(SpaceDisplay spaceDisplay) {
            onSpaceDisplayUpdatedCallback.notifyCalled();
        }
    }

    /**
     * Object for use in {@link #checkAdapterContents(Object...)} that corresponds to
     * {@link #TYPE_HEADER}.
     */
    private static final Integer HEADER = -1;

    private static final String PREF_SHOW_STORAGE_INFO_HEADER =
            "download_home_show_storage_info_header";

    private DownloadHistoryAdapter mAdapter;
    private Observer mObserver;
    private StubbedDownloadDelegate mDownloadDelegate;
    private StubbedOfflineContentProvider mOfflineContentProvider;
    private StubbedProvider mBackendProvider;

    @Before
    public void setUp() throws Exception {
        mBackendProvider = new StubbedProvider();
        mDownloadDelegate = mBackendProvider.getDownloadDelegate();
        mOfflineContentProvider = mBackendProvider.getOfflineContentProvider();
        Editor editor = ContextUtils.getAppSharedPreferences().edit();
        editor.putBoolean(PREF_SHOW_STORAGE_INFO_HEADER, true).apply();

        HashMap<String, Boolean> features = new HashMap<String, Boolean>();
        features.put(ChromeFeatureList.DOWNLOADS_LOCATION_CHANGE, false);
        features.put(ChromeFeatureList.DOWNLOAD_HOME_SHOW_STORAGE_INFO, false);
        ChromeFeatureList.setTestFeatures(features);
    }

    private void initializeAdapter(boolean showOffTheRecord, boolean hasHeader) throws Exception {
        mObserver = new Observer();
        mAdapter = new DownloadHistoryAdapter(showOffTheRecord, null);
        mAdapter.registerAdapterDataObserver(mObserver);
        mAdapter.registerObserverForTest(mObserver);

        // Initialize the Adapter with all the DownloadItems and OfflineItems.
        int callCount = mObserver.onChangedCallback.getCallCount();
        int onSpaceDisplayUpdatedCallCount = mObserver.onSpaceDisplayUpdatedCallback.getCallCount();
        Assert.assertEquals(0, callCount);
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                mAdapter.initialize(mBackendProvider, null);
            }
        });
        mAdapter.getSpaceDisplayForTests().addObserverForTests(mObserver);
        mDownloadDelegate.addCallback.waitForCallback(0);
        // If header should be added, onChanged() will be called twice because both setHeaders()
        // and loadMoreItems() will call notifyDataSetChanged(). Otherwise, setHeaders() will not
        // be called and onChanged() will only be called once.
        mObserver.onChangedCallback.waitForCallback(callCount, hasHeader ? 2 : 1);
        mObserver.onSpaceDisplayUpdatedCallback.waitForCallback(onSpaceDisplayUpdatedCallCount);
    }

    private void onDownloadItemCreated(final DownloadItem item, int numberOfCallsToWaitFor)
            throws Exception {
        int callCount = mObserver.onChangedCallback.getCallCount();
        int onSpaceDisplayUpdatedCallCount = mObserver.onSpaceDisplayUpdatedCallback.getCallCount();
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                mAdapter.onDownloadItemCreated(item);
            }
        });
        if (numberOfCallsToWaitFor > 0) {
            mObserver.onChangedCallback.waitForCallback(callCount, numberOfCallsToWaitFor);
            mObserver.onSpaceDisplayUpdatedCallback.waitForCallback(onSpaceDisplayUpdatedCallCount);
        }
    }

    private void onDownloadItemUpdated(final DownloadItem item, int numberOfCallsToWaitFor)
            throws Exception {
        int callCount = mObserver.onDownloadItemUpdatedCallback.getCallCount();
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                mAdapter.onDownloadItemUpdated(item);
            }
        });
        if (numberOfCallsToWaitFor > 0) {
            mObserver.onDownloadItemUpdatedCallback.waitForCallback(
                    callCount, numberOfCallsToWaitFor);
        }
    }

    private void onDownloadItemRemoved(final String id, final boolean isOffTheRecord,
            int numberOfCallsToWaitFor) throws Exception {
        int callCount = mObserver.onChangedCallback.getCallCount();
        int onSpaceDisplayUpdatedCallCount = mObserver.onSpaceDisplayUpdatedCallback.getCallCount();
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                mAdapter.onDownloadItemRemoved(id, isOffTheRecord);
            }
        });
        if (numberOfCallsToWaitFor > 0) {
            mObserver.onChangedCallback.waitForCallback(callCount, numberOfCallsToWaitFor);
            mObserver.onSpaceDisplayUpdatedCallback.waitForCallback(onSpaceDisplayUpdatedCallCount);
        }
    }

    private void onOfflineItemAdded(final OfflineItem item, int numberOfCallsToWaitFor)
            throws Exception {
        int callCount = mObserver.onChangedCallback.getCallCount();
        final ArrayList<OfflineItem> items = new ArrayList<>();
        items.add(item);
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                mOfflineContentProvider.observer.onItemsAdded(items);
            }
        });
        if (numberOfCallsToWaitFor > 0) {
            mObserver.onChangedCallback.waitForCallback(callCount, numberOfCallsToWaitFor);
        }
    }

    private void onOfflineItemUpdated(final OfflineItem item, int numberOfCallsToWaitFor)
            throws Exception {
        int callCount = mObserver.onOfflineItemUpdatedCallback.getCallCount();
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                mOfflineContentProvider.observer.onItemUpdated(item);
            }
        });
        if (numberOfCallsToWaitFor > 0) {
            mObserver.onChangedCallback.waitForCallback(callCount, numberOfCallsToWaitFor);
        }
    }

    private void onOfflineItemDeleted(ContentId id, int numberOfCallsToWaitFor) throws Exception {
        int callCount = mObserver.onChangedCallback.getCallCount();
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                mOfflineContentProvider.observer.onItemRemoved(id);
            }
        });
        if (numberOfCallsToWaitFor > 0) {
            mObserver.onChangedCallback.waitForCallback(callCount, numberOfCallsToWaitFor);
        }
    }

    private void onFilterChanged(final @DownloadFilter.Type int flag, int numberOfCallsToWaitFor)
            throws Exception {
        int callCount = mObserver.onChangedCallback.getCallCount();
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                mAdapter.onFilterChanged(flag);
            }
        });
        if (numberOfCallsToWaitFor > 0) {
            mObserver.onChangedCallback.waitForCallback(callCount, numberOfCallsToWaitFor);
        }
    }

    /** Nothing downloaded, nothing shown. */
    @Test
    @SmallTest
    public void testInitialize_Empty() throws Exception {
        initializeAdapter(false, false);
        Assert.assertEquals(0, mAdapter.getItemCount());
        Assert.assertEquals(0, mAdapter.getTotalDownloadSize());

        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                mAdapter.onManagerDestroyed();
            }
        });

        mDownloadDelegate.removeCallback.waitForCallback(0);
    }

    /** One downloaded item should show the item and a date header. */
    @Test
    @SmallTest
    public void testInitialize_SingleItem() throws Exception {
        DownloadItem item = StubbedProvider.createDownloadItem(0, "19840116 12:00");
        mDownloadDelegate.regularItems.add(item);
        initializeAdapter(false, true);
        checkAdapterContents(HEADER, null, item);
        Assert.assertEquals(1, mAdapter.getTotalDownloadSize());
    }

    /** Two items downloaded on the same day should end up in the same group, in recency order. */
    @Test
    @SmallTest
    public void testInitialize_TwoItemsOneDate() throws Exception {
        DownloadItem item0 = StubbedProvider.createDownloadItem(0, "19840116 12:00");
        DownloadItem item1 = StubbedProvider.createDownloadItem(1, "19840116 12:01");
        mDownloadDelegate.regularItems.add(item0);
        mDownloadDelegate.regularItems.add(item1);
        initializeAdapter(false, true);
        checkAdapterContents(HEADER, null, item1, item0);
        Assert.assertEquals(11, mAdapter.getTotalDownloadSize());
    }

    /** Two items downloaded on different days should end up in different date groups. */
    @Test
    @SmallTest
    public void testInitialize_TwoItemsTwoDates() throws Exception {
        DownloadItem item0 = StubbedProvider.createDownloadItem(0, "19840116 12:00");
        DownloadItem item1 = StubbedProvider.createDownloadItem(1, "19840117 12:00");
        mDownloadDelegate.regularItems.add(item0);
        mDownloadDelegate.regularItems.add(item1);
        initializeAdapter(false, true);
        checkAdapterContents(HEADER, null, item1, null, item0);
        Assert.assertEquals(11, mAdapter.getTotalDownloadSize());
    }

    /** Storage header shouldn't show up if user has already turned it off. */
    @Test
    @SmallTest
    public void testInitialize_SingleItemNoStorageHeader() throws Exception {
        Editor editor = ContextUtils.getAppSharedPreferences().edit();
        editor.putBoolean(PREF_SHOW_STORAGE_INFO_HEADER, false).apply();
        DownloadItem item = StubbedProvider.createDownloadItem(0, "19840116 12:00");
        mDownloadDelegate.regularItems.add(item);
        initializeAdapter(false, false);
        checkAdapterContents(null, item);
        Assert.assertEquals(1, mAdapter.getTotalDownloadSize());
    }

    /** Toggle the info button. Storage header should turn off/on accordingly. */
    @Test
    @SmallTest
    public void testToggleStorageHeader() throws Exception {
        DownloadItem item0 = StubbedProvider.createDownloadItem(0, "19840116 12:00");
        DownloadItem item1 = StubbedProvider.createDownloadItem(1, "19840116 12:01");
        mDownloadDelegate.regularItems.add(item0);
        mDownloadDelegate.regularItems.add(item1);
        initializeAdapter(false, true);
        checkAdapterContents(HEADER, null, item1, item0);
        Assert.assertEquals(11, mAdapter.getTotalDownloadSize());

        // Turn off info and check that header is gone.
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                mAdapter.setShowStorageInfoHeader(false);
            }
        });
        checkAdapterContents(null, item1, item0);

        // Turn on info and check that header is back again.
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                mAdapter.setShowStorageInfoHeader(true);
            }
        });
        checkAdapterContents(HEADER, null, item1, item0);
    }

    /** Off the record downloads are ignored if the DownloadHistoryAdapter isn't watching them. */
    @Test
    @SmallTest
    public void testInitialize_OffTheRecord_Ignored() throws Exception {
        DownloadItem item0 = StubbedProvider.createDownloadItem(0, "19840116 12:00");
        DownloadItem item1 = StubbedProvider.createDownloadItem(1, "19840116 12:01");
        mDownloadDelegate.regularItems.add(item0);
        mDownloadDelegate.offTheRecordItems.add(item1);
        initializeAdapter(false, true);
        checkAdapterContents(HEADER, null, item0);
        Assert.assertEquals(1, mAdapter.getTotalDownloadSize());
    }

    /** A regular and a off the record item with the same date are bucketed together. */
    @Test
    @SmallTest
    public void testInitialize_OffTheRecord_TwoItemsOneDate() throws Exception {
        DownloadItem item0 = StubbedProvider.createDownloadItem(0, "19840116 18:00");
        DownloadItem item1 = StubbedProvider.createDownloadItem(1, "19840116 12:00");
        mDownloadDelegate.regularItems.add(item0);
        mDownloadDelegate.offTheRecordItems.add(item1);
        initializeAdapter(true, true);
        checkAdapterContents(HEADER, null, item0, item1);
        Assert.assertEquals(11, mAdapter.getTotalDownloadSize());
    }

    /** Test that all the download item types intermingle correctly. */
    @Test
    @SmallTest
    public void testInitialize_ThreeItemsDifferentKinds() throws Exception {
        DownloadItem item0 = StubbedProvider.createDownloadItem(0, "19840116 18:00");
        DownloadItem item1 = StubbedProvider.createDownloadItem(1, "19840116 12:00");
        OfflineItem item2 = StubbedProvider.createOfflineItem(2, "19840117 6:00");
        mDownloadDelegate.regularItems.add(item0);
        mDownloadDelegate.offTheRecordItems.add(item1);
        mOfflineContentProvider.items.add(item2);
        initializeAdapter(true, true);
        checkAdapterContents(HEADER, null, item2, null, item0, item1);
        Assert.assertEquals(100011, mAdapter.getTotalDownloadSize());
    }

    /** Adding and updating new items should bucket them into the proper dates. */
    @Test
    @SmallTest
    public void testUpdate_UpdateItems() throws Exception {
        // Start with an empty Adapter.
        initializeAdapter(false, false);
        Assert.assertEquals(0, mAdapter.getItemCount());
        Assert.assertEquals(0, mAdapter.getTotalDownloadSize());

        // Add the first item.
        Assert.assertEquals(1, mObserver.onChangedCallback.getCallCount());
        DownloadItem item0 = StubbedProvider.createDownloadItem(0, "19840116 12:00");
        onDownloadItemCreated(item0, 2);
        checkAdapterContents(HEADER, null, item0);
        Assert.assertEquals(1, mAdapter.getTotalDownloadSize());

        // Add a second item with a different date.
        Assert.assertEquals(3, mObserver.onChangedCallback.getCallCount());
        DownloadItem item1 = StubbedProvider.createDownloadItem(1, "19840117 12:00");
        onDownloadItemCreated(item1, 2);
        checkAdapterContents(HEADER, null, item1, null, item0);
        Assert.assertEquals(11, mAdapter.getTotalDownloadSize());

        // Add a third item with the same date as the second item.
        Assert.assertEquals(2, mObserver.onDownloadItemCreatedCallback.getCallCount());
        DownloadItem item2 = StubbedProvider.createDownloadItem(
                2, "19840117 18:00", false, DownloadState.IN_PROGRESS, 0);
        onDownloadItemCreated(item2, 2);
        mObserver.onDownloadItemCreatedCallback.waitForCallback(2);
        Assert.assertEquals(mObserver.createdItem, item2);
        checkAdapterContents(HEADER, null, item2, item1, null, item0);
        Assert.assertEquals(11, mAdapter.getTotalDownloadSize());

        // An item with the same download ID as the second item should just update the old one,
        // but it should now be visible.
        DownloadItem item3 = StubbedProvider.createDownloadItem(
                2, "19840117 18:00", false, DownloadState.COMPLETE, 100);
        onDownloadItemUpdated(item3, 1);
        Assert.assertEquals(mObserver.updatedItem, item3);
        checkAdapterContents(HEADER, null, item3, item1, null, item0);
        Assert.assertEquals(111, mAdapter.getTotalDownloadSize());

        // Throw on a new OfflineItem.
        OfflineItem item4 = StubbedProvider.createOfflineItem(0, "19840117 19:00");
        onOfflineItemAdded(item4, 2);
        checkAdapterContents(HEADER, null, item4, item3, item1, null, item0);

        // Update the existing OfflineItem.
        OfflineItem item5 = StubbedProvider.createOfflineItem(0, "19840117 19:00");
        onOfflineItemUpdated(item5, 2);
        checkAdapterContents(HEADER, null, item5, item3, item1, null, item0);
    }

    /** Test removal of items. */
    @Test
    @SmallTest
    public void testRemove_ThreeItemsTwoDates() throws Exception {
        // Initialize the DownloadHistoryAdapter with three items in two date buckets.
        DownloadItem regularItem = StubbedProvider.createDownloadItem(0, "19840116 18:00");
        DownloadItem offTheRecordItem = StubbedProvider.createDownloadItem(
                1, "19840116 12:00", true, DownloadState.COMPLETE, 100);
        OfflineItem offlineItem = StubbedProvider.createOfflineItem(2, "19840117 12:01");
        mDownloadDelegate.regularItems.add(regularItem);
        mDownloadDelegate.offTheRecordItems.add(offTheRecordItem);
        mOfflineContentProvider.items.add(offlineItem);
        initializeAdapter(true, true);
        checkAdapterContents(HEADER, null, offlineItem, null, regularItem, offTheRecordItem);
        Assert.assertEquals(100011, mAdapter.getTotalDownloadSize());

        // Remove an item from the date bucket with two items. Wait for two callbacks as
        // notifyDataSetChanged() is called once when setHeaders() is called and once when items
        // are loaded.
        Assert.assertEquals(2, mObserver.onChangedCallback.getCallCount());
        onDownloadItemRemoved(offTheRecordItem.getId(), true, 2);
        checkAdapterContents(HEADER, null, offlineItem, null, regularItem);
        Assert.assertEquals(100001, mAdapter.getTotalDownloadSize());

        // Remove an item from the second bucket, which removes the bucket entirely.
        Assert.assertEquals(4, mObserver.onChangedCallback.getCallCount());
        onOfflineItemDeleted(offlineItem.id, 2);
        checkAdapterContents(HEADER, null, regularItem);
        Assert.assertEquals(1, mAdapter.getTotalDownloadSize());

        // Remove the last item in the list.
        Assert.assertEquals(6, mObserver.onChangedCallback.getCallCount());
        onDownloadItemRemoved(regularItem.getId(), false, 1);
        Assert.assertEquals(0, mAdapter.getItemCount());
        Assert.assertEquals(0, mAdapter.getTotalDownloadSize());
    }

    /** Test filtering of items. */
    @Test
    @SmallTest
    public void testFilter_SevenItems() throws Exception {
        DownloadItem item0 = StubbedProvider.createDownloadItem(0, "19840116 12:00");
        DownloadItem item1 = StubbedProvider.createDownloadItem(1, "19840116 12:01");
        DownloadItem item2 = StubbedProvider.createDownloadItem(2, "19840117 12:00");
        DownloadItem item3 = StubbedProvider.createDownloadItem(3, "19840117 12:01");
        DownloadItem item4 = StubbedProvider.createDownloadItem(4, "19840118 12:00");
        DownloadItem item5 = StubbedProvider.createDownloadItem(5, "19840118 12:01");
        OfflineItem item6 = StubbedProvider.createOfflineItem(0, "19840118 6:00");
        mDownloadDelegate.regularItems.add(item0);
        mDownloadDelegate.offTheRecordItems.add(item1);
        mDownloadDelegate.regularItems.add(item2);
        mDownloadDelegate.regularItems.add(item3);
        mDownloadDelegate.offTheRecordItems.add(item4);
        mDownloadDelegate.regularItems.add(item5);
        mOfflineContentProvider.items.add(item6);
        initializeAdapter(true, true);
        checkAdapterContents(
                HEADER, null, item5, item4, item6, null, item3, item2, null, item1, item0);
        Assert.assertEquals(1666, mAdapter.getTotalDownloadSize());

        onFilterChanged(DownloadFilter.FILTER_AUDIO, 2);
        checkAdapterContents(HEADER, null, item5, item4);
        Assert.assertEquals(1666, mAdapter.getTotalDownloadSize()); // Total size ignores filters.

        onFilterChanged(DownloadFilter.FILTER_VIDEO, 2);
        checkAdapterContents(HEADER, null, item3);

        onFilterChanged(DownloadFilter.FILTER_IMAGE, 2);
        checkAdapterContents(HEADER, null, item1, item0);

        onFilterChanged(DownloadFilter.FILTER_PAGE, 2);
        checkAdapterContents(HEADER, null, item6);

        onFilterChanged(DownloadFilter.FILTER_ALL, 2);
        checkAdapterContents(
                HEADER, null, item5, item4, item6, null, item3, item2, null, item1, item0);
        Assert.assertEquals(1666, mAdapter.getTotalDownloadSize());
    }

    /** Tests that the list is updated appropriately when Offline Pages are deleted. */
    @Test
    @SmallTest
    public void testFilter_AfterOfflineDeletions() throws Exception {
        OfflineItem item0 = StubbedProvider.createOfflineItem(0, "19840116 6:00");
        OfflineItem item1 = StubbedProvider.createOfflineItem(1, "19840116 12:00");
        OfflineItem item2 = StubbedProvider.createOfflineItem(2, "19840120 6:00");
        mOfflineContentProvider.items.add(item0);
        mOfflineContentProvider.items.add(item1);
        mOfflineContentProvider.items.add(item2);
        initializeAdapter(false, true);
        checkAdapterContents(HEADER, null, item2, null, item1, item0);
        Assert.assertEquals(111000, mAdapter.getTotalDownloadSize());

        // Filter shows everything.
        onOfflineItemDeleted(item1.id, 2);
        checkAdapterContents(HEADER, null, item2, null, item0);

        // Filter shows nothing when the item is deleted because it's a different kind of item.
        onFilterChanged(DownloadFilter.FILTER_AUDIO, 1);
        Assert.assertEquals(0, mAdapter.getItemCount());
        onOfflineItemDeleted(item0.id, 0);
        Assert.assertEquals(0, mAdapter.getItemCount());

        // Filter shows just pages.
        onFilterChanged(DownloadFilter.FILTER_PAGE, 2);
        checkAdapterContents(HEADER, null, item2);
        onOfflineItemDeleted(item2.id, 1);
        Assert.assertEquals(0, mAdapter.getItemCount());
    }

    @Test
    @SmallTest
    public void testInProgress_FilePathMapAccurate() throws Exception {
        Set<DownloadHistoryItemWrapper> toDelete;

        initializeAdapter(false, false);
        Assert.assertEquals(0, mAdapter.getItemCount());
        Assert.assertEquals(0, mAdapter.getTotalDownloadSize());

        // Simulate the creation of a new item by providing a DownloadItem without a path.
        DownloadItem itemCreated = StubbedProvider.createDownloadItem(
                9, "19840118 12:01", false, DownloadState.IN_PROGRESS, 0);
        onDownloadItemCreated(itemCreated, 0);
        mObserver.onDownloadItemCreatedCallback.waitForCallback(0);
        Assert.assertEquals(mObserver.createdItem, itemCreated);

        checkAdapterContents();
        toDelete = mAdapter.getItemsForFilePath(itemCreated.getDownloadInfo().getFilePath());
        Assert.assertNull(toDelete);

        // Update the Adapter with new information about the item.
        DownloadItem itemUpdated = StubbedProvider.createDownloadItem(
                10, "19840118 12:01", false, DownloadState.IN_PROGRESS, 50);
        onDownloadItemUpdated(itemUpdated, 1);
        Assert.assertEquals(mObserver.updatedItem, itemUpdated);

        checkAdapterContents(HEADER, null, itemUpdated);
        toDelete = mAdapter.getItemsForFilePath(itemUpdated.getDownloadInfo().getFilePath());
        Assert.assertNull(toDelete);

        // Tell the Adapter that the item has finished downloading.
        DownloadItem itemCompleted = StubbedProvider.createDownloadItem(
                10, "19840118 12:01", false, DownloadState.COMPLETE, 100);
        onDownloadItemUpdated(itemCompleted, 1);
        Assert.assertEquals(mObserver.updatedItem, itemCompleted);
        checkAdapterContents(HEADER, null, itemCompleted);

        // Confirm that the file now shows up when trying to delete it.
        toDelete = mAdapter.getItemsForFilePath(itemCompleted.getDownloadInfo().getFilePath());
        Assert.assertEquals(1, toDelete.size());
        Assert.assertEquals(itemCompleted.getId(), toDelete.iterator().next().getId());
    }

    @Test
    @SmallTest
    public void testSearch_NoFilter() throws Exception {
        DownloadItem item0 = StubbedProvider.createDownloadItem(0, "19840116 12:00");
        DownloadItem item1 = StubbedProvider.createDownloadItem(1, "19840116 12:01");
        DownloadItem item2 = StubbedProvider.createDownloadItem(2, "19840117 12:00");
        DownloadItem item3 = StubbedProvider.createDownloadItem(3, "19840117 12:01");
        DownloadItem item4 = StubbedProvider.createDownloadItem(4, "19840118 12:00");
        DownloadItem item5 = StubbedProvider.createDownloadItem(5, "19840118 12:01");
        OfflineItem item6 = StubbedProvider.createOfflineItem(0, "19840118 6:00");
        mDownloadDelegate.regularItems.add(item0);
        mDownloadDelegate.offTheRecordItems.add(item1);
        mDownloadDelegate.regularItems.add(item2);
        mDownloadDelegate.regularItems.add(item3);
        mDownloadDelegate.offTheRecordItems.add(item4);
        mDownloadDelegate.regularItems.add(item5);
        mOfflineContentProvider.items.add(item6);
        initializeAdapter(true, true);
        checkAdapterContents(
                HEADER, null, item5, item4, item6, null, item3, item2, null, item1, item0);

        // Perform a search that matches the file name for a few downloads.
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                mAdapter.search("FiLe");
            }
        });

        // Only items matching the query should be shown.
        checkAdapterContents(null, item2, null, item1, item0);

        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                mAdapter.onEndSearch();
            }
        });

        // All items should be shown again after the search is ended.
        checkAdapterContents(
                HEADER, null, item5, item4, item6, null, item3, item2, null, item1, item0);

        // Perform a search that matches the hostname for a couple downloads.
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                mAdapter.search("oNE");
            }
        });

        checkAdapterContents(null, item4, null, item1);
    }

    @Test
    @SmallTest
    public void testSearch_WithFilter() throws Exception {
        DownloadItem item0 = StubbedProvider.createDownloadItem(0, "19840116 12:00");
        DownloadItem item1 = StubbedProvider.createDownloadItem(1, "19840116 12:01");
        DownloadItem item2 = StubbedProvider.createDownloadItem(2, "19840117 12:00");
        DownloadItem item3 = StubbedProvider.createDownloadItem(3, "19840117 12:01");
        DownloadItem item4 = StubbedProvider.createDownloadItem(4, "19840118 12:00");
        DownloadItem item5 = StubbedProvider.createDownloadItem(5, "19840118 12:01");
        OfflineItem item6 = StubbedProvider.createOfflineItem(0, "19840118 6:00");
        mDownloadDelegate.regularItems.add(item0);
        mDownloadDelegate.offTheRecordItems.add(item1);
        mDownloadDelegate.regularItems.add(item2);
        mDownloadDelegate.regularItems.add(item3);
        mDownloadDelegate.offTheRecordItems.add(item4);
        mDownloadDelegate.regularItems.add(item5);
        mOfflineContentProvider.items.add(item6);
        initializeAdapter(true, true);
        checkAdapterContents(
                HEADER, null, item5, item4, item6, null, item3, item2, null, item1, item0);

        // Change the filter
        onFilterChanged(DownloadFilter.FILTER_IMAGE, 2);
        checkAdapterContents(HEADER, null, item1, item0);

        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                mAdapter.search("FiRSt");
            }
        });

        // Only items matching both the filter and the search query should be shown.
        checkAdapterContents(null, item0);

        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                mAdapter.onEndSearch();
            }
        });

        // All items matching the filter should be shown after the search is ended.
        checkAdapterContents(HEADER, null, item1, item0);
    }

    @Test
    @SmallTest
    public void testSearch_RemoveItem() throws Exception {
        final DownloadItem item0 = StubbedProvider.createDownloadItem(0, "19840116 12:00");
        final DownloadItem item1 = StubbedProvider.createDownloadItem(1, "19840116 12:01");
        final DownloadItem item2 = StubbedProvider.createDownloadItem(2, "19840117 12:00");
        final DownloadItem item3 = StubbedProvider.createDownloadItem(3, "19840117 12:01");
        final DownloadItem item4 = StubbedProvider.createDownloadItem(4, "19840118 12:00");
        final DownloadItem item5 = StubbedProvider.createDownloadItem(5, "19840118 12:01");
        OfflineItem item6 = StubbedProvider.createOfflineItem(0, "19840118 6:00");
        mDownloadDelegate.regularItems.add(item0);
        mDownloadDelegate.offTheRecordItems.add(item1);
        mDownloadDelegate.regularItems.add(item2);
        mDownloadDelegate.regularItems.add(item3);
        mDownloadDelegate.offTheRecordItems.add(item4);
        mDownloadDelegate.regularItems.add(item5);
        mOfflineContentProvider.items.add(item6);
        initializeAdapter(true, true);
        checkAdapterContents(
                HEADER, null, item5, item4, item6, null, item3, item2, null, item1, item0);

        // Perform a search that matches the file name for a few downloads.
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                mAdapter.search("FiLe");
            }
        });
        // Only items matching the query should be shown.
        checkAdapterContents(null, item2, null, item1, item0);

        onDownloadItemRemoved(item1.getId(), false, 1);

        checkAdapterContents(null, item2, null, item0);
    }

    /** Checks that the adapter has the correct items in the right places. */
    private void checkAdapterContents(Object... expectedItems) {
        if (expectedItems.length == 0) {
            Assert.assertEquals(0, mAdapter.getItemCount());
            return;
        }

        Assert.assertEquals(expectedItems.length, mAdapter.getItemCount());

        for (int i = 0; i < expectedItems.length; i++) {
            if (HEADER.equals(expectedItems[i])) {
                Assert.assertEquals("The header should be the first item in the adapter.", 0, i);
                Assert.assertEquals(TYPE_HEADER, mAdapter.getItemViewType(i));
            } else if (expectedItems[i] == null) {
                // Expect a date.
                // TODO(dfalcantara): Check what date the header is showing.
                Assert.assertEquals(TYPE_DATE, mAdapter.getItemViewType(i));
            } else {
                // Expect a particular item.
                Assert.assertEquals(TYPE_NORMAL, mAdapter.getItemViewType(i));
                Assert.assertEquals(expectedItems[i],
                        ((DownloadHistoryItemWrapper) mAdapter.getItemAt(i).second).getItem());
            }
        }
    }

}
