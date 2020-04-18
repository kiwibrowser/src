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

import android.content.Context;
import android.content.res.TypedArray;
import android.support.v7.widget.SwitchCompat;
import android.util.AttributeSet;
import android.view.View;
import android.widget.CompoundButton;

import com.android.setupwizardlib.R;

/**
 * An Item with a switch, which the user can
 */
public class SwitchItem extends Item implements CompoundButton.OnCheckedChangeListener {

    public interface OnCheckedChangeListener {
        void onCheckedChange(SwitchItem item, boolean isChecked);
    }

    private boolean mChecked = false;
    private OnCheckedChangeListener mListener;

    public SwitchItem() {
        super();
    }

    public SwitchItem(Context context, AttributeSet attrs) {
        super(context, attrs);
        final TypedArray a = context.obtainStyledAttributes(attrs, R.styleable.SuwSwitchItem);
        mChecked = a.getBoolean(R.styleable.SuwSwitchItem_android_checked, false);
        a.recycle();
    }

    public void setChecked(boolean checked) {
        if (mChecked != checked) {
            mChecked = checked;
            notifyChanged();
            if (mListener != null) {
                mListener.onCheckedChange(this, checked);
            }
        }
    }

    public boolean isChecked() {
        return mChecked;
    }

    @Override
    protected int getDefaultLayoutResource() {
        return R.layout.suw_items_switch;
    }

    /**
     * Toggle the checked state of the switch, without invalidating the entire item.
     *
     * @param view The root view of this item, typically from the argument of onItemClick.
     */
    public void toggle(View view) {
        mChecked = !mChecked;
        final SwitchCompat switchView = (SwitchCompat) view.findViewById(R.id.suw_items_switch);
        switchView.setChecked(mChecked);
    }

    @Override
    public void onBindView(View view) {
        super.onBindView(view);
        final SwitchCompat switchView = (SwitchCompat) view.findViewById(R.id.suw_items_switch);
        switchView.setOnCheckedChangeListener(null);
        switchView.setChecked(mChecked);
        switchView.setOnCheckedChangeListener(this);
        switchView.setEnabled(isEnabled());
    }

    public void setOnCheckedChangeListener(OnCheckedChangeListener listener) {
        mListener = listener;
    }

    @Override
    public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
        mChecked = isChecked;
        if (mListener != null) {
            mListener.onCheckedChange(this, isChecked);
        }
    }
}
