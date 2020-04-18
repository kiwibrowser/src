/*
 * Copyright (C) 2014 The Android Open Source Project
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

package com.android.setupwizard.navigationbar;

import android.app.Activity;
import android.app.Fragment;
import android.content.Context;
import android.content.res.TypedArray;
import android.graphics.Color;
import android.os.Bundle;
import android.util.AttributeSet;
import android.view.ContextThemeWrapper;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.view.ViewTreeObserver;
import android.view.ViewTreeObserver.OnPreDrawListener;
import android.widget.Button;

/**
 * Fragment class for controlling the custom navigation bar shown during setup wizard. Apps in the
 * Android tree can use this by including the common.mk makefile. Apps outside of the tree can
 * create a library project out of the source.
 */
public class SetupWizardNavBar extends Fragment implements OnPreDrawListener, OnClickListener {
    private static final String TAG = "SetupWizardNavBar";
    private static final int IMMERSIVE_FLAGS =
            View.SYSTEM_UI_FLAG_HIDE_NAVIGATION | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY;
    private int mSystemUiFlags = IMMERSIVE_FLAGS | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION;

    private ViewGroup mNavigationBarView;
    private Button mNextButton;
    private Button mBackButton;
    private NavigationBarListener mCallback;

    public interface NavigationBarListener {
        public void onNavigationBarCreated(SetupWizardNavBar bar);
        public void onNavigateBack();
        public void onNavigateNext();
    }

    public SetupWizardNavBar() {
        // no-arg constructor for fragments
    }

    @Override
    public void onAttach(Activity activity) {
        super.onAttach(activity);
        mCallback = (NavigationBarListener) activity;
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
            Bundle savedInstanceState) {
        Context context = new ContextThemeWrapper(getActivity(), getNavbarTheme());
        inflater = LayoutInflater.from(context);
        mNavigationBarView = (ViewGroup) inflater.inflate(R.layout.setup_wizard_navbar_layout,
                container, false);
        mNextButton = (Button) mNavigationBarView.findViewById(R.id.setup_wizard_navbar_next);
        mBackButton = (Button) mNavigationBarView.findViewById(R.id.setup_wizard_navbar_back);
        mNextButton.setOnClickListener(this);
        mBackButton.setOnClickListener(this);
        return mNavigationBarView;
    }

    @Override
    public void onViewCreated(View view, Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);
        mCallback.onNavigationBarCreated(this);
        mNavigationBarView.setSystemUiVisibility(mSystemUiFlags);

        // Set the UI flags before draw because the visibility might change in unexpected /
        // undetectable times, like transitioning from a finishing activity that had a keyboard
        ViewTreeObserver viewTreeObserver = mNavigationBarView.getViewTreeObserver();
        viewTreeObserver.addOnPreDrawListener(this);
    }

    @Override
    public boolean onPreDraw() {
        // View.setSystemUiVisibility checks if the visibility changes before applying them
        // so the performance impact is contained
        mNavigationBarView.setSystemUiVisibility(mSystemUiFlags);
        return true;
    }

    /**
     * Sets whether system navigation bar should be hidden.
     * @param useImmersiveMode True to activate immersive mode and hide the system navigation bar
     */
    public void setUseImmersiveMode(boolean useImmersiveMode) {
        // By default, enable layoutHideNavigation if immersive mode is used
        setUseImmersiveMode(useImmersiveMode, useImmersiveMode);
    }

    public void setUseImmersiveMode(boolean useImmersiveMode, boolean layoutHideNavigation) {
        if (useImmersiveMode) {
            mSystemUiFlags |= IMMERSIVE_FLAGS;
            if (layoutHideNavigation) {
                mSystemUiFlags |= View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION;
            }
        } else {
            mSystemUiFlags &= ~(IMMERSIVE_FLAGS | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION);
        }
        mNavigationBarView.setSystemUiVisibility(mSystemUiFlags);
    }

    private int getNavbarTheme() {
        // Normally we can automatically guess the theme by comparing the foreground color against
        // the background color. But we also allow specifying explicitly using
        // setup_wizard_navbar_theme.
        TypedArray attributes = getActivity().obtainStyledAttributes(
                new int[] {
                        R.attr.setup_wizard_navbar_theme,
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
            theme = isDarkBg ? R.style.setup_wizard_navbar_theme_dark :
                    R.style.setup_wizard_navbar_theme_light;
        }
        attributes.recycle();
        return theme;
    }

    @Override
    public void onClick(View v) {
        if (v == mBackButton) {
            mCallback.onNavigateBack();
        } else if (v == mNextButton) {
            mCallback.onNavigateNext();
        }
    }

    public Button getBackButton() {
        return mBackButton;
    }

    public Button getNextButton() {
        return mNextButton;
    }

    public static class NavButton extends Button {

        public NavButton(Context context, AttributeSet attrs, int defStyleAttr, int defStyleRes) {
            super(context, attrs, defStyleAttr, defStyleRes);
        }

        public NavButton(Context context, AttributeSet attrs, int defStyleAttr) {
            super(context, attrs, defStyleAttr);
        }

        public NavButton(Context context, AttributeSet attrs) {
            super(context, attrs);
        }

        public NavButton(Context context) {
            super(context);
        }

        @Override
        public void setEnabled(boolean enabled) {
            super.setEnabled(enabled);
            // The color of the button is #de000000 / #deffffff when enabled. When disabled, apply
            // additional 23% alpha, so the overall opacity is 20%.
            setAlpha(enabled ? 1.0f : 0.23f);
        }
    }

}
