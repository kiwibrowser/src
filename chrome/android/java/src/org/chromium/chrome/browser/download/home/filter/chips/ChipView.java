// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.download.home.filter.chips;

import android.content.Context;
import android.content.res.ColorStateList;
import android.graphics.drawable.Drawable;
import android.support.annotation.DrawableRes;
import android.support.v4.graphics.drawable.DrawableCompat;
import android.support.v7.widget.AppCompatTextView;
import android.util.AttributeSet;

import org.chromium.base.ApiCompatibilityUtils;

/**
 * A {@link AppCompatTextView} that visually represents a {@link Chip} in a chip list.  The way to
 * get a properly set up {@link ChipView} with the proper properties would be to inflate
 * R.layout.chip.
 */
public class ChipView extends AppCompatTextView {
    public ChipView(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    /**
     * Used to effectively set the start compound drawable on the {@link TextView}.  Currently there
     * is no compat method to handle setting the {@link ColorStateList} for compound drawables.
     * @param icon The drawable id that represents the icon to use in the chip.
     */
    public void setChipIcon(@DrawableRes int icon) {
        Drawable drawable =
                DrawableCompat.wrap(ApiCompatibilityUtils.getDrawable(getResources(), icon));

        ColorStateList textColor = getTextColors();
        if (textColor != null) DrawableCompat.setTintList(drawable.mutate(), textColor);
        ApiCompatibilityUtils.setCompoundDrawablesRelativeWithIntrinsicBounds(
                this, drawable, null, null, null);
    }
}