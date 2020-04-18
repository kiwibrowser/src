// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.autofill.keyboard_accessory;

import android.support.v7.widget.RecyclerView;

import org.chromium.chrome.browser.modelutil.RecyclerViewAdapter;

class PasswordAccessorySheetViewAdapter<VH extends RecyclerView.ViewHolder>
        extends RecyclerViewAdapter<PasswordAccessorySheetModel, VH> {
    /**
     * Construct a new {@link RecyclerViewAdapter}.
     *
     * @param model      The {@link PasswordAccessorySheetModel} model used to retrieve items to
     *                   display in the {@link RecyclerView}.
     * @param viewBinder The {@link ViewBinder} binding this adapter to the view holder.
     */
    public PasswordAccessorySheetViewAdapter(PasswordAccessorySheetModel model,
            ViewBinder<PasswordAccessorySheetModel, VH> viewBinder) {
        super(model, viewBinder);
    }

    @Override
    public int getItemViewType(int position) {
        return mModel.getItem(position).getType();
    }
}