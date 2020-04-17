// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.widget;

import android.graphics.Color;
import org.chromium.base.ContextUtils;
import java.lang.reflect.Field;
import android.content.res.ColorStateList;
import android.content.Context;
import android.support.design.widget.TextInputLayout;
import android.text.TextUtils;
import android.util.AttributeSet;
import android.view.View;
import android.view.ViewGroup;
import android.widget.EditText;

import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.ui.R;

import java.util.ArrayList;

/**
 * Wraps around the Android Support library's {@link TextInputLayout} to handle various issues.
 */
public class CompatibilityTextInputLayout extends TextInputLayout {

    public CompatibilityTextInputLayout(Context context) {
        super(context);
    }

    public CompatibilityTextInputLayout(Context context, AttributeSet attrs) {
        super(context, attrs);
        // Disable the hint animation initially to work around a bug in the support library that
        // causes the hint text and text in populated EditText views to overlap when first
        // displayed on M-. See https://crbug.com/740057.
        setHintAnimationEnabled(false);
    }

    @Override
    public void setError(CharSequence error) {
        super.setError(error);
        if (TextUtils.isEmpty(error)) setErrorEnabled(false);
    }

    @Override
    public void onFinishInflate() {
        super.onFinishInflate();

        if (ContextUtils.getAppSharedPreferences().getBoolean("user_night_mode_enabled", false) || ContextUtils.getAppSharedPreferences().getString("active_theme", "").equals("Diamond Black")) {
            setHintTextAppearance(R.style.WhiteBody);
            try {
                Field fDefaultTextColor = TextInputLayout.class.getDeclaredField("mDefaultTextColor");
                fDefaultTextColor.setAccessible(true);
                fDefaultTextColor.set(this, new ColorStateList(new int[][]{{0}}, new int[]{ Color.WHITE }));

                Field fFocusedTextColor = TextInputLayout.class.getDeclaredField("mFocusedTextColor");
                fFocusedTextColor.setAccessible(true);
                fFocusedTextColor.set(this, new ColorStateList(new int[][]{{0}}, new int[]{ Color.WHITE }));
             } catch (Exception e) {
                e.printStackTrace();
             }
        }

        // If there is an EditText descendant, make this serve as the label for it.
        ArrayList<EditText> views = new ArrayList<>();
        findEditTextChildren(this, views);
        if (views.size() == 1) {
            ApiCompatibilityUtils.setLabelFor(this, views.get(0).getId());
            if (ContextUtils.getAppSharedPreferences().getBoolean("user_night_mode_enabled", false) || ContextUtils.getAppSharedPreferences().getString("active_theme", "").equals("Diamond Black")) {
                views.get(0).setTextColor(Color.WHITE);
                views.get(0).setHintTextColor(Color.WHITE);
            }
        }
    }

    @Override
    public void onAttachedToWindow() {
        super.onAttachedToWindow();
        setHintAnimationEnabled(true);
    }

    /**
     * Dig through the descendant hierarchy to find the EditText displayed by this TextInputLayout.
     * This is necessary because the Support Library version we're using automatically inserts a
     * FrameLayout when a TextInputEditText isn't used as the child.
     *
     * @param views Finds all EditText children of the root view.
     */
    private static void findEditTextChildren(ViewGroup root, ArrayList<EditText> views) {
        for (int nChild = 0; nChild < root.getChildCount(); nChild++) {
            View child = root.getChildAt(nChild);
            if (child instanceof ViewGroup) {
                findEditTextChildren((ViewGroup) child, views);
            } else if (child instanceof EditText) {
                views.add((EditText) child);
            }
        }
    }
}
