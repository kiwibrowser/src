/*
 * Copyright (C) 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIN'D, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.setupwizardlib.util;

import android.content.Context;
import android.content.res.TypedArray;
import android.graphics.drawable.Drawable;
import android.os.Bundle;
import android.support.v7.widget.LinearLayoutManager;
import android.support.v7.widget.RecyclerView;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;

import com.android.setupwizardlib.R;
import com.android.setupwizardlib.view.HeaderRecyclerView;

/**
 * A helper delegate to integrate GLIF theme with PreferenceFragment v14. To use this, create an
 * instance and delegate {@code PreferenceFragment#onCreateRecyclerView} to it. Then call
 * {@code PreferenceFragment#setDivider} to {@link #getDividerDrawable(android.content.Context)} in
 * order to make sure the correct inset is applied to the dividers.
 *
 * @deprecated Use {@link com.android.setupwizardlib.GlifPreferenceLayout}
 */
@Deprecated
public class GlifPreferenceDelegate {

    public static final int[] ATTRS_LIST_DIVIDER = new int[]{ android.R.attr.listDivider };

    private HeaderRecyclerView mRecyclerView;
    private boolean mHasIcons;

    public GlifPreferenceDelegate(boolean hasIcons) {
        mHasIcons = hasIcons;
    }

    public RecyclerView onCreateRecyclerView(LayoutInflater inflater, ViewGroup parent,
            Bundle savedInstanceState) {
        final Context inflaterContext = inflater.getContext();
        mRecyclerView = new HeaderRecyclerView(inflaterContext);
        mRecyclerView.setLayoutManager(new LinearLayoutManager(inflaterContext));
        final View header = inflater.inflate(R.layout.suw_glif_header, mRecyclerView, false);
        mRecyclerView.setHeader(header);
        return mRecyclerView;
    }

    public Drawable getDividerDrawable(Context context) {
        final TypedArray a = context.obtainStyledAttributes(ATTRS_LIST_DIVIDER);
        final Drawable defaultDivider = a.getDrawable(0);
        a.recycle();

        final int dividerInset = context.getResources().getDimensionPixelSize(
                mHasIcons ? R.dimen.suw_items_glif_icon_divider_inset
                        : R.dimen.suw_items_glif_text_divider_inset);
        return DrawableLayoutDirectionHelper.createRelativeInsetDrawable(defaultDivider,
                dividerInset /* start */, 0 /* top */, 0 /* end */, 0 /* bottom */,
                context);
    }

    public void setHeaderText(CharSequence text) {
        final View header = mRecyclerView.getHeader();

        final View titleView = header.findViewById(R.id.suw_layout_title);
        if (titleView instanceof TextView)  {
            ((TextView) titleView).setText(text);
        }
    }

    public void setIcon(Drawable icon) {
        final View header = mRecyclerView.getHeader();

        final View iconView = header.findViewById(R.id.suw_layout_icon);
        if (iconView instanceof ImageView) {
            ((ImageView) iconView).setImageDrawable(icon);
        }
    }
}
