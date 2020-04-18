// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.autofill.keyboard_accessory;

import android.support.annotation.IntDef;
import android.support.annotation.Nullable;

import org.chromium.chrome.browser.modelutil.ListObservable;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.util.ArrayList;
import java.util.List;

class PasswordAccessorySheetModel extends ListObservable {
    static class Item implements KeyboardAccessoryData.Action {
        @Retention(RetentionPolicy.SOURCE)
        @IntDef({TYPE_LABEL, TYPE_SUGGESTIONS})
        @interface Type {}
        static final int TYPE_LABEL = 1;
        static final int TYPE_SUGGESTIONS = 2;

        private final int mViewType;
        private final @Nullable Delegate mDelegate;
        private final String mCaption;

        Item(@Type int viewType, String caption, @Nullable Delegate delegate) {
            mViewType = viewType;
            mCaption = caption;
            mDelegate = delegate;
        }

        @Type
        int getType() {
            return this.mViewType;
        }

        @Override
        public String getCaption() {
            return this.mCaption;
        }

        @Override
        public Delegate getDelegate() {
            return this.mDelegate;
        }
    }

    private final List<Item> mItems = new ArrayList<>();

    @Override
    public int getItemCount() {
        return mItems.size();
    }

    void addLabel(String caption) {
        mItems.add(new Item(Item.TYPE_LABEL, caption, null));
        notifyItemRangeInserted(mItems.size() - 1, 1);
    }

    void addSuggestion(KeyboardAccessoryData.Action action) {
        mItems.add(new Item(Item.TYPE_SUGGESTIONS, action.getCaption(), action.getDelegate()));
        notifyItemRangeInserted(mItems.size() - 1, 1);
    }

    public Item getItem(int position) {
        return mItems.get(position);
    }
}