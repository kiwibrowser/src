// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.preferences;

import android.content.Context;
import android.preference.CheckBoxPreference;
import android.util.AttributeSet;
import android.view.View;
import android.widget.Checkable;

import android.view.View;
import android.widget.CheckBox;
import android.widget.TextView;
import android.support.v4.widget.CompoundButtonCompat;
import android.content.res.ColorStateList;
import org.chromium.base.ContextUtils;
import android.graphics.Color;
import android.graphics.drawable.ColorDrawable;
import android.widget.ListView;

/**
 * A CheckBoxPreference which can be checked via a SeekBarPreference. A normal CheckBoxPreference
 * cannot be used in this way as calling setChecked(...) calls notifyChanged() which causes the
 * drag on the seek bar to be cancelled.
 */
public class SeekBarLinkedCheckBoxPreference extends CheckBoxPreference {
    private Checkable mCheckable;
    private SeekBarPreference mLinkedSeekBarPreference;

    public SeekBarLinkedCheckBoxPreference(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    @Override
    protected void onBindView(View view) {
        super.onBindView(view);
        mCheckable = (Checkable) view.findViewById(android.R.id.checkbox);
        mCheckable.setChecked(isChecked());
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
    public void setChecked(boolean checked) {
        super.setChecked(checked);
        if (mCheckable != null) mCheckable.setChecked(checked);
    }

    public void setLinkedSeekBarPreference(SeekBarPreference linkedSeekBarPreference) {
        mLinkedSeekBarPreference = linkedSeekBarPreference;
    }

    @Override
    protected void notifyChanged() {
        if (mLinkedSeekBarPreference == null || !mLinkedSeekBarPreference.isTrackingTouch()) {
            super.notifyChanged();
        }
    }
}
