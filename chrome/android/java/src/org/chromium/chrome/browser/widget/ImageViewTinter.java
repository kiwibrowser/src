// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.widget;

import android.content.res.ColorStateList;
import android.content.res.TypedArray;
import android.graphics.Canvas;
import android.graphics.PorterDuff;
import android.support.annotation.Nullable;
import android.util.AttributeSet;
import android.widget.ImageView;

import org.chromium.chrome.R;

/**
 * Utility for tinting an ImageView and its Drawable.
 *
 * Example usage in XML:
 * <ImageViewTinterInstanceOwner
 *     xmlns:android="http://schemas.android.com/apk/res/android"
 *     xmlns:chrome="http://schemas.android.com/apk/res-auto"
 *     chrome:chrometint="@color/light_active_color" />
 *
 * The default style used by the Application will likely cause your Drawable to be automatically
 * tinted.  To prevent this, set the value of chrome:chrometint to "@null".
 */
public class ImageViewTinter {
    /** Classes that own an ImageViewTinter must implement these functions. */
    public static interface ImageViewTinterOwner {
        /** See {@link ImageViewTinter#drawableStateChanged}. */
        void drawableStateChanged();

        /** See {@link ImageViewTinter#setTint}. */
        void setTint(ColorStateList tintList);

        /** See {@link ImageView#onDraw}. */
        void onDraw(Canvas canvas);
    }

    private ImageView mImageView;
    private ColorStateList mTintList;

    /**
     * Constructor.  Should be called with the AttributeSet and style of the ImageView so that XML
     * attributes for it can be parsed.
     * @param view     ImageView being tinted.
     * @param attrs    AttributeSet that is pulled in from an XML layout.  May be null.
     * @param defStyle Style that is pulled in from an XML layout.
     */
    public ImageViewTinter(ImageViewTinterOwner view, @Nullable AttributeSet attrs, int defStyle) {
        mImageView = (ImageView) view;

        // Parse out the attributes from the XML.
        if (attrs != null) {
            TypedArray a = mImageView.getContext().obtainStyledAttributes(
                    attrs, R.styleable.TintedImage, defStyle, 0);
            setTint(a.getColorStateList(R.styleable.TintedImage_chrometint));
            a.recycle();
        }
    }

    /**
     * Sets the tint color for the given ImageView for all states.
     * @param tintList The set of colors to use.
     */
    public void setTint(ColorStateList tintList) {
        if (mTintList == tintList) return;
        mTintList = tintList;
        updateTintColor();
    }

    /** Call when the state of the Drawable has changed. */
    public void drawableStateChanged() {
        updateTintColor();
    }

    private void updateTintColor() {
        if (mImageView.getDrawable() == null) {
            return;
        } else if (mTintList == null) {
            mImageView.clearColorFilter();
            return;
        }

        int tintColor = mTintList.getColorForState(mImageView.getDrawableState(), 0);
        mImageView.setColorFilter(tintColor, PorterDuff.Mode.SRC_IN);
    }
}
