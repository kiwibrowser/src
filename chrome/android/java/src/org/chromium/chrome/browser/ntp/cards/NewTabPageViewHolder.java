// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.ntp.cards;

import android.support.annotation.CallSuper;
import android.support.v7.widget.RecyclerView;
import android.view.View;

import org.chromium.base.Callback;
import org.chromium.chrome.browser.metrics.ImpressionTracker;

/**
 * Holds metadata about an item we want to display on the NTP. An item can be anything that will be
 * displayed on the NTP {@link RecyclerView}.
 */
public class NewTabPageViewHolder extends RecyclerView.ViewHolder {
    private final ImpressionTracker mImpressionTracker;

    /**
     * Constructs a {@link NewTabPageViewHolder} used to display an part of the NTP (e.g., header,
     * article snippet, above-the-fold view, etc.)
     *
     * @param itemView The {@link View} for this item
     */
    public NewTabPageViewHolder(View itemView) {
        super(itemView);
        mImpressionTracker = new ImpressionTracker(itemView);
    }

    /**
     * Whether this item can be swiped and dismissed. The default implementation disallows it.
     * @return {@code true} if the item can be swiped and dismissed, {@code false} otherwise.
     */
    public boolean isDismissable() {
        return false;
    }

    /**
     * Update the layout params for the view holder.
     */
    public void updateLayoutParams() {
    }

    /**
     * Called when the view holder is recycled, to release unused resources.
     * @see NewTabPageAdapter#onViewRecycled(NewTabPageViewHolder)
     */
    @CallSuper
    public void recycle() {
        mImpressionTracker.setListener(null);
    }

    protected RecyclerView.LayoutParams getParams() {
        return (RecyclerView.LayoutParams) itemView.getLayoutParams();
    }

    protected void setImpressionListener(ImpressionTracker.Listener listener) {
        mImpressionTracker.setListener(listener);
    }

    /**
     * A callback to perform a partial bind on a {@link NewTabPageViewHolder}.
     *
     * This interface is used to strengthen type assertions, as those would be less useful with a
     * generic class due to type erasure.
     *
     * @see InnerNode#notifyItemChanged(int, PartialBindCallback)
     */
    public interface PartialBindCallback extends Callback<NewTabPageViewHolder> {}
}
