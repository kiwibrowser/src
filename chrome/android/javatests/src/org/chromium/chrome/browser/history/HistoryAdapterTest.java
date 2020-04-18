// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.history;

import static org.chromium.chrome.browser.widget.DateDividedAdapter.TYPE_DATE;
import static org.chromium.chrome.browser.widget.DateDividedAdapter.TYPE_HEADER;
import static org.chromium.chrome.browser.widget.DateDividedAdapter.TYPE_NORMAL;

import android.support.test.filters.SmallTest;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.ContextUtils;
import org.chromium.base.ThreadUtils;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.widget.selection.SelectionDelegate;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;

import java.util.Date;
import java.util.concurrent.TimeUnit;

/**
 * Tests for the {@link HistoryAdapter}.
 */
@RunWith(ChromeJUnit4ClassRunner.class)
public class HistoryAdapterTest {
    private StubbedHistoryProvider mHistoryProvider;
    private HistoryAdapter mAdapter;

    @Before
    public void setUp() throws Exception {
        mHistoryProvider = new StubbedHistoryProvider();
        mAdapter = new HistoryAdapter(new SelectionDelegate<HistoryItem>(), null, mHistoryProvider);
        mAdapter.generateHeaderItemsForTest();
    }

    private void initializeAdapter() {
        ThreadUtils.runOnUiThreadBlocking(new Runnable(){
            @Override
            public void run() {
                mAdapter.initialize();
            }
        });
    }

    @Test
    @SmallTest
    public void testInitialize_Empty() {
        initializeAdapter();
        checkAdapterContents(false);
    }

    @Test
    @SmallTest
    public void testInitialize_SingleItem() {
        Date today = new Date();
        long timestamp = today.getTime();
        HistoryItem item1 = StubbedHistoryProvider.createHistoryItem(0, timestamp);
        mHistoryProvider.addItem(item1);

        initializeAdapter();

        // There should be three items - the header, a date and the history item.
        checkAdapterContents(true, null, null, item1);
    }

    @Test
    @SmallTest
    public void testRemove_TwoItemsOneDate() {
        Date today = new Date();
        long timestamp = today.getTime();
        HistoryItem item1 = StubbedHistoryProvider.createHistoryItem(0, timestamp);
        mHistoryProvider.addItem(item1);

        HistoryItem item2 = StubbedHistoryProvider.createHistoryItem(1, timestamp);
        mHistoryProvider.addItem(item2);

        initializeAdapter();

        // There should be four items - the list header, a date header and two history items.
        checkAdapterContents(true, null, null, item1, item2);

        mAdapter.markItemForRemoval(item1);

        // Check that one item was removed.
        checkAdapterContents(true, null, null, item2);
        Assert.assertEquals(1, mHistoryProvider.markItemForRemovalCallback.getCallCount());
        Assert.assertEquals(0, mHistoryProvider.removeItemsCallback.getCallCount());

        mAdapter.markItemForRemoval(item2);

        // There should no longer be any items in the adapter.
        checkAdapterContents(false);
        Assert.assertEquals(2, mHistoryProvider.markItemForRemovalCallback.getCallCount());
        Assert.assertEquals(0, mHistoryProvider.removeItemsCallback.getCallCount());

        mAdapter.removeItems();
        Assert.assertEquals(1, mHistoryProvider.removeItemsCallback.getCallCount());
    }

    @Test
    @SmallTest
    public void testRemove_TwoItemsTwoDates() {
        Date today = new Date();
        long timestamp = today.getTime();
        HistoryItem item1 = StubbedHistoryProvider.createHistoryItem(0, timestamp);
        mHistoryProvider.addItem(item1);

        long timestamp2 = today.getTime() - TimeUnit.DAYS.toMillis(3);
        HistoryItem item2 = StubbedHistoryProvider.createHistoryItem(1, timestamp2);
        mHistoryProvider.addItem(item2);

        initializeAdapter();

        // There should be five items - the list header, a date header, a history item, another
        // date header and another history item.
        checkAdapterContents(true, null, null, item1, null, item2);

        mAdapter.markItemForRemoval(item1);

        // Check that the first item and date header were removed.
        checkAdapterContents(true, null, null, item2);
        Assert.assertEquals(1, mHistoryProvider.markItemForRemovalCallback.getCallCount());
        Assert.assertEquals(0, mHistoryProvider.removeItemsCallback.getCallCount());

        mAdapter.markItemForRemoval(item2);

        // There should no longer be any items in the adapter.
        checkAdapterContents(false);
        Assert.assertEquals(2, mHistoryProvider.markItemForRemovalCallback.getCallCount());
        Assert.assertEquals(0, mHistoryProvider.removeItemsCallback.getCallCount());

        mAdapter.removeItems();
        Assert.assertEquals(1, mHistoryProvider.removeItemsCallback.getCallCount());
    }

