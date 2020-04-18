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

package com.android.setupwizardlib;

import android.annotation.TargetApi;
import android.content.Context;
import android.content.res.ColorStateList;
import android.content.res.TypedArray;
import android.graphics.drawable.Drawable;
import android.os.Build;
import android.os.Build.VERSION_CODES;
import android.util.AttributeSet;
import android.view.InflateException;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewStub;
import android.widget.ImageView;
import android.widget.ProgressBar;
import android.widget.ScrollView;
import android.widget.TextView;

import com.android.setupwizardlib.view.StatusBarBackgroundLayout;

/**
 * Layout for the GLIF theme used in Setup Wizard for N.
 *
 * <p>Example usage:
 * <pre>{@code
 * &lt;com.android.setupwizardlib.GlifLayout
 *     xmlns:android="http://schemas.android.com/apk/res/android"
 *     xmlns:app="http://schemas.android.com/apk/res-auto"
 *     android:layout_width="match_parent"
 *     android:layout_height="match_parent"
 *     android:icon="@drawable/my_icon"
 *     app:suwHeaderText="@string/my_title">
 *
 *     &lt;!-- Content here -->
 *
 * &lt;/com.android.setupwizardlib.GlifLayout>
 * }</pre>
 */
public class GlifLayout extends TemplateLayout {

    private static final String TAG = "GlifLayout";

    private ColorStateList mPrimaryColor;

    public GlifLayout(Context context) {
        this(context, 0, 0);
    }

    public GlifLayout(Context context, int template) {
        this(context, template, 0);
    }

    public GlifLayout(Context context, int template, int containerId) {
        super(context, template, containerId);
        init(null, R.attr.suwLayoutTheme);
    }

    public GlifLayout(Context context, AttributeSet attrs) {
        super(context, attrs);
        init(attrs, R.attr.suwLayoutTheme);
    }

    @TargetApi(VERSION_CODES.HONEYCOMB)
    public GlifLayout(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        init(attrs, defStyleAttr);
    }

    // All the constructors delegate to this init method. The 3-argument constructor is not
    // available in LinearLayout before v11, so call super with the exact same arguments.
    private void init(AttributeSet attrs, int defStyleAttr) {
        final TypedArray a = getContext().obtainStyledAttributes(attrs,
                R.styleable.SuwGlifLayout, defStyleAttr, 0);

        final Drawable icon = a.getDrawable(R.styleable.SuwGlifLayout_android_icon);
        if (icon != null) {
            setIcon(icon);
        }

        // Set the header color
        final ColorStateList headerColor =
                a.getColorStateList(R.styleable.SuwGlifLayout_suwHeaderColor);
        if (headerColor != null) {
            setHeaderColor(headerColor);
        }


        // Set the header text
        final CharSequence headerText =
                a.getText(R.styleable.SuwGlifLayout_suwHeaderText);
        if (headerText != null) {
            setHeaderText(headerText);
        }

        final ColorStateList primaryColor =
                a.getColorStateList(R.styleable.SuwGlifLayout_android_colorPrimary);
        setPrimaryColor(primaryColor);

        a.recycle();
    }

    @Override
    protected View onInflateTemplate(LayoutInflater inflater, int template) {
        if (template == 0) {
            template = R.layout.suw_glif_template;
        }
        try {
            return super.onInflateTemplate(inflater, template);
        } catch (RuntimeException e) {
            // Versions before M throws RuntimeException for unsuccessful attribute resolution
            // Versions M+ will throw an InflateException (which extends from RuntimeException)
            throw new InflateException("Unable to inflate layout. Are you using "
                    + "@style/SuwThemeGlif (or its descendant) as your theme?", e);
        }
    }

    @Override
    protected ViewGroup findContainer(int containerId) {
        if (containerId == 0) {
            containerId = R.id.suw_layout_content;
        }
        return super.findContainer(containerId);
    }

    /**
     * Same as {@link android.view.View#findViewById(int)}, but may include views that are managed
     * by this view but not currently added to the view hierarchy. e.g. recycler view or list view
     * headers that are not currently shown.
     */
    protected View findManagedViewById(int id) {
        return findViewById(id);
    }

    public ScrollView getScrollView() {
        final View view = findManagedViewById(R.id.suw_scroll_view);
        return view instanceof ScrollView ? (ScrollView) view : null;
    }

