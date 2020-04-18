/*
 * Copyright (C) 2013 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.example.bitmapsample;

import android.content.Context;
import android.graphics.Canvas;
import android.util.AttributeSet;
import android.widget.ListView;

import com.android.bitmap.drawable.ExtendedBitmapDrawable;
import com.android.bitmap.view.BitmapDrawableImageView;

public class BitmapView extends BitmapDrawableImageView {
    private final float mDensity;

    private ListView mListView;

    public BitmapView(Context c) {
        this(c, null);
    }

    public BitmapView(Context c, AttributeSet attrs) {
        super(c, attrs);
        mDensity = getResources().getDisplayMetrics().density;
    }

    @Override
    protected int getSuggestedMinimumHeight() {
        return (int) (100 * mDensity);
    }

    @Override
    protected void onSizeChanged(final int w, final int h, int oldw, int oldh) {
        ExtendedBitmapDrawable drawable = getTypedDrawable();
        drawable.setDecodeDimensions(w, h);
    }

    public void setListView(final ListView listView) {
        mListView = listView;
    }

    @Override
    protected void onDraw(final Canvas canvas) {
        ExtendedBitmapDrawable drawable = getTypedDrawable();
        float fraction = (float) getBottom() / (mListView.getHeight() + getHeight());
        drawable.setParallaxFraction(fraction);

        super.onDraw(canvas);
    }
}