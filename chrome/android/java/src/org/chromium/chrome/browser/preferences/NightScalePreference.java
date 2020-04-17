// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.preferences;

import android.content.Context;
import android.util.AttributeSet;
import android.util.TypedValue;
import android.view.View;
import android.widget.TextView;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.accessibility.NightModePrefs;
import org.chromium.chrome.browser.accessibility.NightModePrefs.NightModePrefsObserver;

import android.view.View;
import org.chromium.base.ContextUtils;
import android.graphics.Color;
import android.graphics.drawable.ColorDrawable;
import android.widget.ListView;

/**
 * Preference that allows the user to change the scaling factor that's applied to web page text.
 * This also shows a preview of how large a typical web page's text will appear.
 */
public class NightScalePreference extends NightModeSeekBarPreference {
    private TextView mPreview;
    private View mView;
    private final NightModePrefs mNightModePrefs;

    /**
     * Constructor for inflating from XML.
     */
    public NightScalePreference(Context context, AttributeSet attrs) {
        super(context, attrs);

        mNightModePrefs = NightModePrefs.getInstance(getContext());

        setLayoutResource(R.layout.custom_preference);
        setWidgetLayoutResource(R.layout.preference_night_scale);
    }

    @Override
    protected View onCreateView(android.view.ViewGroup parent) {
        if (mView == null) mView = super.onCreateView(parent);
        return mView;
    }

    @Override
    protected void onBindView(View view) {
        super.onBindView(view);
        if (ContextUtils.getAppSharedPreferences().getBoolean("user_night_mode_enabled", false) || ContextUtils.getAppSharedPreferences().getString("active_theme", "").equals("Diamond Black")) {
            ((TextView) view.findViewById(android.R.id.title)).setTextColor(Color.WHITE);
            if (((TextView) view.findViewById(android.R.id.summary)) != null)
              ((TextView) view.findViewById(android.R.id.summary)).setTextColor(Color.GRAY);
            if (((TextView) view.findViewById(R.id.seekbar_amount)) != null)
              ((TextView) view.findViewById(R.id.seekbar_amount)).setTextColor(Color.GRAY);
        }
    }
}
