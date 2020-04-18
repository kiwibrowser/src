// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.download.home.list;

import org.chromium.base.VisibleForTesting;
import org.chromium.chrome.browser.modelutil.ListObservable;
import org.chromium.components.offline_items_collection.OfflineItem;

import java.util.ArrayList;
import java.util.Calendar;
import java.util.Date;
import java.util.List;

/**
 * A simple {@link ListObservable} that acts as a backing store for the output of
 * {@link DateOrderedList}.  This will be updated in the near future to support large batch updates
 * to minimize the hit on {@link ListObserver}s.
 */
public class DateOrderedListModel extends BatchListObservable {
    /**
     * Represents an item meant to be shown in the UI.  If {@code ListItem#item} is {@code null},
     * this item represents a date separate for a new day's worth of items.  Otherwise it represents
     * an item itself.
     */
    public static class ListItem {
        /** If not {@code null}, the {@link OfflineItem} to show in the UI. */
        public final OfflineItem item;

        /** The date that reflects the {@link ListItem}. */
        public final Date date;

        /** A stable (non-changing) id that represents this {@link ListItem}. */
        public final long stableId;

        /** Builds a {@link ListItem} representing {@code item}.  This will set the {@code date}. */
        ListItem(OfflineItem item) {
            this.stableId = generateStableId(item);
            this.item = item;
            this.date = new Date(item.creationTimeMs);
        }

        /** Builds a {@link ListItem} representing {@code date}.  This will not set {@code item}. */
        ListItem(Calendar calendar) {
            this.stableId = generateStableIdForDayOfYear(calendar);
            this.item = null;
            this.date = calendar.getTime();
        }

        @VisibleForTesting
        static long generateStableId(OfflineItem item) {
            return (((long) item.id.hashCode()) << 32) + (item.creationTimeMs & 0x0FFFFFFFF);
        }

        @VisibleForTesting
        static long generateStableIdForDayOfYear(Calendar calendar) {
            return (calendar.get(Calendar.YEAR) << 16) + calendar.get(Calendar.DAY_OF_YEAR);
        }
    }

    private final List<ListItem> mItems = new ArrayList<>();

    /** Adds {@code item} to this list at {@code index}. */
    public void addItem(int index, ListItem item) {
        mItems.add(index, item);
        notifyItemRangeInserted(index, 1);
    }

    /** Removes the {@link ListItem} at {@code index}. */
    public void removeItem(int index) {
        mItems.remove(index);
        notifyItemRangeRemoved(index, 1);
    }

    /** Sets the {@link ListItem} at {@code index} to {@code item}. */
    public void setItem(int index, ListItem item) {
        mItems.set(index, item);
        notifyItemRangeChanged(index, 1, null);
    }

    /** @return The {@link ListItem} at {@code index}. */
    public ListItem getItemAt(int index) {
        return mItems.get(index);
    }

    // ListObservable implementation.
    @Override
    public int getItemCount() {
        return mItems.size();
    }
}