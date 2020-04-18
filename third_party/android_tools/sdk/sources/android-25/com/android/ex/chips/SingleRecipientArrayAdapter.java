/*
 * Copyright (C) 2011 The Android Open Source Project
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

package com.android.ex.chips;

import android.content.Context;
import android.graphics.drawable.StateListDrawable;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;

import com.android.ex.chips.DropdownChipLayouter.AdapterType;

class SingleRecipientArrayAdapter extends ArrayAdapter<RecipientEntry> {
    private final DropdownChipLayouter mDropdownChipLayouter;
    private final StateListDrawable mDeleteDrawable;

    public SingleRecipientArrayAdapter(Context context, RecipientEntry entry,
        DropdownChipLayouter dropdownChipLayouter) {
        this(context, entry, dropdownChipLayouter, null);
    }

    public SingleRecipientArrayAdapter(Context context, RecipientEntry entry,
            DropdownChipLayouter dropdownChipLayouter, StateListDrawable deleteDrawable) {
        super(context,
                dropdownChipLayouter.getAlternateItemLayoutResId(AdapterType.SINGLE_RECIPIENT),
                new RecipientEntry[] { entry });

        mDropdownChipLayouter = dropdownChipLayouter;
        mDeleteDrawable = deleteDrawable;
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent) {
        return mDropdownChipLayouter.bindView(convertView, parent, getItem(position), position,
                AdapterType.SINGLE_RECIPIENT, null, mDeleteDrawable);
    }
}
