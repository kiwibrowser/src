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
import android.widget.CompoundButton;
import android.widget.CompoundButton.OnCheckedChangeListener;
import android.widget.NumberPicker;
import android.widget.NumberPicker.OnValueChangeListener;

@BindingMethods({
        @BindingMethod(type = NumberPicker.class, attribute = "android:format", method = "setFormatter"),
        @BindingMethod(type = NumberPicker.class, attribute = "android:onScrollStateChange", method = "setOnScrollListener"),
})
@InverseBindingMethods({
        @InverseBindingMethod(type = NumberPicker.class, attribute = "android:value"),
})
public class NumberPickerBindingAdapter {

    @BindingAdapter("android:value")
    public static void setValue(NumberPicker view, int value) {
        if (view.getValue() != value) {
            view.setValue(value);
        }
    }

    @BindingAdapter(value = {"android:onValueChange", "android:valueAttrChanged"},
            requireAll = false)
    public static void setListeners(NumberPicker view, final OnValueChangeListener listener,
            final InverseBindingListener attrChange) {
        if (attrChange == null) {
            view.setOnValueChangedListener(listener);
        } else {
            view.setOnValueChangedListener(new OnValueChangeListener() {
                @Override
                public void onValueChange(NumberPicker picker, int oldVal, int newVal) {
                    if (listener != null) {
                        listener.onValueChange(picker, oldVal, newVal);
                    }
                    attrChange.onChange();
                }
            });
        }
    }
}
