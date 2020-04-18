// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.ntp.cards;

import org.chromium.base.Callback;

import java.util.Collections;
import java.util.Set;

/**
 * A permanent leaf in the tree, i.e. a single item.
 * If the leaf is not to be a permanent member of the tree, see {@link OptionalLeaf} for an
 * implementation that will take care of hiding or showing the item.
 */
public abstract class Leaf extends ChildNode {
    protected Leaf() {
        // Initialize the item count to 1 (at this point the parent is null, so no notification will
        // be sent).
        notifyItemInserted(0);
    }

    @Override
    protected final int getItemCountForDebugging() {
        return 1;
    }

    @Override
    @ItemViewType
    public int getItemViewType(int position) {
        if (position != 0) throw new IndexOutOfBoundsException();
        return getItemViewType();
    }

    @Override
    public void onBindViewHolder(NewTabPageViewHolder holder, int position) {
        if (position != 0) throw new IndexOutOfBoundsException();
        onBindViewHolder(holder);
    }

    @Override
    public Set<Integer> getItemDismissalGroup(int position) {
        return Collections.emptySet();
    }

    @Override
    public void dismissItem(int position, Callback<String> itemRemovedCallback) {
        assert false;
    }

    /**
     * Display the data for this item.
     * @param holder The view holder that should be updated.
     * @see #onBindViewHolder(NewTabPageViewHolder, int)
     * @see android.support.v7.widget.RecyclerView.Adapter#onBindViewHolder
     */
    protected abstract void onBindViewHolder(NewTabPageViewHolder holder);

    /**
     * @return The view type of this item.
     * @see android.support.v7.widget.RecyclerView.Adapter#getItemViewType
     */
    @ItemViewType
    protected abstract int getItemViewType();
}