    public TextView getHeaderTextView() {
        return (TextView) findManagedViewById(R.id.suw_layout_title);
    }

    public void setHeaderText(int title) {
        setHeaderText(getContext().getResources().getText(title));
    }

    public void setHeaderText(CharSequence title) {
        final TextView titleView = getHeaderTextView();
        if (titleView != null) {
            titleView.setText(title);
        }
    }

    public CharSequence getHeaderText() {
        final TextView titleView = getHeaderTextView();
        return titleView != null ? titleView.getText() : null;
    }

    public void setHeaderColor(ColorStateList color) {
        final TextView titleView = getHeaderTextView();
        if (titleView != null) {
            titleView.setTextColor(color);
        }
    }

    public ColorStateList getHeaderColor() {
        final TextView titleView = getHeaderTextView();
        return titleView != null ? titleView.getTextColors() : null;
    }

    public void setIcon(Drawable icon) {
        final ImageView iconView = getIconView();
        if (iconView != null) {
            iconView.setImageDrawable(icon);
        }
    }

    public Drawable getIcon() {
        final ImageView iconView = getIconView();
        return iconView != null ? iconView.getDrawable() : null;
    }

    protected ImageView getIconView() {
        return (ImageView) findManagedViewById(R.id.suw_layout_icon);
    }

    public void setPrimaryColor(ColorStateList color) {
        mPrimaryColor = color;
        setGlifPatternColor(color);
        setProgressBarColor(color);
    }

    public ColorStateList getPrimaryColor() {
        return mPrimaryColor;
    }

    private void setGlifPatternColor(ColorStateList color) {
        if (Build.VERSION.SDK_INT >= VERSION_CODES.LOLLIPOP) {
            setSystemUiVisibility(View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN);
            final View patternBg = findManagedViewById(R.id.suw_pattern_bg);
            if (patternBg != null) {
                final GlifPatternDrawable background =
                        new GlifPatternDrawable(color.getDefaultColor());
                if (patternBg instanceof StatusBarBackgroundLayout) {
                    ((StatusBarBackgroundLayout) patternBg).setStatusBarBackground(background);
                } else {
                    patternBg.setBackground(background);
                }
            }
        }
    }

    public boolean isProgressBarShown() {
        final View progressBar = findManagedViewById(R.id.suw_layout_progress);
        return progressBar != null && progressBar.getVisibility() == View.VISIBLE;
    }

    public void setProgressBarShown(boolean shown) {
        if (shown) {
            View progressBar = getProgressBar();
            if (progressBar != null) {
                progressBar.setVisibility(View.VISIBLE);
            }
        } else {
            View progressBar = peekProgressBar();
            if (progressBar != null) {
                progressBar.setVisibility(View.GONE);
            }
        }
    }

    /**
     * Gets the progress bar in the layout. If the progress bar has not been used before, it will be
     * installed (i.e. inflated from its view stub).
     *
     * @return The progress bar of this layout. May be null only if the template used doesn't have a
     *         progress bar built-in.
     */
    private ProgressBar getProgressBar() {
        final View progressBar = peekProgressBar();
        if (progressBar == null) {
            final ViewStub progressBarStub =
                    (ViewStub) findManagedViewById(R.id.suw_layout_progress_stub);
            if (progressBarStub != null) {
                progressBarStub.inflate();
            }
            setProgressBarColor(mPrimaryColor);
        }
        return peekProgressBar();
    }

    /**
     * Gets the progress bar in the layout only if it has been installed.
     * {@link #setProgressBarShown(boolean)} should be called before this to ensure the progress bar
     * is set up correctly.
     *
     * @return The progress bar of this layout, or null if the progress bar is not installed. The
     *         null case can happen either if {@link #setProgressBarShown(boolean)} with true was
     *         not called before this, or if the template does not contain a progress bar.
     */
    public ProgressBar peekProgressBar() {
        return (ProgressBar) findManagedViewById(R.id.suw_layout_progress);
    }

    private void setProgressBarColor(ColorStateList color) {
        if (Build.VERSION.SDK_INT >= VERSION_CODES.LOLLIPOP) {
            final ProgressBar bar = peekProgressBar();
            if (bar != null) {
                bar.setIndeterminateTintList(color);
            }
        }
    }
}
