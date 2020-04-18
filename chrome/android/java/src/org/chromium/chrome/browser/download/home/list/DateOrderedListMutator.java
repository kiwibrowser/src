// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.download.home.list;

import org.chromium.base.CollectionUtil;
import org.chromium.chrome.browser.download.home.filter.OfflineItemFilterObserver;
import org.chromium.chrome.browser.download.home.filter.OfflineItemFilterSource;
import org.chromium.components.offline_items_collection.ContentId;
import org.chromium.components.offline_items_collection.OfflineItem;

import java.util.ArrayList;
import java.util.Calendar;
import java.util.Collection;
import java.util.Collections;
import java.util.List;

/**
 * A class responsible for turning a {@link Collection} of {@link OfflineItem}s into a list meant
 * to be displayed in the download home UI.  This list has the following properties:
 * - Sorted.
 * - Separated by date headers for each individual day.
 * - Converts changes in the form of {@link Collection}s to delta changes on the list.
 *
 * TODO(dtrainor): This should be optimized in the near future.  There are a few key things that can
 * be changed:
 * - Do a single iterating across each list to merge/unmerge them.  This requires sorting and
 *   tracking the current position across both as iterating (see {@link #onItemsRemoved(Collection)}
 *   for an example since that is close to doing what we want - minus the contains() call).
 */
public class DateOrderedListMutator implements OfflineItemFilterObserver {
    private static final int INVALID_INDEX = -1;

    private final DateOrderedListModel mModel;

    /**
     * Creates an DateOrderedList instance that will reflect {@code source}.
     * @param source The source of data for this list.
     * @param model  The model that will be the storage for the updated list.
     */
    public DateOrderedListMutator(OfflineItemFilterSource source, DateOrderedListModel model) {
        mModel = model;
        source.addObserver(this);
        onItemsAdded(source.getItems());
    }

    // OfflineItemFilterObserver implementation.
    @Override
    public void onItemsAdded(Collection<OfflineItem> items) {
        List<OfflineItem> sorted = new ArrayList<>(items);
        Collections.sort(sorted, (lhs, rhs) -> {
            long delta = rhs.creationTimeMs - lhs.creationTimeMs;
            if (delta > 0) return 1;
            if (delta < 0) return -1;
            return 0;
        });

        for (OfflineItem item : sorted) {
            int index = getBestIndexFor(item);
            mModel.addItem(index, new DateOrderedListModel.ListItem(item));

            boolean isFirst = index == 0;
            boolean isPrevSameDay = !isFirst
                    && CalendarUtils.isSameDay(
                               mModel.getItemAt(index - 1).date.getTime(), item.creationTimeMs);

            if (isFirst || !isPrevSameDay) {
                Calendar startOfDay = CalendarUtils.getStartOfDay(item.creationTimeMs);
                mModel.addItem(index, new DateOrderedListModel.ListItem(startOfDay));
            }
        }

        mModel.dispatchLastEvent();
    }

    @Override
    public void onItemsRemoved(Collection<OfflineItem> items) {
        for (int i = mModel.getItemCount() - 1; i >= 0; i--) {
            DateOrderedListModel.ListItem item = mModel.getItemAt(i);
            boolean isHeader = item.item == null;
            boolean isLast = i == mModel.getItemCount() - 1;
            boolean isNextHeader = isLast ? false : mModel.getItemAt(i + 1).item == null;

            boolean removeHeader = isHeader && (isLast || isNextHeader);
            boolean removeItem = !isHeader && items.contains(item.item);

            if (removeHeader || removeItem) mModel.removeItem(i);
        }

        mModel.dispatchLastEvent();
    }

    @Override
    public void onItemUpdated(OfflineItem oldItem, OfflineItem item) {
        assert oldItem.id.equals(item.id);

        int i = indexOfItem(oldItem.id);
        if (i == INVALID_INDEX) return;

        // If the update changed the creation time, remove and add the element to get it positioned.
        if (oldItem.creationTimeMs != item.creationTimeMs) {
            onItemsRemoved(CollectionUtil.newArrayList(oldItem));
            onItemsAdded(CollectionUtil.newArrayList(item));
        } else {
            mModel.setItem(i, new DateOrderedListModel.ListItem(item));
        }

        mModel.dispatchLastEvent();
    }

    private int indexOfItem(ContentId id) {
        for (int i = 0; i < mModel.getItemCount(); i++) {
            OfflineItem item = mModel.getItemAt(i).item;
            if (item != null && item.id.equals(id)) return i;
        }

        return INVALID_INDEX;
    }

    private int getBestIndexFor(OfflineItem item) {
        for (int i = 0; i < mModel.getItemCount(); i++) {
            long timeStamp = mModel.getItemAt(i).date.getTime();
            if (item.creationTimeMs < timeStamp) return i;
        }

        return mModel.getItemCount();
    }
}