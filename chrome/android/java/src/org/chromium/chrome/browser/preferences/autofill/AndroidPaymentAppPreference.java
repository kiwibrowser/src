// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.preferences.autofill;

import android.content.Context;
import android.preference.Preference;
import android.view.View;
import android.view.ViewGroup;

import org.chromium.chrome.R;
import org.chromium.ui.HorizontalListDividerDrawable;

/** Preference with fixed icon size for Android payment apps. */
public class AndroidPaymentAppPreference extends Preference {
    private boolean mDrawDivider;

    public AndroidPaymentAppPreference(Context context) {
        super(context, null);
    }

    @Override
    protected View onCreateView(ViewGroup parent) {
        View view = super.onCreateView(parent);

        int iconSize =
                getContext().getResources().getDimensionPixelSize(R.dimen.payments_favicon_size);
        View iconView = view.findViewById(android.R.id.icon);
        ViewGroup.LayoutParams layoutParams = iconView.getLayoutParams();
        layoutParams.width = iconSize;
        layoutParams.height = iconSize;
        iconView.setLayoutParams(layoutParams);

        return view;
    }

    @Override
    protected void onBindView(View view) {
        super.onBindView(view);

        if (mDrawDivider) {
            int left = view.getPaddingLeft();
            int right = view.getPaddingRight();
            int top = view.getPaddingTop();
            int bottom = view.getPaddingBottom();
            view.setBackground(HorizontalListDividerDrawable.create(getContext()));
            view.setPadding(left, top, right, bottom);
        }
    }

    /**
     * Sets whether a horizontal divider line should be drawn at the bottom of this preference.
     */
    public void setDrawDivider(boolean drawDivider) {
        if (mDrawDivider != drawDivider) {
            mDrawDivider = drawDivider;
            notifyChanged();
        }
    }
}