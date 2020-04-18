// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.download.home.filter;

import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import org.junit.Assert;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnit;
import org.mockito.junit.MockitoRule;
import org.robolectric.annotation.Config;

import org.chromium.base.CollectionUtil;
import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.components.offline_items_collection.OfflineItem;
import org.chromium.components.offline_items_collection.OfflineItemFilter;

import java.util.Collection;

/** Unit tests for the TypeOfflineItemFilter class. */
@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class TypeOfflineItemFilterTest {
    @Mock
    private OfflineItemFilterSource mSource;

    @Mock
    private OfflineItemFilterObserver mObserver;

    @Rule
    public MockitoRule mMockitoRule = MockitoJUnit.rule();

    @Test
    public void testTypeFiltering() {
        OfflineItem item1 = buildItem(OfflineItemFilter.FILTER_PAGE, false);
        OfflineItem item2 = buildItem(OfflineItemFilter.FILTER_VIDEO, false);
        OfflineItem item3 = buildItem(OfflineItemFilter.FILTER_AUDIO, false);
        OfflineItem item4 = buildItem(OfflineItemFilter.FILTER_IMAGE, false);
        OfflineItem item5 = buildItem(OfflineItemFilter.FILTER_OTHER, false);
        OfflineItem item6 = buildItem(OfflineItemFilter.FILTER_DOCUMENT, false);
        OfflineItem item7 = buildItem(OfflineItemFilter.FILTER_PAGE, true);
        OfflineItem item8 = buildItem(OfflineItemFilter.FILTER_VIDEO, true);
        Collection<OfflineItem> sourceItems =
                CollectionUtil.newHashSet(item1, item2, item3, item4, item5, item6, item7, item8);
        when(mSource.getItems()).thenReturn(sourceItems);

        TypeOfflineItemFilter filter = new TypeOfflineItemFilter(mSource);
        filter.addObserver(mObserver);
        Assert.assertEquals(CollectionUtil.newHashSet(item1, item2, item3, item4, item5, item6),
                filter.getItems());

        filter.onFilterSelected(Filters.VIDEOS);
        verify(mObserver, times(1))
                .onItemsRemoved(CollectionUtil.newHashSet(item1, item3, item4, item5, item6));
        Assert.assertEquals(CollectionUtil.newHashSet(item2), filter.getItems());

        filter.onFilterSelected(Filters.MUSIC);
        verify(mObserver, times(1)).onItemsRemoved(CollectionUtil.newHashSet(item2));
        verify(mObserver, times(1)).onItemsAdded(CollectionUtil.newHashSet(item3));
        Assert.assertEquals(CollectionUtil.newHashSet(item3), filter.getItems());

        filter.onFilterSelected(Filters.IMAGES);
        verify(mObserver, times(1)).onItemsRemoved(CollectionUtil.newHashSet(item3));
        verify(mObserver, times(1)).onItemsAdded(CollectionUtil.newHashSet(item4));
        Assert.assertEquals(CollectionUtil.newHashSet(item4), filter.getItems());

        filter.onFilterSelected(Filters.SITES);
        verify(mObserver, times(1)).onItemsRemoved(CollectionUtil.newHashSet(item4));
        verify(mObserver, times(1)).onItemsAdded(CollectionUtil.newHashSet(item1));
        Assert.assertEquals(CollectionUtil.newHashSet(item1), filter.getItems());

        filter.onFilterSelected(Filters.OTHER);
        verify(mObserver, times(1)).onItemsRemoved(CollectionUtil.newHashSet(item1));
        verify(mObserver, times(1)).onItemsAdded(CollectionUtil.newHashSet(item5, item6));
        Assert.assertEquals(CollectionUtil.newHashSet(item5, item6), filter.getItems());

        filter.onFilterSelected(Filters.PREFETCHED);
        verify(mObserver, times(1)).onItemsRemoved(CollectionUtil.newHashSet(item5, item6));
        verify(mObserver, times(1)).onItemsAdded(CollectionUtil.newHashSet(item7, item8));
        Assert.assertEquals(CollectionUtil.newHashSet(item7, item8), filter.getItems());

        filter.onFilterSelected(Filters.NONE);
        verify(mObserver, times(1))
                .onItemsAdded(CollectionUtil.newHashSet(item1, item2, item3, item4, item5, item6));
        Assert.assertEquals(CollectionUtil.newHashSet(item1, item2, item3, item4, item5, item6),
                filter.getItems());
    }

    private static OfflineItem buildItem(@OfflineItemFilter int filter, boolean isSuggested) {
        OfflineItem item = new OfflineItem();
        item.filter = filter;
        item.isSuggested = isSuggested;
        return item;
    }
}