    @Test
    @SmallTest
    public void testSearch() {
        Date today = new Date();
        long timestamp = today.getTime();
        HistoryItem item1 = StubbedHistoryProvider.createHistoryItem(0, timestamp);
        mHistoryProvider.addItem(item1);

        long timestamp2 = today.getTime() - TimeUnit.DAYS.toMillis(3);
        HistoryItem item2 = StubbedHistoryProvider.createHistoryItem(1, timestamp2);
        mHistoryProvider.addItem(item2);

        initializeAdapter();
        checkAdapterContents(true, null, null, item1, null, item2);

        mAdapter.search("google");

        // The header should be hidden during the search.
        checkAdapterContents(false, null, item1);

        mAdapter.onEndSearch();

        // The header should be shown again after the search.
        checkAdapterContents(true, null, null, item1, null, item2);
    }

    @Test
    @SmallTest
    public void testLoadMoreItems() {
        Date today = new Date();
        long timestamp = today.getTime();
        HistoryItem item1 = StubbedHistoryProvider.createHistoryItem(0, timestamp);
        mHistoryProvider.addItem(item1);

        HistoryItem item2 = StubbedHistoryProvider.createHistoryItem(1, timestamp);
        mHistoryProvider.addItem(item2);

        HistoryItem item3 = StubbedHistoryProvider.createHistoryItem(2, timestamp);
        mHistoryProvider.addItem(item3);

        HistoryItem item4 = StubbedHistoryProvider.createHistoryItem(3, timestamp);
        mHistoryProvider.addItem(item4);

        long timestamp2 = today.getTime() - TimeUnit.DAYS.toMillis(2);
        HistoryItem item5 = StubbedHistoryProvider.createHistoryItem(4, timestamp2);
        mHistoryProvider.addItem(item5);

        HistoryItem item6 = StubbedHistoryProvider.createHistoryItem(0, timestamp2);
        mHistoryProvider.addItem(item6);

        long timestamp3 = today.getTime() - TimeUnit.DAYS.toMillis(4);
        HistoryItem item7 = StubbedHistoryProvider.createHistoryItem(1, timestamp3);
        mHistoryProvider.addItem(item7);

        initializeAdapter();

        // Only the first five of the seven items should be loaded.
        checkAdapterContents(true, null, null, item1, item2, item3, item4, null, item5);
        Assert.assertTrue(mAdapter.canLoadMoreItems());

        mAdapter.loadMoreItems();

        // All items should now be loaded.
        checkAdapterContents(true, null, null, item1, item2, item3, item4, null, item5, item6,
                null, item7);
        Assert.assertFalse(mAdapter.canLoadMoreItems());
    }

    @Test
    @SmallTest
    public void testOnHistoryDeleted() throws Exception {
        Date today = new Date();
        long timestamp = today.getTime();
        HistoryItem item1 = StubbedHistoryProvider.createHistoryItem(0, timestamp);
        mHistoryProvider.addItem(item1);

        initializeAdapter();

        checkAdapterContents(true, null, null, item1);

        mHistoryProvider.removeItem(item1);

        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                mAdapter.onHistoryDeleted();
            }
        });

        checkAdapterContents(false);
    }

    @Test
    @SmallTest
    public void testBlockedSite() {
        Date today = new Date();
        long timestamp = today.getTime();
        HistoryItem item1 = StubbedHistoryProvider.createHistoryItem(0, timestamp);
        mHistoryProvider.addItem(item1);

        HistoryItem item2 = StubbedHistoryProvider.createHistoryItem(5, timestamp);
        mHistoryProvider.addItem(item2);

        initializeAdapter();

        checkAdapterContents(true, null, null, item1, item2);
        Assert.assertEquals(ContextUtils.getApplicationContext().getString(
                                    R.string.android_history_blocked_site),
                item2.getTitle());
        Assert.assertTrue(item2.wasBlockedVisit());
    }

    private void checkAdapterContents(boolean hasHeader, Object... expectedItems) {
        Assert.assertEquals(expectedItems.length, mAdapter.getItemCount());
        Assert.assertEquals(hasHeader, mAdapter.hasListHeader());

        for (int i = 0; i < expectedItems.length; i++) {
            if (i == 0 && hasHeader) {
                Assert.assertEquals(TYPE_HEADER, mAdapter.getItemViewType(i));
                continue;
            }

            if (expectedItems[i] == null) {
                // TODO(twellington): Check what date header is showing.
                Assert.assertEquals(TYPE_DATE, mAdapter.getItemViewType(i));
            } else {
                Assert.assertEquals(TYPE_NORMAL, mAdapter.getItemViewType(i));
                Assert.assertEquals(expectedItems[i], mAdapter.getItemAt(i).second);
            }
        }
    }
}
