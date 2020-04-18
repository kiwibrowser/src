/*
 * Copyright (C) 2016 The Android Open Source Project
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

import android.content.Context;
import android.os.Bundle;
import android.support.v7.widget.RecyclerView;
import android.util.AttributeSet;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

/**
 * A layout to be used with {@code PreferenceFragment} in v14 support library. This can be specified
 * as the {@code android:layout} in the {@code app:preferenceFragmentStyle} in
 * {@code app:preferenceTheme}.
 *
 * <p />Example:
 * <pre>{@code
 * &lt;style android:name="MyActivityTheme">
 *     &lt;item android:name="preferenceTheme">@style/MyPreferenceTheme&lt;/item>
 * &lt;/style>
 *
 * &lt;style android:name="MyPreferenceTheme">
 *     &lt;item android:name="preferenceFragmentStyle">@style/MyPreferenceFragmentStyle&lt;/item>
 * &lt;/style>
 *
 * &lt;style android:name="MyPreferenceFragmentStyle">
 *     &lt;item android:name="android:layout">@layout/my_preference_layout&lt;/item>
 * &lt;/style>
 * }</pre>
 *
 * where {@code my_preference_layout} is a layout that contains
 * {@link com.android.setupwizardlib.GlifPreferenceLayout}.
 *
 * <p />Example:
 * <pre>{@code
 * &lt;com.android.setupwizardlib.GlifPreferenceLayout
 *     xmlns:android="http://schemas.android.com/apk/res/android"
 *     android:id="@id/list_container"
 *     android:layout_width="match_parent"
 *     android:layout_height="match_parent" />
 * }</pre>
 *
 * <p />Fragments using this layout <em>must</em> delegate {@code onCreateRecyclerView} to the
 * implementation in this class:
 * {@link #onCreateRecyclerView(android.view.LayoutInflater, android.view.ViewGroup,
 * android.os.Bundle)}
 */
public class GlifPreferenceLayout extends GlifRecyclerLayout {

    private RecyclerView mRecyclerView;

    public GlifPreferenceLayout(Context context) {
        super(context);
    }

    public GlifPreferenceLayout(Context context, int template, int containerId) {
        super(context, template, containerId);
    }

    public GlifPreferenceLayout(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    public GlifPreferenceLayout(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
    }

    public RecyclerView getRecyclerView() {
        return mRecyclerView;
    }

    @Override
    protected ViewGroup findContainer(int containerId) {
        if (containerId == 0) {
            containerId = R.id.suw_layout_content;
        }
        return super.findContainer(containerId);
    }

    /**
     * This method must be called in {@code PreferenceFragment#onCreateRecyclerView}.
     */
    public RecyclerView onCreateRecyclerView(LayoutInflater inflater, ViewGroup parent,
            Bundle savedInstanceState) {
        return mRecyclerView;
    }

    @Override
    protected View onInflateTemplate(LayoutInflater inflater, int template) {
        if (template == 0) {
            template = R.layout.suw_glif_preference_template;
        }
        return super.onInflateTemplate(inflater, template);
    }

    @Override
    protected void onTemplateInflated() {
        // Inflate the recycler view here, so attributes on the decoration views can be applied
        // immediately.
        final LayoutInflater inflater = LayoutInflater.from(getContext());
        mRecyclerView = (RecyclerView) inflater.inflate(R.layout.suw_glif_preference_recycler_view,
                this, false);
        initRecyclerView(mRecyclerView);
    }
}
