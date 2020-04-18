// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.autofill.keyboard_accessory;

import android.support.v7.widget.LinearLayoutManager;
import android.support.v7.widget.RecyclerView;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.autofill.keyboard_accessory.PasswordAccessorySheetModel.Item;
import org.chromium.chrome.browser.modelutil.RecyclerViewAdapter;

/**
 * This stateless class provides methods to bind the items in a {@link PasswordAccessorySheetModel}
 * to the {@link RecyclerView} used as view of the Password accessory sheet component.
 */
class PasswordAccessorySheetViewBinder
        implements RecyclerViewAdapter.ViewBinder<PasswordAccessorySheetModel,
                PasswordAccessorySheetViewBinder.TextViewHolder> {
    /**
     * Holds a TextView that represents a list entry.
     */
    static class TextViewHolder extends RecyclerView.ViewHolder {
        TextViewHolder(View itemView) {
            super(itemView);
        }

        TextView getView() {
            return (TextView) itemView;
        }
    }

    @Override
    public TextViewHolder onCreateViewHolder(ViewGroup parent, @Item.Type int viewType) {
        if (viewType == Item.TYPE_LABEL) {
            return new TextViewHolder(
                    LayoutInflater.from(parent.getContext())
                            .inflate(R.layout.password_accessory_sheet_label, parent, false));
        }
        if (viewType == Item.TYPE_SUGGESTIONS) {
            return new TextViewHolder(
                    LayoutInflater.from(parent.getContext())
                            .inflate(R.layout.password_accessory_sheet_suggestion, parent, false));
        }
        assert false : "Every Item.Type needs to be assigned a view!";
        return null;
    }

    @Override
    public void onBindViewHolder(
            PasswordAccessorySheetModel model, TextViewHolder holder, int position) {
        KeyboardAccessoryData.Action item = model.getItem(position);
        holder.getView().setText(item.getCaption());
        if (item.getDelegate() != null) {
            holder.getView().setOnClickListener(src -> item.getDelegate().onActionTriggered(item));
        }
    }

    void initializeView(RecyclerView view, RecyclerViewAdapter adapter) {
        view.setLayoutManager(
                new LinearLayoutManager(view.getContext(), LinearLayoutManager.VERTICAL, false));
        view.setItemAnimator(null);
        view.setAdapter(adapter);
    }
}