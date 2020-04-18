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
import android.support.v7.widget.LinearLayoutManager;
import android.support.v7.widget.RecyclerView;
import android.util.AttributeSet;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import com.android.setupwizardlib.items.ItemGroup;
import com.android.setupwizardlib.items.ItemInflater;
import com.android.setupwizardlib.items.RecyclerItemAdapter;
import com.android.setupwizardlib.util.DrawableLayoutDirectionHelper;
import com.android.setupwizardlib.view.HeaderRecyclerView;

/**
 * A GLIF themed layout with a RecyclerView. {@code android:entries} can also be used to specify an
 * {@link com.android.setupwizardlib.items.ItemHierarchy} to be used with this layout in XML.
 */
public class GlifRecyclerLayout extends GlifLayout {

    private RecyclerView mRecyclerView;
    private View mHeader;

    private DividerItemDecoration mDividerDecoration;
    private Drawable mDefaultDivider;
    private Drawable mDivider;
    private int mDividerInset;

    public GlifRecyclerLayout(Context context) {
        this(context, 0, 0);
    }

    public GlifRecyclerLayout(Context context, int template) {
        this(context, template, 0);
    }

    public GlifRecyclerLayout(Context context, int template, int containerId) {
        super(context, template, containerId);
        init(context, null, 0);
    }

    public GlifRecyclerLayout(Context context, AttributeSet attrs) {
        super(context, attrs);
        init(context, attrs, 0);
    }

    @TargetApi(VERSION_CODES.HONEYCOMB)
    public GlifRecyclerLayout(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        init(context, attrs, defStyleAttr);
    }

    private void init(Context context, AttributeSet attrs, int defStyleAttr) {
        final TypedArray a = context.obtainStyledAttributes(attrs,
                R.styleable.SuwGlifRecyclerLayout, defStyleAttr, 0);
        final int xml = a.getResourceId(R.styleable.SuwGlifRecyclerLayout_android_entries, 0);
        if (xml != 0) {
            final ItemGroup inflated = (ItemGroup) new ItemInflater(context).inflate(xml);
            RecyclerItemAdapter adapter = new RecyclerItemAdapter(inflated);
            adapter.setHasStableIds(a.getBoolean(
                    R.styleable.SuwGlifRecyclerLayout_suwHasStableIds, false));
            setAdapter(adapter);
        }
        int dividerInset =
                a.getDimensionPixelSize(R.styleable.SuwGlifRecyclerLayout_suwDividerInset, 0);
        if (dividerInset == 0) {
            dividerInset = getResources()
                    .getDimensionPixelSize(R.dimen.suw_items_glif_icon_divider_inset);
        }
        setDividerInset(dividerInset);
        a.recycle();
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
    protected View onInflateTemplate(LayoutInflater inflater, int template) {
        if (template == 0) {
            template = R.layout.suw_glif_recycler_template;
        }
        return super.onInflateTemplate(inflater, template);
    }

    @Override
    protected ViewGroup findContainer(int containerId) {
        if (containerId == 0) {
            containerId = R.id.suw_recycler_view;
        }
        return super.findContainer(containerId);
    }

    @Override
    protected void onTemplateInflated() {
        initRecyclerView((RecyclerView) findViewById(R.id.suw_recycler_view));
    }

    protected void initRecyclerView(RecyclerView recyclerView) {
        mRecyclerView = recyclerView;
        mRecyclerView.setLayoutManager(new LinearLayoutManager(getContext()));
        if (mRecyclerView instanceof HeaderRecyclerView) {
            mHeader = ((HeaderRecyclerView) mRecyclerView).getHeader();
        }
        mDividerDecoration = new DividerItemDecoration(getContext());
        mRecyclerView.addItemDecoration(mDividerDecoration);
    }

    @Override
    protected View findManagedViewById(int id) {
        if (mHeader != null) {
            final View view = mHeader.findViewById(id);
            if (view != null) {
                return view;
            }
        }
        return super.findViewById(id);
    }

    public void setDividerItemDecoration(DividerItemDecoration decoration) {
        mRecyclerView.removeItemDecoration(mDividerDecoration);
        mDividerDecoration = decoration;
        mRecyclerView.addItemDecoration(mDividerDecoration);
        updateDivider();
    }

    public RecyclerView getRecyclerView() {
        return mRecyclerView;
    }

    public void setAdapter(RecyclerView.Adapter adapter) {
        getRecyclerView().setAdapter(adapter);
    }

    public RecyclerView.Adapter getAdapter() {
        final RecyclerView.Adapter adapter = getRecyclerView().getAdapter();
        if (adapter instanceof HeaderRecyclerView.HeaderAdapter) {
            return ((HeaderRecyclerView.HeaderAdapter) adapter).getWrappedAdapter();
        }
        return adapter;
    }

    /**
     * Sets the start inset of the divider. This will use the default divider drawable set in the
     * theme and inset it {@code inset} pixels to the right (or left in RTL layouts).
     *
     * @param inset The number of pixels to inset on the "start" side of the list divider. Typically
     *              this will be either {@code @dimen/suw_items_glif_icon_divider_inset} or
     *              {@code @dimen/suw_items_glif_text_divider_inset}.
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
            if (mDefaultDivider == null) {
                mDefaultDivider = mDividerDecoration.getDivider();
            }
            mDivider = DrawableLayoutDirectionHelper.createRelativeInsetDrawable(mDefaultDivider,
                    mDividerInset /* start */, 0 /* top */, 0 /* end */, 0 /* bottom */, this);
            mDividerDecoration.setDivider(mDivider);
        }
    }

    public Drawable getDivider() {
        return mDivider;
    }
}
