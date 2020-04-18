// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.download.home.filter.chips;

import android.content.Context;
import android.graphics.Rect;
import android.support.v7.widget.LinearLayoutManager;
import android.support.v7.widget.RecyclerView;
import android.support.v7.widget.RecyclerView.ItemDecoration;
import android.support.v7.widget.RecyclerView.State;
import android.view.View;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.modelutil.RecyclerViewAdapter;
import org.chromium.chrome.browser.modelutil.RecyclerViewAdapter.ViewBinder;
import org.chromium.chrome.browser.modelutil.RecyclerViewModelChangeProcessor;

/**
 * The coordinator responsible for managing a list of chips.  To get the {@link View} that
 * represents this coordinator use {@link #getView()}.
 */
public class ChipsCoordinator implements ChipsProvider.Observer {
    private final ChipsProvider mProvider;
    private final ChipsModel mModel;
    private final RecyclerViewModelChangeProcessor<ChipsModel, ChipsViewBinder.ChipsViewHolder>
            mModelChangeProcessor;
    private final RecyclerView mView;

    /**
     * Builds and initializes this coordinator, including all sub-components.
     * @param context The {@link Context} to use to grab all of the resources.
     * @param provider The source for the underlying Chip state.
     */
    public ChipsCoordinator(Context context, ChipsProvider provider) {
        assert context != null;
        assert provider != null;

        mProvider = provider;

        // Build the underlying components.
        mModel = new ChipsModel();
        mView = createView(context);
        ViewBinder<ChipsModel, ChipsViewBinder.ChipsViewHolder> viewBinder = new ChipsViewBinder();
        RecyclerViewAdapter<ChipsModel, ChipsViewBinder.ChipsViewHolder> adapter =
                new RecyclerViewAdapter<>(mModel, viewBinder);
        mModelChangeProcessor =
                new RecyclerViewModelChangeProcessor<ChipsModel, ChipsViewBinder.ChipsViewHolder>(
                        adapter);

        mView.setAdapter(adapter);
        adapter.setViewBinder(viewBinder);
        mModel.addObserver(mModelChangeProcessor);

        mProvider.addObserver(this);
        mModel.setChips(mProvider.getChips());
    }

    /**
     * Destroys the coordinator.  This should be called when the coordinator is no longer in use.
     * The coordinator should not be used after that point.
     */
    public void destroy() {
        mProvider.removeObserver(this);
    }

    /** @return The {@link View} that represents this coordinator. */
    public View getView() {
        return mView;
    }

    // ChipsProvider.Observer implementation.
    @Override
    public void onChipChanged(int position, Chip chip) {
        mModel.updateChip(position, chip);
        mModel.setChips(mProvider.getChips());
    }

    private static RecyclerView createView(Context context) {
        RecyclerView view = new RecyclerView(context);
        view.setLayoutManager(
                new LinearLayoutManager(context, LinearLayoutManager.HORIZONTAL, false));
        view.addItemDecoration(new SpaceItemDecoration(context));
        view.getItemAnimator().setChangeDuration(0);
        return view;
    }

    private static class SpaceItemDecoration extends ItemDecoration {
        private final int mPaddingPx;

        public SpaceItemDecoration(Context context) {
            mPaddingPx = (int) context.getResources().getDimension(R.dimen.chip_list_padding);
        }

        @Override
        public void getItemOffsets(Rect outRect, View view, RecyclerView parent, State state) {
            int position = parent.getChildAdapterPosition(view);
            boolean isFirst = position == 0;
            boolean isLast = position == parent.getAdapter().getItemCount() - 1;

            outRect.left = isFirst ? 2 * mPaddingPx : mPaddingPx;
            outRect.right = isLast ? 2 * mPaddingPx : mPaddingPx;
        }
    }
}