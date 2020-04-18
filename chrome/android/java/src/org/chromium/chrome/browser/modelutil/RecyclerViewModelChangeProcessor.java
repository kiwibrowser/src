// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.modelutil;

import android.support.v7.widget.RecyclerView;
import android.support.v7.widget.RecyclerView.ViewHolder;

import org.chromium.chrome.browser.modelutil.ListObservable.ListObserver;

/**
 * A model change processor for use with a {@link RecyclerView}. The
 * {@link RecyclerViewModelChangeProcessor} should be registered as an observer of a
 * {@link ListObservable} model. Notifies the associated {@link RecyclerViewAdapter<VH>} of
 * changes to the model.
 *
 * @param <E> The type of the {@link ListObservable} model.
 * @param <VH> The {@link ViewHolder} type for the RecyclerView.
 */
public class RecyclerViewModelChangeProcessor<E extends ListObservable, VH extends ViewHolder>
        implements ListObserver {
    private RecyclerViewAdapter<E, VH> mAdapter;

    /**
     * Constructs a new {@link RecyclerViewModelChangeProcessor}.
     * @param adapter The {@link RecyclerViewAdapter<VH>} to be notified of changes to a
     *                {@link ListObservable} model.
     */
    public RecyclerViewModelChangeProcessor(RecyclerViewAdapter<E, VH> adapter) {
        mAdapter = adapter;
    }

    @Override
    public void onItemRangeInserted(ListObservable source, int index, int count) {
        mAdapter.notifyItemRangeInserted(index, count);
    }

    @Override
    public void onItemRangeRemoved(ListObservable source, int index, int count) {
        mAdapter.notifyItemRangeRemoved(index, count);
    }

    @Override
    public void onItemRangeChanged(ListObservable source, int index, int count, Object payload) {
        mAdapter.notifyItemRangeChanged(index, count, payload);
    }
}
