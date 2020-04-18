// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.download.home.filter.chips;

import android.support.v7.widget.RecyclerView.ViewHolder;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.modelutil.RecyclerViewAdapter.ViewBinder;

/** Responsible for binding the {@link ChipsModel} to {@link ViewHolder}s in the RecyclerView. */
class ChipsViewBinder implements ViewBinder<ChipsModel, ChipsViewBinder.ChipsViewHolder> {
    /** The {@link ViewHolder} responsible for reflecting a {@link Chip} to a {@link ChipView}. */
    public static class ChipsViewHolder extends ViewHolder {
        /**
         * Builds a ChipsViewHolder around a specific {@link ChipView}.
         * @param itemView
         */
        public ChipsViewHolder(View itemView) {
            super(itemView);

            assert itemView instanceof ChipView;
        }

        /**
         * Pushes the properties of {@code chip} to {@code itemView}.
         * @param chip The {@link Chip} to visually reflect in the stored {@link ChipView}.
         */
        public void bind(Chip chip) {
            ChipView view = (ChipView) itemView;

            view.setEnabled(chip.enabled);
            view.setSelected(chip.selected);
            view.setOnClickListener(v -> chip.chipSelectedListener.run());

            view.setChipIcon(chip.icon);
            view.setText(chip.text);
            view.setContentDescription(view.getContext().getText(chip.contentDescription));
        }
    }

    // ViewBinder implementation.
    @Override
    public ChipsViewHolder onCreateViewHolder(ViewGroup parent, int viewType) {
        return new ChipsViewHolder(
                LayoutInflater.from(parent.getContext()).inflate(R.layout.chip, null));
    }

    @Override
    public void onBindViewHolder(ChipsModel model, ChipsViewHolder holder, int position) {
        holder.bind(model.getItemAt(position));
    }
}