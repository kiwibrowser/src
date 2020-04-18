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
import android.databinding.adapters.AdapterViewBindingAdapter.OnItemSelected;
import android.databinding.adapters.AdapterViewBindingAdapter.OnItemSelectedComponentListener;
import android.databinding.adapters.AdapterViewBindingAdapter.OnNothingSelected;
import android.widget.AutoCompleteTextView;
import android.widget.AutoCompleteTextView.Validator;

@BindingMethods({
        @BindingMethod(type = AutoCompleteTextView.class, attribute = "android:completionThreshold", method = "setThreshold"),
        @BindingMethod(type = AutoCompleteTextView.class, attribute = "android:popupBackground", method = "setDropDownBackgroundDrawable"),
        @BindingMethod(type = AutoCompleteTextView.class, attribute = "android:onDismiss", method = "setOnDismissListener"),
        @BindingMethod(type = AutoCompleteTextView.class, attribute = "android:onItemClick", method = "setOnItemClickListener"),
})
public class AutoCompleteTextViewBindingAdapter {

    @BindingAdapter(value = {"android:fixText", "android:isValid"}, requireAll = false)
    public static void setValidator(AutoCompleteTextView view, final FixText fixText,
            final IsValid isValid) {
        if (fixText == null && isValid == null) {
            view.setValidator(null);
        } else {
            view.setValidator(new Validator() {
                @Override
                public boolean isValid(CharSequence text) {
                    if (isValid != null) {
                        return isValid.isValid(text);
                    } else {
                        return true;
                    }
                }

                @Override
                public CharSequence fixText(CharSequence invalidText) {
                    if (fixText != null) {
                        return fixText.fixText(invalidText);
                    } else {
                        return invalidText;
                    }
                }
            });
        }
    }

    @BindingAdapter(value = {"android:onItemSelected", "android:onNothingSelected"},
            requireAll = false)
    public static void setOnItemSelectedListener(AutoCompleteTextView view,
            final OnItemSelected selected, final OnNothingSelected nothingSelected) {
        if (selected == null && nothingSelected == null) {
            view.setOnItemSelectedListener(null);
        } else {
            view.setOnItemSelectedListener(
                    new OnItemSelectedComponentListener(selected, nothingSelected, null));
        }
    }

    public interface IsValid {
        boolean isValid(CharSequence text);
    }

    public interface FixText {
        CharSequence fixText(CharSequence invalidText);
    }
}
