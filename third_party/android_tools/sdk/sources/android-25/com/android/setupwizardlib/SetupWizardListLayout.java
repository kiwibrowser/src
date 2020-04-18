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
import android.content.res.TypedArray;
import android.graphics.drawable.Drawable;
import android.os.Build;
import android.os.Build.VERSION_CODES;
import android.util.AttributeSet;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ListAdapter;
import android.widget.ListView;

import com.android.setupwizardlib.util.DrawableLayoutDirectionHelper;
import com.android.setupwizardlib.util.ListViewRequireScrollHelper;
import com.android.setupwizardlib.view.NavigationBar;

public class SetupWizardListLayout extends SetupWizardLayout {

    private static final String TAG = "SetupWizardListLayout";
    private ListView mListView;
    private Drawable mDivider;
    private Drawable mDefaultDivider;
    private int mDividerInset;

    public SetupWizardListLayout(Context context) {
        this(context, 0, 0);
    }

    public SetupWizardListLayout(Context context, int template) {
        this(context, template, 0);
    }

    public SetupWizardListLayout(Context context, int template, int containerId) {
        super(context, template, containerId);
        init(context, null, 0);
    }

    public SetupWizardListLayout(Context context, AttributeSet attrs) {
        super(context, attrs);
        init(context, attrs, 0);
    }

    @TargetApi(VERSION_CODES.HONEYCOMB)
    public SetupWizardListLayout(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        init(context, attrs, defStyleAttr);
    }

    private void init(Context context, AttributeSet attrs, int defStyleAttr) {
        final TypedArray a = context.obtainStyledAttributes(attrs,
                R.styleable.SuwSetupWizardListLayout, defStyleAttr, 0);
        int dividerInset =
                a.getDimensionPixelSize(R.styleable.SuwSetupWizardListLayout_suwDividerInset, 0);
        setDividerInset(dividerInset);
        a.recycle();
    }

    @Override
    protected View onInflateTemplate(LayoutInflater inflater, int template) {
        if (template == 0) {
            template = R.layout.suw_list_template;
        }
        return super.onInflateTemplate(inflater, template);
    }

    @Override
    protected ViewGroup findContainer(int containerId) {
        if (containerId == 0) {
            containerId = android.R.id.list;
        }
        return super.findContainer(containerId);
    }

    @Override
    protected void onLayout(boolean changed, int left, int top, int right, int bottom) {
        super.onLayout(changed, left, top, right, bottom);
        if (mDivider == null) {
            // Update divider in case layout direction has just been resolved
            updateDivider();
        }
    }

    @Override
    protected void onTemplateInflated() {
        mListView = (ListView) findViewById(android.R.id.list);
    }

    public ListView getListView() {
        return mListView;
    }

    public void setAdapter(ListAdapter adapter) {
        getListView().setAdapter(adapter);
    }

    @Override
    public void requireScrollToBottom() {
        final NavigationBar navigationBar = getNavigationBar();
        final ListView listView = getListView();
        if (navigationBar != null && listView != null) {
            ListViewRequireScrollHelper.requireScroll(navigationBar, listView);
        } else {
            Log.e(TAG, "Both suw_layout_navigation_bar and list must exist in"
                    + " the template to require scrolling.");
        }
    }

    /**
     * Sets the start inset of the divider. This will use the default divider drawable set in the
     * theme and inset it {@code inset} pixels to the right (or left in RTL layouts).
     *
     * @param inset The number of pixels to inset on the "start" side of the list divider. Typically
     *              this will be either {@code @dimen/suw_items_icon_divider_inset} or
     *              {@code @dimen/suw_items_text_divider_inset}.
     */
    public void setDividerInset(int inset) {
        mDividerInset = inset;
        updateDivider();
    }

    public int getDividerInset() {
        return mDividerInset;
    }

    private void updateDivider() {
        boolean shouldUpdate = true;
        if (Build.VERSION.SDK_INT >= VERSION_CODES.KITKAT) {
            shouldUpdate = isLayoutDirectionResolved();
        }
        if (shouldUpdate) {
            final ListView listView = getListView();
            if (mDefaultDivider == null) {
                mDefaultDivider = listView.getDivider();
            }
            mDivider = DrawableLayoutDirectionHelper.createRelativeInsetDrawable(mDefaultDivider,
                    mDividerInset /* start */, 0 /* top */, 0 /* end */, 0 /* bottom */, this);
            listView.setDivider(mDivider);
        }
    }

    public Drawable getDivider() {
        return mDivider;
    }
}
