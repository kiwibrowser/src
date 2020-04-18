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

@BindingMethods({
        @BindingMethod(type = CompoundButton.class, attribute = "android:buttonTint", method = "setButtonTintList"),
        @BindingMethod(type = CompoundButton.class, attribute = "android:onCheckedChanged", method = "setOnCheckedChangeListener"),
})
@InverseBindingMethods({
        @InverseBindingMethod(type = CompoundButton.class, attribute = "android:checked"),
})
public class CompoundButtonBindingAdapter {

    @BindingAdapter("android:checked")
    public static void setChecked(CompoundButton view, boolean checked) {
        if (view.isChecked() != checked) {
            view.setChecked(checked);
        }
    }

    @BindingAdapter(value = {"android:onCheckedChanged", "android:checkedAttrChanged"},
            requireAll = false)
    public static void setListeners(CompoundButton view, final OnCheckedChangeListener listener,
            final InverseBindingListener attrChange) {
        if (attrChange == null) {
            view.setOnCheckedChangeListener(listener);
        } else {
            view.setOnCheckedChangeListener(new OnCheckedChangeListener() {
                @Override
                public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                    if (listener != null) {
                        listener.onCheckedChanged(buttonView, isChecked);
                    }
                    attrChange.onChange();
                }
            });
        }
    }

}
