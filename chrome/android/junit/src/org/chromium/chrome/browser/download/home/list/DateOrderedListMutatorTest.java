// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.download.home.list;

import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import org.junit.After;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnit;
import org.mockito.junit.MockitoRule;
import org.robolectric.annotation.Config;

import org.chromium.base.CollectionUtil;
import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.chrome.browser.download.home.filter.OfflineItemFilterSource;
import org.chromium.chrome.browser.modelutil.ListObservable.ListObserver;
import org.chromium.components.offline_items_collection.OfflineItem;

import java.util.Calendar;
import java.util.Collections;

/** Unit tests for the DateOrderedListMutator class. */
@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class DateOrderedListMutatorTest {
    @Mock
    OfflineItemFilterSource mSource;

    @Mock
    ListObserver mObserver;

    DateOrderedListModel mModel;

    @Rule
    public MockitoRule mMockitoRule = MockitoJUnit.rule();

    @Before
    public void setUp() {
        mModel = new DateOrderedListModel();
    }

    @After
    public void tearDown() {
        mModel = null;
    }

    /**
     * Action                               List
     * 1. Set()                             [ ]
     */
    @Test
    public void testNoItemsAndSetup() {
        when(mSource.getItems()).thenReturn(Collections.emptySet());
        DateOrderedListMutator list = new DateOrderedListMutator(mSource, mModel);
        verify(mSource, times(1)).addObserver(list);

        Assert.assertEquals(0, mModel.getItemCount());
    }

    /**
     * Action                               List
     * 1. Set(item1 @ 1:00 1/1/2018)        [ HEADER @ 0:00 1/1/2018,
     *                                        item1  @ 1:00 1/1/2018 ]
     */
    @Test
    public void testSingleItem() {
        OfflineItem item1 = buildItem("1", buildCalendar(2018, 1, 1, 1));
        when(mSource.getItems()).thenReturn(CollectionUtil.newArrayList(item1));
        DateOrderedListMutator list = new DateOrderedListMutator(mSource, mModel);

        Assert.assertEquals(2, mModel.getItemCount());
        assertListItemEquals(mModel.getItemAt(0), buildCalendar(2018, 1, 1, 0), null);
        assertListItemEquals(mModel.getItemAt(1), buildCalendar(2018, 1, 1, 1), item1);
    }

    /**
     * Action                               List
     * 1. Set(item1 @ 1:00 1/1/2018,        [ HEADER @ 0:00 1/1/2018,
     *        item2 @ 2:00 1/1/2018)          item1  @ 1:00 1/1/2018,
     *                                        item2  @ 2:00 1/1/2018 ]
     */
    @Test
    public void testTwoItemsSameDay() {
        OfflineItem item1 = buildItem("1", buildCalendar(2018, 1, 1, 1));
        OfflineItem item2 = buildItem("2", buildCalendar(2018, 1, 1, 2));
        when(mSource.getItems()).thenReturn(CollectionUtil.newArrayList(item1, item2));
        DateOrderedListMutator list = new DateOrderedListMutator(mSource, mModel);

        Assert.assertEquals(3, mModel.getItemCount());
        assertListItemEquals(mModel.getItemAt(0), buildCalendar(2018, 1, 1, 0), null);
        assertListItemEquals(mModel.getItemAt(1), buildCalendar(2018, 1, 1, 1), item1);
        assertListItemEquals(mModel.getItemAt(2), buildCalendar(2018, 1, 1, 2), item2);
    }

    /**
     * Action                               List
     * 1. Set(item1 @ 0:00 1/1/2018,        [ HEADER @ 0:00 1/1/2018,
     *        item2 @ 0:00 1/2/2018)          item1  @ 0:00 1/1/2018,
     *                                        HEADER @ 0:00 1/2/2018,
     *                                        item2  @ 0:00 1/1/2018 ]
     */
    @Test
    public void testTwoItemsDifferentDayMatchHeader() {
        OfflineItem item1 = buildItem("1", buildCalendar(2018, 1, 1, 0));
        OfflineItem item2 = buildItem("2", buildCalendar(2018, 1, 2, 0));
        when(mSource.getItems()).thenReturn(CollectionUtil.newArrayList(item1, item2));
        DateOrderedListMutator list = new DateOrderedListMutator(mSource, mModel);

        Assert.assertEquals(4, mModel.getItemCount());
        assertListItemEquals(mModel.getItemAt(0), buildCalendar(2018, 1, 1, 0), null);
        assertListItemEquals(mModel.getItemAt(1), buildCalendar(2018, 1, 1, 0), item1);
        assertListItemEquals(mModel.getItemAt(2), buildCalendar(2018, 1, 2, 0), null);
        assertListItemEquals(mModel.getItemAt(3), buildCalendar(2018, 1, 2, 0), item2);
    }

    /**
     * Action                               List
     * 1. Set(item1 @ 5:00 1/1/2018,        [ HEADER @ 0:00 1/1/2018,
     *        item2 @ 4:00 1/1/2018)          item2  @ 4:00 1/1/2018,
     *                                        item1  @ 5:00 1/1/2018 ]
     */
    @Test
    public void testTwoItemsSameDayOutOfOrder() {
        OfflineItem item1 = buildItem("1", buildCalendar(2018, 1, 1, 5));
        OfflineItem item2 = buildItem("2", buildCalendar(2018, 1, 1, 4));
        when(mSource.getItems()).thenReturn(CollectionUtil.newArrayList(item1, item2));
        DateOrderedListMutator list = new DateOrderedListMutator(mSource, mModel);

        Assert.assertEquals(3, mModel.getItemCount());
        assertListItemEquals(mModel.getItemAt(0), buildCalendar(2018, 1, 1, 0), null);
        assertListItemEquals(mModel.getItemAt(1), buildCalendar(2018, 1, 1, 4), item2);
        assertListItemEquals(mModel.getItemAt(2), buildCalendar(2018, 1, 1, 5), item1);
    }

    /**
     * Action                               List
     * 1. Set(item1 @ 5:00 1/1/2018,        [ HEADER @ 0:00 1/1/2018,
     *        item2 @ 4:00 1/2/2018)          item2  @ 5:00 1/1/2018,
     *                                        HEADER @ 0:00 1/2/2018,
     *                                        item1  @ 4:00 1/2/2018 ]
     */
    @Test
    public void testTwoItemsDifferentDay() {
        OfflineItem item1 = buildItem("1", buildCalendar(2018, 1, 1, 5));
        OfflineItem item2 = buildItem("2", buildCalendar(2018, 1, 2, 4));
        when(mSource.getItems()).thenReturn(CollectionUtil.newArrayList(item1, item2));
        DateOrderedListMutator list = new DateOrderedListMutator(mSource, mModel);

        Assert.assertEquals(4, mModel.getItemCount());
        assertListItemEquals(mModel.getItemAt(0), buildCalendar(2018, 1, 1, 0), null);
        assertListItemEquals(mModel.getItemAt(1), buildCalendar(2018, 1, 1, 5), item1);
        assertListItemEquals(mModel.getItemAt(2), buildCalendar(2018, 1, 2, 0), null);
        assertListItemEquals(mModel.getItemAt(3), buildCalendar(2018, 1, 2, 4), item2);
    }

    /**
     * Action                               List
     * 1. Set(item1 @ 3:00 1/2/2018,        [ HEADER @ 0:00 1/1/2018,
     *        item2 @ 4:00 1/1/2018)          item2  @ 4:00 1/1/2018,
     *                                        HEADER @ 0:00 1/2/2018,
     *                                        item1  @ 3:00 1/2/2018 ]
     */
    @Test
    public void testTwoItemsDifferentDayOutOfOrder() {
        OfflineItem item1 = buildItem("1", buildCalendar(2018, 1, 2, 3));
        OfflineItem item2 = buildItem("2", buildCalendar(2018, 1, 1, 4));
        when(mSource.getItems()).thenReturn(CollectionUtil.newArrayList(item1, item2));
        DateOrderedListMutator list = new DateOrderedListMutator(mSource, mModel);

        Assert.assertEquals(4, mModel.getItemCount());
        assertListItemEquals(mModel.getItemAt(0), buildCalendar(2018, 1, 1, 0), null);
        assertListItemEquals(mModel.getItemAt(1), buildCalendar(2018, 1, 1, 4), item2);
        assertListItemEquals(mModel.getItemAt(2), buildCalendar(2018, 1, 2, 0), null);
        assertListItemEquals(mModel.getItemAt(3), buildCalendar(2018, 1, 2, 3), item1);
    }

    /**
     * Action                               List
     * 1. Set()                             [ ]
     *
     * 2. Add(item1 @ 4:00 1/1/2018)        [ HEADER @ 0:00 1/1/2018,
     *                                        item1  @ 4:00 1/1/2018 ]
     */
    @Test
    public void testAddItemToEmptyList() {
        when(mSource.getItems()).thenReturn(Collections.emptySet());
        DateOrderedListMutator list = new DateOrderedListMutator(mSource, mModel);
        mModel.addObserver(mObserver);

        OfflineItem item1 = buildItem("1", buildCalendar(2018, 1, 1, 4));
        when(mSource.getItems()).thenReturn(CollectionUtil.newArrayList(item1));
        list.onItemsAdded(CollectionUtil.newArrayList(item1));

        verify(mObserver, times(1)).onItemRangeInserted(mModel, 0, 2);
        Assert.assertEquals(2, mModel.getItemCount());
        assertListItemEquals(mModel.getItemAt(0), buildCalendar(2018, 1, 1, 0), null);
        assertListItemEquals(mModel.getItemAt(1), buildCalendar(2018, 1, 1, 4), item1);
    }

    /**
     * Action                               List
     * 1. Set(item1 @ 2:00 1/2/2018)        [ HEADER @ 0:00 1/2/2018,
     *                                        item1  @ 2:00 1/2/2018 ]
     *
     * 2. Add(item2 @ 1:00 1/2/2018)        [ HEADER @ 0:00 1/2/2018,
     *                                        item2  @ 1:00 1/2/2018
     *                                        item1  @ 2:00 1/2/2018 ]
     */
    @Test
    public void testAddFirstItemToList() {
        OfflineItem item1 = buildItem("1", buildCalendar(2018, 1, 2, 2));
        when(mSource.getItems()).thenReturn(CollectionUtil.newArrayList(item1));
        DateOrderedListMutator list = new DateOrderedListMutator(mSource, mModel);
        mModel.addObserver(mObserver);

        // Add an item on the same day that will be placed first.
        OfflineItem item2 = buildItem("2", buildCalendar(2018, 1, 2, 1));
        when(mSource.getItems()).thenReturn(CollectionUtil.newArrayList(item1, item2));
        list.onItemsAdded(CollectionUtil.newArrayList(item2));

        verify(mObserver, times(1)).onItemRangeInserted(mModel, 1, 1);
        Assert.assertEquals(3, mModel.getItemCount());
        assertListItemEquals(mModel.getItemAt(0), buildCalendar(2018, 1, 2, 0), null);
        assertListItemEquals(mModel.getItemAt(1), buildCalendar(2018, 1, 2, 1), item2);
        assertListItemEquals(mModel.getItemAt(2), buildCalendar(2018, 1, 2, 2), item1);

        // Add an item on an earlier day that will be placed first.
        OfflineItem item3 = buildItem("3", buildCalendar(2018, 1, 1, 2));
        when(mSource.getItems()).thenReturn(CollectionUtil.newArrayList(item1, item2, item3));
        list.onItemsAdded(CollectionUtil.newArrayList(item3));

        verify(mObserver, times(1)).onItemRangeInserted(mModel, 0, 2);
        Assert.assertEquals(5, mModel.getItemCount());
        assertListItemEquals(mModel.getItemAt(0), buildCalendar(2018, 1, 1, 0), null);
        assertListItemEquals(mModel.getItemAt(1), buildCalendar(2018, 1, 1, 2), item3);
        assertListItemEquals(mModel.getItemAt(2), buildCalendar(2018, 1, 2, 0), null);
        assertListItemEquals(mModel.getItemAt(3), buildCalendar(2018, 1, 2, 1), item2);
        assertListItemEquals(mModel.getItemAt(4), buildCalendar(2018, 1, 2, 2), item1);
    }

    /**
     * Action                               List
     * 1. Set(item1 @ 2:00 1/2/2018)        [ HEADER @ 0:00 1/2/2018,
     *                                        item1  @ 2:00 1/2/2018 ]
     *
     * 2. Add(item2 @ 3:00 1/2/2018)        [ HEADER @ 0:00 1/2/2018,
     *                                        item1  @ 2:00 1/2/2018
     *                                        item2  @ 3:00 1/2/2018 ]
     *
     * 3. Add(item3 @ 4:00 1/3/2018)        [ HEADER @ 0:00 1/2/2018,
     *                                        item1  @ 2:00 1/2/2018
     *                                        item2  @ 3:00 1/2/2018,
     *                                        HEADER @ 0:00 1/3/2018,
     *                                        item3  @ 4:00 1/3/2018
     */
    @Test
    public void testAddLastItemToList() {
        OfflineItem item1 = buildItem("1", buildCalendar(2018, 1, 2, 2));
        when(mSource.getItems()).thenReturn(CollectionUtil.newArrayList(item1));
        DateOrderedListMutator list = new DateOrderedListMutator(mSource, mModel);
        mModel.addObserver(mObserver);

        // Add an item on the same day that will be placed last.
        OfflineItem item2 = buildItem("2", buildCalendar(2018, 1, 2, 3));
        when(mSource.getItems()).thenReturn(CollectionUtil.newArrayList(item1, item2));
        list.onItemsAdded(CollectionUtil.newArrayList(item2));

        verify(mObserver, times(1)).onItemRangeInserted(mModel, 2, 1);
        Assert.assertEquals(3, mModel.getItemCount());
        assertListItemEquals(mModel.getItemAt(0), buildCalendar(2018, 1, 2, 0), null);
        assertListItemEquals(mModel.getItemAt(1), buildCalendar(2018, 1, 2, 2), item1);
        assertListItemEquals(mModel.getItemAt(2), buildCalendar(2018, 1, 2, 3), item2);

        // Add an item on a later day that will be placed last.
        OfflineItem item3 = buildItem("3", buildCalendar(2018, 1, 3, 4));
        when(mSource.getItems()).thenReturn(CollectionUtil.newArrayList(item1, item2, item3));
        list.onItemsAdded(CollectionUtil.newArrayList(item3));

        verify(mObserver, times(1)).onItemRangeInserted(mModel, 3, 2);
        Assert.assertEquals(5, mModel.getItemCount());
        assertListItemEquals(mModel.getItemAt(0), buildCalendar(2018, 1, 2, 0), null);
        assertListItemEquals(mModel.getItemAt(1), buildCalendar(2018, 1, 2, 2), item1);
        assertListItemEquals(mModel.getItemAt(2), buildCalendar(2018, 1, 2, 3), item2);
        assertListItemEquals(mModel.getItemAt(3), buildCalendar(2018, 1, 3, 0), null);
        assertListItemEquals(mModel.getItemAt(4), buildCalendar(2018, 1, 3, 4), item3);
    }

    /**
     * Action                               List
     * 1. Set(item1 @ 2:00 1/2/2018)        [ HEADER @ 0:00 1/2/2018,
     *                                        item1  @ 2:00 1/2/2018 ]
     *
     * 2. Remove(item1)                     [ ]
     */
    @Test
    public void testRemoveOnlyItemInList() {
        OfflineItem item1 = buildItem("1", buildCalendar(2018, 1, 2, 2));
        when(mSource.getItems()).thenReturn(CollectionUtil.newArrayList(item1));
        DateOrderedListMutator list = new DateOrderedListMutator(mSource, mModel);
        mModel.addObserver(mObserver);

        when(mSource.getItems()).thenReturn(Collections.emptySet());
        list.onItemsRemoved(CollectionUtil.newArrayList(item1));

        verify(mObserver, times(1)).onItemRangeRemoved(mModel, 0, 2);
        Assert.assertEquals(0, mModel.getItemCount());
    }

    /**
     * Action                               List
     * 1. Set(item1 @ 2:00 1/2/2018,        [ HEADER @ 0:00 1/2/2018,
     *        item2 @ 3:00 1/2/2018)          item1  @ 2:00 1/2/2018,
     *                                        item2  @ 3:00 1/2/2018 ]
     *
     * 2. Remove(item1)                     [ HEADER @ 0:00 1/2/2018,
     *                                        item2  @ 3:00 1/2/2018 ]
     */
    @Test
    public void testRemoveFirstItemInListSameDay() {
        OfflineItem item1 = buildItem("1", buildCalendar(2018, 1, 2, 2));
        OfflineItem item2 = buildItem("2", buildCalendar(2018, 1, 2, 3));
        when(mSource.getItems()).thenReturn(CollectionUtil.newArrayList(item1, item2));
        DateOrderedListMutator list = new DateOrderedListMutator(mSource, mModel);
        mModel.addObserver(mObserver);

        when(mSource.getItems()).thenReturn(CollectionUtil.newArrayList(item2));
        list.onItemsRemoved(CollectionUtil.newArrayList(item1));

        verify(mObserver, times(1)).onItemRangeRemoved(mModel, 1, 1);
        Assert.assertEquals(2, mModel.getItemCount());
        assertListItemEquals(mModel.getItemAt(0), buildCalendar(2018, 1, 2, 0), null);
        assertListItemEquals(mModel.getItemAt(1), buildCalendar(2018, 1, 2, 3), item2);
    }

    /**
     * Action                               List
     * 1. Set(item1 @ 2:00 1/2/2018,        [ HEADER @ 0:00 1/2/2018,
     *        item2 @ 3:00 1/2/2018)          item1  @ 2:00 1/2/2018,
     *                                        item2  @ 3:00 1/2/2018 ]
     *
     * 2. Remove(item2)                     [ HEADER @ 0:00 1/2/2018,
     *                                        item1  @ 2:00 1/2/2018 ]
     */
    @Test
    public void testRemoveLastItemInListSameDay() {
        OfflineItem item1 = buildItem("1", buildCalendar(2018, 1, 2, 2));
        OfflineItem item2 = buildItem("2", buildCalendar(2018, 1, 2, 3));
        when(mSource.getItems()).thenReturn(CollectionUtil.newArrayList(item1, item2));
        DateOrderedListMutator list = new DateOrderedListMutator(mSource, mModel);
        mModel.addObserver(mObserver);

        when(mSource.getItems()).thenReturn(CollectionUtil.newArrayList(item1));
        list.onItemsRemoved(CollectionUtil.newArrayList(item2));

        verify(mObserver, times(1)).onItemRangeRemoved(mModel, 2, 1);
        Assert.assertEquals(2, mModel.getItemCount());
        assertListItemEquals(mModel.getItemAt(0), buildCalendar(2018, 1, 2, 0), null);
        assertListItemEquals(mModel.getItemAt(1), buildCalendar(2018, 1, 2, 2), item1);
    }

    /**
     * Action                               List
     * 1. Set(item1 @ 2:00 1/2/2018,        [ HEADER @ 0:00 1/2/2018,
     *        item2 @ 3:00 1/3/2018)          item1  @ 2:00 1/2/2018,
     *                                        HEADER @ 0:00 1/3/2018,
     *                                        item2  @ 3:00 1/3/2018 ]
     *
     * 2. Remove(item2)                     [ HEADER @ 0:00 1/2/2018,
     *                                        item1  @ 2:00 1/2/2018 ]
     */
    @Test
    public void testRemoveLastItemInListWithMultipleDays() {
        OfflineItem item1 = buildItem("1", buildCalendar(2018, 1, 2, 2));
        OfflineItem item2 = buildItem("2", buildCalendar(2018, 1, 3, 3));
        when(mSource.getItems()).thenReturn(CollectionUtil.newArrayList(item1, item2));
        DateOrderedListMutator list = new DateOrderedListMutator(mSource, mModel);
        mModel.addObserver(mObserver);

        when(mSource.getItems()).thenReturn(CollectionUtil.newArrayList(item1));
        list.onItemsRemoved(CollectionUtil.newArrayList(item2));

        verify(mObserver, times(1)).onItemRangeRemoved(mModel, 2, 2);
        Assert.assertEquals(2, mModel.getItemCount());
        assertListItemEquals(mModel.getItemAt(0), buildCalendar(2018, 1, 2, 0), null);
        assertListItemEquals(mModel.getItemAt(1), buildCalendar(2018, 1, 2, 2), item1);
    }

    /**
     * Action                               List
     * 1. Set()                             [ ]
     *
     * 2. Add(item1 @ 4:00  1/1/2018,       [ HEADER @ 0:00  1/1/2018,
     *        item2 @ 6:00  1/1/2018,         item1  @ 4:00  1/1/2018,
     *        item3 @ 12:00 1/2/2018,         item2  @ 6:00  1/1/2018,
     *        item4 @ 10:00 1/2/2018)         HEADER @ 0:00  1/2/2018,
     *                                        item4  @ 10:00 1/3/2018,
     *                                        item3  @ 12:00 1/3/2018 ]
     */
    @Test
    public void testAddMultipleItems() {
        when(mSource.getItems()).thenReturn(Collections.emptySet());
        DateOrderedListMutator list = new DateOrderedListMutator(mSource, mModel);
        mModel.addObserver(mObserver);

        OfflineItem item1 = buildItem("1", buildCalendar(2018, 1, 1, 4));
        OfflineItem item2 = buildItem("2", buildCalendar(2018, 1, 1, 6));
        OfflineItem item3 = buildItem("3", buildCalendar(2018, 1, 2, 12));
        OfflineItem item4 = buildItem("4", buildCalendar(2018, 1, 2, 10));

        when(mSource.getItems())
                .thenReturn(CollectionUtil.newArrayList(item1, item2, item3, item4));
        list.onItemsAdded(CollectionUtil.newArrayList(item1, item2, item3, item4));

        verify(mObserver, times(1)).onItemRangeInserted(mModel, 0, 6);
        Assert.assertEquals(6, mModel.getItemCount());
        assertListItemEquals(mModel.getItemAt(0), buildCalendar(2018, 1, 1, 0), null);
        assertListItemEquals(mModel.getItemAt(1), buildCalendar(2018, 1, 1, 4), item1);
        assertListItemEquals(mModel.getItemAt(2), buildCalendar(2018, 1, 1, 6), item2);
        assertListItemEquals(mModel.getItemAt(3), buildCalendar(2018, 1, 2, 0), null);
        assertListItemEquals(mModel.getItemAt(4), buildCalendar(2018, 1, 2, 10), item4);
        assertListItemEquals(mModel.getItemAt(5), buildCalendar(2018, 1, 2, 12), item3);
    }

    /**
     * Action                               List
     * 1. Set(item1 @ 4:00  1/1/2018,       [ HEADER @ 0:00  1/1/2018,
     *        item2 @ 6:00  1/1/2018,         item1  @ 4:00  1/1/2018,
     *        item3 @ 12:00 1/2/2018,         item2  @ 6:00  1/1/2018,
     *        item4 @ 10:00 1/2/2018)         HEADER @ 0:00  1/2/2018,
     *                                        item4  @ 10:00 1/3/2018,
     *                                        item3  @ 12:00 1/3/2018 ]
     *
     * 2. Remove(item1,                     [ HEADER @ 0:00  1/1/2018,
     *           item3,                       item2  @ 6:00  1/1/2018 ]
     *           item4)
     */
    @Test
    public void testRemoveMultipleItems() {
        OfflineItem item1 = buildItem("1", buildCalendar(2018, 1, 1, 4));
        OfflineItem item2 = buildItem("2", buildCalendar(2018, 1, 1, 6));
        OfflineItem item3 = buildItem("3", buildCalendar(2018, 1, 2, 12));
        OfflineItem item4 = buildItem("4", buildCalendar(2018, 1, 2, 10));

        when(mSource.getItems())
                .thenReturn(CollectionUtil.newArrayList(item1, item2, item3, item4));
        DateOrderedListMutator list = new DateOrderedListMutator(mSource, mModel);
        mModel.addObserver(mObserver);

        when(mSource.getItems()).thenReturn(CollectionUtil.newArrayList(item2));
        list.onItemsRemoved(CollectionUtil.newArrayList(item1, item3, item4));

        verify(mObserver, times(1)).onItemRangeRemoved(mModel, 3, 3);
        verify(mObserver, times(1)).onItemRangeRemoved(mModel, 1, 1);
        Assert.assertEquals(2, mModel.getItemCount());
        assertListItemEquals(mModel.getItemAt(0), buildCalendar(2018, 1, 1, 0), null);
        assertListItemEquals(mModel.getItemAt(1), buildCalendar(2018, 1, 1, 6), item2);
    }

    /**
     * Action                               List
     * 1. Set(item1 @ 4:00 1/1/2018)        [ HEADER    @ 0:00  1/1/2018,
     *                                        item1     @ 4:00  1/1/2018 ]
     *
     * 2. Update (item1,
     *            newItem1 @ 4:00 1/1/2018) [ HEADER    @ 0:00  1/1/2018,
     *                                        newItem1  @ 4:00  1/1/2018 ]
     */
    @Test
    public void testItemUpdatedSameTimestamp() {
        OfflineItem item1 = buildItem("1", buildCalendar(2018, 1, 1, 4));

        when(mSource.getItems()).thenReturn(CollectionUtil.newArrayList(item1));
        DateOrderedListMutator list = new DateOrderedListMutator(mSource, mModel);
        mModel.addObserver(mObserver);

        // Update an item with the same timestamp.
        OfflineItem newItem1 = buildItem("1", buildCalendar(2018, 1, 1, 4));
        when(mSource.getItems()).thenReturn(CollectionUtil.newArrayList(newItem1));
        list.onItemUpdated(item1, newItem1);

        verify(mObserver, times(1)).onItemRangeChanged(mModel, 1, 1, null);
        Assert.assertEquals(2, mModel.getItemCount());
        assertListItemEquals(mModel.getItemAt(0), buildCalendar(2018, 1, 1, 0), null);
        assertListItemEquals(mModel.getItemAt(1), buildCalendar(2018, 1, 1, 4), newItem1);
    }

    /**
     * Action                               List
     * 1. Set(item1 @ 4:00 1/1/2018,        [ HEADER    @ 0:00  1/1/2018,
     *        item2 @ 5:00 1/1/2018)          item1     @ 4:00  1/1/2018,
     *                                        item2     @ 5:00  1/1/2018
     * 2. Update (item1,
     *            newItem1 @ 6:00 1/1/2018) [ HEADER    @ 0:00  1/1/2018,
     *                                        item2     @ 5:00  1/1/2018,
     *                                        newItem1  @ 6:00  1/1/2018 ]
     */
    @Test
    public void testItemUpdatedSameDay() {
        OfflineItem item1 = buildItem("1", buildCalendar(2018, 1, 1, 4));
        OfflineItem item2 = buildItem("2", buildCalendar(2018, 1, 1, 5));

        when(mSource.getItems()).thenReturn(CollectionUtil.newArrayList(item1, item2));
        DateOrderedListMutator list = new DateOrderedListMutator(mSource, mModel);
        mModel.addObserver(mObserver);

        // Update an item with the same timestamp.
        OfflineItem newItem1 = buildItem("1", buildCalendar(2018, 1, 1, 6));
        when(mSource.getItems()).thenReturn(CollectionUtil.newArrayList(newItem1, item2));
        list.onItemUpdated(item1, newItem1);

        verify(mObserver, times(1)).onItemRangeRemoved(mModel, 1, 1);
        verify(mObserver, times(1)).onItemRangeInserted(mModel, 2, 1);
        Assert.assertEquals(3, mModel.getItemCount());
        assertListItemEquals(mModel.getItemAt(0), buildCalendar(2018, 1, 1, 0), null);
        assertListItemEquals(mModel.getItemAt(1), buildCalendar(2018, 1, 1, 5), item2);
        assertListItemEquals(mModel.getItemAt(2), buildCalendar(2018, 1, 1, 6), newItem1);
    }

    /**
     * Action                               List
     * 1. Set(item1 @ 4:00 1/1/2018)        [ HEADER    @ 0:00  1/1/2018,
     *                                        item1     @ 4:00  1/1/2018 ]
     *
     * 2. Update (item1,
     *            newItem1 @ 6:00 1/2/2018) [ HEADER    @ 0:00  1/2/2018,
     *                                        newItem1  @ 6:00  1/2/2018 ]
     */
    @Test
    public void testItemUpdatedDifferentDay() {
        OfflineItem item1 = buildItem("1", buildCalendar(2018, 1, 1, 4));

        when(mSource.getItems()).thenReturn(CollectionUtil.newArrayList(item1));
        DateOrderedListMutator list = new DateOrderedListMutator(mSource, mModel);
        mModel.addObserver(mObserver);

        // Update an item with the same timestamp.
        OfflineItem newItem1 = buildItem("1", buildCalendar(2018, 1, 2, 6));
        when(mSource.getItems()).thenReturn(CollectionUtil.newArrayList(newItem1));
        list.onItemUpdated(item1, newItem1);

        verify(mObserver, times(1)).onItemRangeRemoved(mModel, 0, 2);
        verify(mObserver, times(1)).onItemRangeInserted(mModel, 0, 2);
        Assert.assertEquals(2, mModel.getItemCount());
        assertListItemEquals(mModel.getItemAt(0), buildCalendar(2018, 1, 2, 0), null);
        assertListItemEquals(mModel.getItemAt(1), buildCalendar(2018, 1, 2, 6), newItem1);
    }

    private static Calendar buildCalendar(int year, int month, int dayOfMonth, int hourOfDay) {
        Calendar calendar = CalendarFactory.get();
        calendar.set(year, month, dayOfMonth, hourOfDay, 0);
        return calendar;
    }

    private static OfflineItem buildItem(String id, Calendar calendar) {
        OfflineItem item = new OfflineItem();
        item.id.namespace = "test";
        item.id.id = id;
        item.creationTimeMs = calendar.getTimeInMillis();
        return item;
    }

    private static void assertListItemEquals(
            DateOrderedListModel.ListItem item, Calendar calendar, OfflineItem offlineItem) {
        if (offlineItem == null) {
            Assert.assertEquals(
                    DateOrderedListModel.ListItem.generateStableIdForDayOfYear(calendar),
                    item.stableId);
        } else {
            Assert.assertEquals(
                    DateOrderedListModel.ListItem.generateStableId(offlineItem), item.stableId);
        }

        Assert.assertEquals(offlineItem, item.item);
        Calendar calendar2 = CalendarFactory.get();
        calendar2.setTime(item.date);
        Assert.assertEquals(calendar.getTimeInMillis(), calendar2.getTimeInMillis());
        Assert.assertEquals(calendar.getTimeInMillis(), item.date.getTime());
    }
}