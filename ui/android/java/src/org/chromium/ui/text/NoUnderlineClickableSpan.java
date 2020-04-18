// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.ui.text;

import android.support.annotation.ColorRes;
import android.text.TextPaint;
import android.text.style.ClickableSpan;
import android.view.View;

import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.base.Callback;
import org.chromium.base.ContextUtils;
import org.chromium.ui.R;

/**
 * Shows a blue clickable link with underlines turned off.
 */
public class NoUnderlineClickableSpan extends ClickableSpan {
    private final int mColor;
    private final Callback<View> mOnClick;

    public NoUnderlineClickableSpan(Callback<View> onClickCallback) {
        this(R.color.google_blue_700, onClickCallback);
    }

    public NoUnderlineClickableSpan(@ColorRes int colorResId, Callback<View> onClickCallback) {
        mColor = ApiCompatibilityUtils.getColor(
                ContextUtils.getApplicationContext().getResources(), colorResId);
        mOnClick = onClickCallback;
    }

    @Override
    public final void onClick(View view) {
        mOnClick.onResult(view);
    }

    // Disable underline on the link text.
    @Override
    public void updateDrawState(TextPaint textPaint) {
        super.updateDrawState(textPaint);
        textPaint.setUnderlineText(false);
        textPaint.setColor(mColor);
    }
}
