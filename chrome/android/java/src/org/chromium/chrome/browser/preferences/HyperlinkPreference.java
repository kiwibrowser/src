// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.preferences;

import android.content.Context;
import android.content.res.TypedArray;
import android.preference.Preference;
import android.util.AttributeSet;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.TextView;

import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.customtabs.CustomTabActivity;
import org.chromium.ui.base.LocalizationUtils;

import android.view.View;
import org.chromium.base.ContextUtils;
import android.graphics.Color;
import android.graphics.drawable.ColorDrawable;

/**
 * A preference that navigates to an URL.
 */
public class HyperlinkPreference extends Preference {

    private final int mTitleResId;
    private final int mUrlResId;
    private final int mColor;
    private final boolean mImitateWebLink;

    public HyperlinkPreference(Context context, AttributeSet attrs) {
        super(context, attrs);
        TypedArray a = context.obtainStyledAttributes(attrs,
                R.styleable.HyperlinkPreference, 0, 0);
        mUrlResId = a.getResourceId(R.styleable.HyperlinkPreference_url, 0);
        mImitateWebLink = a.getBoolean(R.styleable.HyperlinkPreference_imitateWebLink, false);
        a.recycle();
        mTitleResId = getTitleRes();
        mColor = ApiCompatibilityUtils.getColor(context.getResources(), R.color.google_blue_700);
    }

    @Override
    protected void onClick() {
        CustomTabActivity.showInfoPage(getContext(),
                LocalizationUtils.substituteLocalePlaceholder(getContext().getString(mUrlResId)));
    }

    @Override
    protected void onBindView(View view) {
        super.onBindView(view);
        TextView titleView = (TextView) view.findViewById(android.R.id.title);
        titleView.setSingleLine(false);

        if (ContextUtils.getAppSharedPreferences().getBoolean("user_night_mode_enabled", false) || ContextUtils.getAppSharedPreferences().getString("active_theme", "").equals("Diamond Black")) {
            ((TextView) view.findViewById(android.R.id.title)).setTextColor(Color.WHITE);
            if (((TextView) view.findViewById(android.R.id.summary)) != null)
              ((TextView) view.findViewById(android.R.id.summary)).setTextColor(Color.GRAY);
        }
        if (mImitateWebLink) {
            setSelectable(false);

            titleView.setClickable(true);
            titleView.setTextColor(mColor);
            titleView.setOnClickListener(new OnClickListener() {
                @Override
                public void onClick(View v) {
                    HyperlinkPreference.this.onClick();
                }
            });
        }
    }
}
