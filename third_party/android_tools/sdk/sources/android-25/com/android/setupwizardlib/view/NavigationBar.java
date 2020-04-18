/*
 * Copyright (C) 2015 The Android Open Source Project
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

package com.android.setupwizardlib.view;

import android.annotation.TargetApi;
import android.content.Context;
import android.content.res.TypedArray;
import android.graphics.Color;
import android.os.Build.VERSION_CODES;
import android.util.AttributeSet;
import android.view.ContextThemeWrapper;
import android.view.View;
import android.widget.Button;
import android.widget.LinearLayout;

import com.android.setupwizardlib.R;

/**
 * Custom navigation bar for use with setup wizard. This bar contains a back button, more button and
 * next button. By default, the more button is hidden, and typically the next button will be hidden
 * if the more button is shown.
 *
 * @see com.android.setupwizardlib.util.RequireScrollHelper
 */
public class NavigationBar extends LinearLayout implements View.OnClickListener {

    /**
     * An interface to listen to events of the navigation bar, namely when the user clicks on the
     * back or next button.
     */
    public interface NavigationBarListener {
        void onNavigateBack();
        void onNavigateNext();
    }

    private static int getNavbarTheme(Context context) {
        // Normally we can automatically guess the theme by comparing the foreground color against
        // the background color. But we also allow specifying explicitly using suwNavBarTheme.
        TypedArray attributes = context.obtainStyledAttributes(
                new int[] {
                        R.attr.suwNavBarTheme,
                        android.R.attr.colorForeground,
                        android.R.attr.colorBackground });
        int theme = attributes.getResourceId(0, 0);
        if (theme == 0) {
            // Compare the value of the foreground against the background color to see if current
            // theme is light-on-dark or dark-on-light.
            float[] foregroundHsv = new float[3];
            float[] backgroundHsv = new float[3];
            Color.colorToHSV(attributes.getColor(1, 0), foregroundHsv);
            Color.colorToHSV(attributes.getColor(2, 0), backgroundHsv);
            boolean isDarkBg = foregroundHsv[2] > backgroundHsv[2];
            theme = isDarkBg ? R.style.SuwNavBarThemeDark : R.style.SuwNavBarThemeLight;
        }
        attributes.recycle();
        return theme;
    }

    private static Context getThemedContext(Context context) {
        final int theme = getNavbarTheme(context);
        return new ContextThemeWrapper(context, theme);
    }

    private Button mNextButton;
    private Button mBackButton;
    private Button mMoreButton;
    private NavigationBarListener mListener;

    public NavigationBar(Context context) {
        super(getThemedContext(context));
        init();
    }

    public NavigationBar(Context context, AttributeSet attrs) {
        super(getThemedContext(context), attrs);
        init();
    }

    @TargetApi(VERSION_CODES.HONEYCOMB)
    public NavigationBar(Context context, AttributeSet attrs, int defStyleAttr) {
        super(getThemedContext(context), attrs, defStyleAttr);
        init();
    }

    // All the constructors delegate to this init method. The 3-argument constructor is not
    // available in LinearLayout before v11, so call super with the exact same arguments.
    private void init() {
        View.inflate(getContext(), R.layout.suw_navbar_view, this);
        mNextButton = (Button) findViewById(R.id.suw_navbar_next);
        mBackButton = (Button) findViewById(R.id.suw_navbar_back);
        mMoreButton = (Button) findViewById(R.id.suw_navbar_more);
    }

    public Button getBackButton() {
        return mBackButton;
    }

    public Button getNextButton() {
        return mNextButton;
    }

    public Button getMoreButton() {
        return mMoreButton;
    }

    public void setNavigationBarListener(NavigationBarListener listener) {
        mListener = listener;
        if (mListener != null) {
            getBackButton().setOnClickListener(this);
            getNextButton().setOnClickListener(this);
        }
    }

    @Override
    public void onClick(View view) {
        if (mListener != null) {
            if (view == getBackButton()) {
                mListener.onNavigateBack();
            } else if (view == getNextButton()) {
                mListener.onNavigateNext();
            }
        }
    }
}
