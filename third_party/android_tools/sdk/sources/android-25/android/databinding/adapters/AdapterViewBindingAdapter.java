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
package android.databinding.adapters;

import android.databinding.BindingAdapter;
import android.databinding.BindingMethod;
import android.databinding.BindingMethods;
import android.databinding.InverseBindingListener;
import android.databinding.InverseBindingMethod;
import android.databinding.InverseBindingMethods;
import android.view.View;
import android.widget.AbsListView;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemSelectedListener;

@BindingMethods({
        @BindingMethod(type = AdapterView.class, attribute = "android:onItemClick", method = "setOnItemClickListener"),
        @BindingMethod(type = AdapterView.class, attribute = "android:onItemLongClick", method = "setOnItemLongClickListener"),
})
@InverseBindingMethods({
        @InverseBindingMethod(type = AbsListView.class, attribute = "android:selectedItemPosition"),
})
public class AdapterViewBindingAdapter {

    @BindingAdapter("android:selectedItemPosition")
    public static void setSelectedItemPosition(AdapterView view, int position) {
        if (view.getSelectedItemPosition() != position) {
            view.setSelection(position);
        }
    }

    @BindingAdapter(value = {"android:onItemSelected", "android:onNothingSelected",
            "android:selectedItemPositionAttrChanged"}, requireAll = false)
    public static void setOnItemSelectedListener(AdapterView view, final OnItemSelected selected,
            final OnNothingSelected nothingSelected, final InverseBindingListener attrChanged) {
        if (selected == null && nothingSelected == null && attrChanged == null) {
            view.setOnItemSelectedListener(null);
        } else {
            view.setOnItemSelectedListener(
                    new OnItemSelectedComponentListener(selected, nothingSelected, attrChanged));
        }
    }

    public static class OnItemSelectedComponentListener implements OnItemSelectedListener {
        private final OnItemSelected mSelected;
        private final OnNothingSelected mNothingSelected;
        private final InverseBindingListener mAttrChanged;

        public OnItemSelectedComponentListener(OnItemSelected selected,
                OnNothingSelected nothingSelected, InverseBindingListener attrChanged) {
            this.mSelected = selected;
            this.mNothingSelected = nothingSelected;
            this.mAttrChanged = attrChanged;
        }

        @Override
        public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
            if (mSelected != null) {
                mSelected.onItemSelected(parent, view, position, id);
            }
            if (mAttrChanged != null) {
                mAttrChanged.onChange();
            }
        }

        @Override
        public void onNothingSelected(AdapterView<?> parent) {
            if (mNothingSelected != null) {
                mNothingSelected.onNothingSelected(parent);
            }
            if (mAttrChanged != null) {
                mAttrChanged.onChange();
            }
        }
    }

    public interface OnItemSelected {
        void onItemSelected(AdapterView<?> parent, View view, int position, long id);
    }

    public interface OnNothingSelected {
        void onNothingSelected(AdapterView<?> parent);
    }
}
