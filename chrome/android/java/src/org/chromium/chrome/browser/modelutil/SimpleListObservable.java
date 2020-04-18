// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
package org.chromium.chrome.browser.modelutil;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

/**
 * A {@link ListObservable} containing an {@link ArrayList} of Tabs or Actions.
 * It allows models to compose different ListObservables.
 * @param <T> The object type that this class manages in a list.
 */
public class SimpleListObservable<T> extends ListObservable {
    private final List<T> mItems = new ArrayList<>();

    /**
     * Returns the item at the given position.
     * @param index The position to get the item from.
     * @return Returns the found item.
     */
    public T get(int index) {
        return mItems.get(index);
    }

    @Override
    public int getItemCount() {
        return mItems.size();
    }

    /**
     * Appends a given item to the last position of the held {@link ArrayList}. Notifies observers
     * about the inserted item.
     * @param item The item to be stored.
     */
    public void add(T item) {
        mItems.add(item);
        notifyItemRangeInserted(mItems.size() - 1, 1);
    }

    /**
     * Removes a given item from the held {@link ArrayList}. Notifies observers about the removal.
     * @param item The item to be removed.
     */
    public void remove(T item) {
        int position = mItems.indexOf(item);
        mItems.remove(position);
        notifyItemRangeRemoved(position, 1);
    }

    /**
     * Replaces all held items with the given array of items.
     * If the held list was empty, notify observers about inserted elements.
     * If the held list isn't empty but the new list is, notify observer about the removal.
     * If the new and old list aren't empty, notify observer about the complete exchange.
     * @param newItems The set of items that should replace all held items.
     */
    public void set(T[] newItems) {
        if (mItems.isEmpty()) {
            if (newItems.length == 0) {
                return; // Nothing to do, nothing changes.
            }
            mItems.addAll(Arrays.asList(newItems));
            notifyItemRangeInserted(0, mItems.size());
            return;
        }
        int oldSize = mItems.size();
        mItems.clear();
        if (newItems.length == 0) {
            notifyItemRangeRemoved(0, oldSize);
            return;
        }
        mItems.addAll(Arrays.asList(newItems));
        notifyItemRangeChanged(0, Math.max(oldSize, mItems.size()), this);
    }
}