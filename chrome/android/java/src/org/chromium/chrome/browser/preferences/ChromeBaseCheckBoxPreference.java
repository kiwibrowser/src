// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.preferences;

import android.content.Context;
import android.preference.CheckBoxPreference;
import android.util.AttributeSet;
import android.view.View;
import android.widget.CheckBox;
import android.widget.TextView;

import android.support.v4.widget.CompoundButtonCompat;
import android.content.res.ColorStateList;
import org.chromium.base.ContextUtils;
import android.graphics.Color;

/**
 * Contains the basic functionality that should be shared by all CheckBoxPreference in Chrome.
 */
public class ChromeBaseCheckBoxPreference extends CheckBoxPreference {

    private ManagedPreferenceDelegate mManagedPrefDelegate;

    /**
     * Constructor for inflating from XML.
     */
    public ChromeBaseCheckBoxPreference(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    /**
     * Sets the ManagedPreferenceDelegate which will determine whether this preference is managed.
     */
    public void setManagedPreferenceDelegate(ManagedPreferenceDelegate delegate) {
        mManagedPrefDelegate = delegate;
        ManagedPreferencesUtils.initPreference(mManagedPrefDelegate, this);
    }

    @Override
    protected void onBindView(View view) {
        super.onBindView(view);
        ((TextView) view.findViewById(android.R.id.title)).setSingleLine(false);
        ManagedPreferencesUtils.onBindViewToPreference(mManagedPrefDelegate, this, view);
        if (ContextUtils.getAppSharedPreferences().getBoolean("user_night_mode_enabled", false) || ContextUtils.getAppSharedPreferences().getString("active_theme", "").equals("Diamond Black")) {
            ((TextView) view.findViewById(android.R.id.title)).setTextColor(Color.WHITE);
            if (((TextView) view.findViewById(android.R.id.summary)) != null)
              ((TextView) view.findViewById(android.R.id.summary)).setTextColor(Color.GRAY);
            if (((CheckBox) view.findViewById(android.R.id.checkbox)) != null) {
              ColorStateList colorStateList = new ColorStateList(
                      new int[][]{
                              new int[]{-android.R.attr.state_checked}, // unchecked
                              new int[]{android.R.attr.state_checked} , // checked
                      },
                      new int[]{
                              Color.parseColor("#555555"),  //unchecked color
                              Color.parseColor("#FFFFFF"),  //checked color
                      }
              );
              CompoundButtonCompat.setButtonTintList((CheckBox) view.findViewById(android.R.id.checkbox), colorStateList);
            }
        }
    }

    @Override
    protected void onClick() {
        if (ManagedPreferencesUtils.onClickPreference(mManagedPrefDelegate, this)) return;
        super.onClick();
    }
}
