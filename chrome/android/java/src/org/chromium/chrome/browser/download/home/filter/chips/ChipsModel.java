// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.download.home.filter.chips;

import org.chromium.chrome.browser.modelutil.ListObservable;

import java.util.ArrayList;
import java.util.List;

/**
 * A model that holds a list of chips to visually represent in the UI.
 * TODO(dtrainor): Move to SimpleListObservable once that is supported.
 */
class ChipsModel extends ListObservable {
    private final List<Chip> mChips = new ArrayList<>();

    // ListObservable implementation.
    @Override
    public int getItemCount() {
        return mChips.size();
    }

    /**
     * Populates the model with a list of {@link Chip}s.  The list of chips is copied but the
     * internal {@link Chip} objects are not.
     * @param chips The new list of {@link Chip}s to store in the model.
     */
    public void setChips(List<Chip> chips) {
        if (chips.size() == mChips.size()) {
            // Avoid complete list rebuilds for the case where the Chip count is the same.
            mChips.clear();
            mChips.addAll(chips);
            notifyItemRangeChanged(0, mChips.size(), null);
        } else {
            if (!mChips.isEmpty()) {
                int size = mChips.size();
                mChips.clear();
                notifyItemRangeRemoved(0, size);
            }

            if (!chips.isEmpty()) {
                mChips.addAll(chips);
                notifyItemRangeInserted(0, mChips.size());
            }
        }
    }

    /** Updates an individual item in the list of Chips. */
    public void updateChip(int position, Chip chip) {
        assert position >= 0 && position < mChips.size();
        mChips.set(position, chip);
        notifyItemRangeChanged(position, 1, null);
    }

    /**
     * @param position The position of the {@link Chip} in the list of chips to return.
     * @return The {@link Chip} stored at {@code position} in the list of chips.
     */
    public Chip getItemAt(int position) {
        assert position >= 0 && position < mChips.size();
        return mChips.get(position);
    }
}