// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.modelutil;

import static org.hamcrest.CoreMatchers.is;
import static org.junit.Assert.assertThat;
import static org.mockito.Mockito.verify;

import android.support.test.filters.SmallTest;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.robolectric.annotation.Config;

import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.base.test.util.Feature;

/**
 * Basic test ensuring the {@link SimpleListObservable} notifies listeners properly.
 */
@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class SimpleListObservableTest {
    @Mock
    private ListObservable.ListObserver mObserver;

    private SimpleListObservable<Integer> mIntegerList = new SimpleListObservable<>();

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);
        mIntegerList.addObserver(mObserver);
    }

    @After
    public void tearDown() {
        mIntegerList.removeObserver(mObserver);
    }

    @Test
    @SmallTest
    @Feature({"keyboard-accessory"})
    public void testNotifiesSuccessfulInsertions() {
        // Setting an empty list ist always an insertion.
        assertThat(mIntegerList.getItemCount(), is(0));
        mIntegerList.set(new Integer[] {333, 88888888, 22});
        verify(mObserver).onItemRangeInserted(mIntegerList, 0, 3);
        assertThat(mIntegerList.getItemCount(), is(3));
        assertThat(mIntegerList.get(1), is(88888888));

        // Adding Items is always an insertion.
        mIntegerList.add(55555);
        verify(mObserver).onItemRangeInserted(mIntegerList, 3, 1);
        assertThat(mIntegerList.getItemCount(), is(4));
        assertThat(mIntegerList.get(3), is(55555));
    }

    @Test
    @SmallTest
    @Feature({"keyboard-accessory"})
    public void testModelNotifiesSuccessfulRemoval() {
        Integer eightEights = 88888888;
        mIntegerList.set(new Integer[] {333, eightEights, 22});
        assertThat(mIntegerList.getItemCount(), is(3));

        // Removing any item by instance is always a removal.
        mIntegerList.remove(eightEights);
        verify(mObserver).onItemRangeRemoved(mIntegerList, 1, 1);

        // Setting an empty list is a removal of all items.
        mIntegerList.set(new Integer[] {});
        verify(mObserver).onItemRangeRemoved(mIntegerList, 0, 2);
    }

    @Test
    @SmallTest
    @Feature({"keyboard-accessory"})
    public void testModelNotifiesReplacedData() {
        // The initial setting is an insertion.
        mIntegerList.set(new Integer[] {333, 88888888, 22});
        verify(mObserver).onItemRangeInserted(mIntegerList, 0, 3);

        // Setting non-empty data is a change of all elements.
        mIntegerList.set(new Integer[] {4444, 22});
        verify(mObserver).onItemRangeChanged(mIntegerList, 0, 3, mIntegerList);

        // Setting data of same values is a change.
        mIntegerList.set(new Integer[] {4444, 22, 1, 666666});
        verify(mObserver).onItemRangeChanged(mIntegerList, 0, 4, mIntegerList);

        // Setting empty data is a removal.
        mIntegerList.set(new Integer[] {});
        verify(mObserver).onItemRangeRemoved(mIntegerList, 0, 4);
    }
}