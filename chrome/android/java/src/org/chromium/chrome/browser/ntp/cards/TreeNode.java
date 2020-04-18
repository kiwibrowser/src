// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.ntp.cards;

import org.chromium.base.Callback;

import java.util.Set;

/**
 * A tree interface to allow the New Tab Page RecyclerView to delegate to other components.
 */
public interface TreeNode {
    /**
     * Sets the parent of this node. This method should be called at most once. Before the parent
     * has been set, the node will not send any notifications about changes to its subtree.
     * @param parent the parent of this node.
     */
    void setParent(NodeParent parent);

    /**
     * Detaches the node from the parent so that changes in the node are no longer notified to the
     * parent. This is needed when the parent removes this node from its children.
     */
    void detach();

    /**
     * Returns the number of items under this subtree. This method may be called
     * before initialization.
     *
     * @return The number of items under this subtree.
     * @see android.support.v7.widget.RecyclerView.Adapter#getItemCount()
     */
    int getItemCount();

    /**
     * @param position The position to query
     * @return The view type of the item at {@code position} under this subtree.
     * @see android.support.v7.widget.RecyclerView.Adapter#getItemViewType
     */
    @ItemViewType
    int getItemViewType(int position);

    /**
     * Display the data at {@code position} under this subtree.
     * @param holder The view holder that should be updated.
     * @param position The position of the item under this subtree.
     * @see android.support.v7.widget.RecyclerView.Adapter#onBindViewHolder
     */
    void onBindViewHolder(NewTabPageViewHolder holder, int position);

    /**
     * @param position the position of an item to be dismissed.
     * @return the set of item positions that should be dismissed simultaneously when dismissing the
     *         item at the given {@code position} (including the position itself), or an empty set
     *         if the item can't be dismissed.
     * @see NewTabPageAdapter#getItemDismissalGroup
     */
    Set<Integer> getItemDismissalGroup(int position);

    /**
     * Dismiss the item at the given {@code position}.
     * @param position The position of the item to be dismissed.
     * @param itemRemovedCallback Should be called with the title of the dismissed item, to announce
     * it for accessibility purposes.
     * @see NewTabPageAdapter#dismissItem
     */
    void dismissItem(int position, Callback<String> itemRemovedCallback);

    /**
     * Iterates over all items under this subtree and visits them with the given
     * {@link NodeVisitor}.
     * @param visitor The {@link NodeVisitor} with which to visit all items under this subtree.
     */
    void visitItems(NodeVisitor visitor);
}
