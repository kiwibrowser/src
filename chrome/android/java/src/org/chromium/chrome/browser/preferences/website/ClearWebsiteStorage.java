// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.preferences.website;

import android.content.Context;
import android.content.DialogInterface;
import android.preference.DialogPreference;
import android.util.AttributeSet;

import android.view.View;
import org.chromium.base.ContextUtils;
import android.graphics.Color;
import android.graphics.drawable.ColorDrawable;
import android.widget.TextView;

/**
 * Dialog that prompts the user to clear website storage on the device.
 */
public class ClearWebsiteStorage extends DialogPreference {

    private DialogInterface.OnClickListener mConfirmationListener;

    public ClearWebsiteStorage(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
    }

    public ClearWebsiteStorage(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    public void setConfirmationListener(DialogInterface.OnClickListener listener) {
        mConfirmationListener = listener;
    }

    @Override
    protected void onDialogClosed(boolean positiveResult) {
        if (positiveResult && mConfirmationListener != null) {
            mConfirmationListener.onClick(null, 0);
        }
    }

    @Override
    protected void onBindView(View view) {
        super.onBindView(view);
        if (ContextUtils.getAppSharedPreferences().getBoolean("user_night_mode_enabled", false) || ContextUtils.getAppSharedPreferences().getString("active_theme", "").equals("Diamond Black")) {
            if (((TextView) view.findViewById(android.R.id.title)) != null)
               ((TextView) view.findViewById(android.R.id.title)).setTextColor(Color.WHITE);
            if (((TextView) view.findViewById(android.R.id.summary)) != null)
               ((TextView) view.findViewById(android.R.id.summary)).setTextColor(Color.GRAY);
        }
    }
}
