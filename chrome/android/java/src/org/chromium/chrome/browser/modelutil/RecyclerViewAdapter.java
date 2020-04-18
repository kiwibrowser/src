// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.modelutil;

import android.support.v7.widget.RecyclerView;
import android.support.v7.widget.RecyclerView.ViewHolder;
import android.view.ViewGroup;

/**
 * An adapter that uses a {@link ViewBinder} to bind items in a {@link ListObservable} model to
 * {@link ViewHolder}s.
 *
 * @param <E> The type of the {@link ListObservable} model.
 * @param <VH> The {@link ViewHolder} type for the {@link RecyclerView}.
 */
public class RecyclerViewAdapter<E extends ListObservable, VH extends ViewHolder>
        extends RecyclerView.Adapter<VH> {
    /**
     * A view binder used to bind items in the {@link ListObservable} model to {@link ViewHolder}s.
     *
     * @param <E> The type of the {@link ListObservable} model.
     * @param <VH> The {@link ViewHolder} type for the {@link RecyclerView}.
     */
    public interface ViewBinder<E, VH> {
        /**
         * Called when the {@link RecyclerView} needs a new {@link ViewHolder} of the given
         * {@code viewType} to represent an item.
         *
         * @param parent The {@link ViewGroup} into which the new {@link View} will be added after
         *               it's bound to an adapter position.
         * @param viewType The view type of the new {@link View}.
         * @return A new {@link ViewHolder} that holds a {@link View} of the given view type.
         */
        VH onCreateViewHolder(ViewGroup parent, int viewType);

        /**
         * Called to display the item at the specified {@code position} in the provided
         * {@code holder}.
         *
         * @param model The {@link ListObservable} model used to retrieve the item at
         *              {@code position}.
         * @param holder The {@link ViewHolder} which should be updated to represent {@code item}.
         * @param position The position of the item to be bound.
         */
        void onBindViewHolder(E model, VH holder, int position);
    }

    protected final E mModel;

    private ViewBinder<E, VH> mViewBinder;

    /**
     * Construct a new {@link RecyclerViewAdapter}.
     * @param model The {@link ListObservable} model used to retrieve items to display in the
     *              {@link RecyclerView}.
     * @param viewBinder The {@link ViewBinder} binding this adapter to the view holder.
     */
    public RecyclerViewAdapter(E model, RecyclerViewAdapter.ViewBinder<E, VH> viewBinder) {
        mModel = model;
        mViewBinder = viewBinder;
    }

    /**
     * Set the {@link ViewBinder} to use with this adapter.
     *
     * @param viewBinder The {@link ViewBinder} used to bind items in the {@link ListObservable}
     *                   model to {@link ViewHolder}s.
     */
    public void setViewBinder(ViewBinder<E, VH> viewBinder) {
        mViewBinder = viewBinder;
    }

    @Override
    public int getItemCount() {
        return mModel.getItemCount();
    }

    @Override
    public VH onCreateViewHolder(ViewGroup parent, int viewType) {
        return mViewBinder.onCreateViewHolder(parent, viewType);
    }

    @Override
    public void onBindViewHolder(VH holder, int position) {
        mViewBinder.onBindViewHolder(mModel, holder, position);
    }
}
