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

package com.android.setupwizardlib.items;

import android.content.res.TypedArray;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.LayerDrawable;
import android.support.v7.widget.RecyclerView;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import com.android.setupwizardlib.R;

/**
 * An adapter used with RecyclerView to display an {@link ItemHierarchy}. The item hierarchy used to
 * create this adapter can be inflated by {@link com.android.setupwizardlib.items.ItemInflater} from
 * XML.
 */
public class RecyclerItemAdapter extends RecyclerView.Adapter<ItemViewHolder>
        implements ItemHierarchy.Observer {

    private static final String TAG = "RecyclerItemAdapter";

    public interface OnItemSelectedListener {
        void onItemSelected(IItem item);
    }

    private final ItemHierarchy mItemHierarchy;
    private OnItemSelectedListener mListener;

    public RecyclerItemAdapter(ItemHierarchy hierarchy) {
        mItemHierarchy = hierarchy;
        mItemHierarchy.registerObserver(this);
    }

    public IItem getItem(int position) {
        return mItemHierarchy.getItemAt(position);
    }

    @Override
    public long getItemId(int position) {
        IItem mItem = getItem(position);
        if (mItem instanceof AbstractItem) {
            final int id = ((AbstractItem) mItem).getId();
            return id > 0 ? id : RecyclerView.NO_ID;
        } else {
            return RecyclerView.NO_ID;
        }
    }

    @Override
    public int getItemCount() {
        return mItemHierarchy.getCount();
    }

    @Override
    public ItemViewHolder onCreateViewHolder(ViewGroup parent, int viewType) {
        final LayoutInflater inflater = LayoutInflater.from(parent.getContext());
        final View view = inflater.inflate(viewType, parent, false);
        final ItemViewHolder viewHolder = new ItemViewHolder(view);

        final TypedArray typedArray = parent.getContext()
                .obtainStyledAttributes(R.styleable.SuwRecyclerItemAdapter);
        Drawable selectableItemBackground = typedArray.getDrawable(
                R.styleable.SuwRecyclerItemAdapter_android_selectableItemBackground);
        if (selectableItemBackground == null) {
            selectableItemBackground = typedArray.getDrawable(
                    R.styleable.SuwRecyclerItemAdapter_selectableItemBackground);
        }

        final Drawable background = typedArray.getDrawable(
                R.styleable.SuwRecyclerItemAdapter_android_colorBackground);

        if (selectableItemBackground == null || background == null) {
            Log.e(TAG, "Cannot resolve required attributes."
                    + " selectableItemBackground=" + selectableItemBackground
                    + " background=" + background);
        } else {
            final Drawable[] layers = { background, selectableItemBackground };
            view.setBackgroundDrawable(new LayerDrawable(layers));
        }

        typedArray.recycle();

        view.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                final IItem item = viewHolder.getItem();
                if (mListener != null && item != null && item.isEnabled()) {
                    mListener.onItemSelected(item);
                }
            }
        });

        return viewHolder;
    }

    @Override
    public void onBindViewHolder(ItemViewHolder holder, int position) {
        final IItem item = getItem(position);
        item.onBindView(holder.itemView);
        holder.setEnabled(item.isEnabled());
        holder.setItem(item);
    }

    @Override
    public int getItemViewType(int position) {
        // Use layout resource as item view type. RecyclerView item type does not have to be
        // contiguous.
        IItem item = getItem(position);
        return item.getLayoutResource();
    }

    @Override
    public void onChanged(ItemHierarchy hierarchy) {
        notifyDataSetChanged();
    }

    public ItemHierarchy findItemById(int id) {
        return mItemHierarchy.findItemById(id);
    }

    public ItemHierarchy getRootItemHierarchy() {
        return mItemHierarchy;
    }

    public void setOnItemSelectedListener(OnItemSelectedListener listener) {
        mListener = listener;
    }
}
