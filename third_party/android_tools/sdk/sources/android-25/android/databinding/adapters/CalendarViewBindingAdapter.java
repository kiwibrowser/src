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
import android.databinding.InverseBindingListener;
import android.databinding.InverseBindingMethod;
import android.databinding.InverseBindingMethods;
import android.widget.CalendarView;
import android.widget.CalendarView.OnDateChangeListener;

@InverseBindingMethods({
        @InverseBindingMethod(type = CalendarView.class, attribute = "android:date"),
})
public class CalendarViewBindingAdapter {
    @BindingAdapter("android:date")
    public static void setDate(CalendarView view, long date) {
        if (view.getDate() != date) {
            view.setDate(date);
        }
    }

    @BindingAdapter(value = {"android:onSelectedDayChange", "android:dateAttrChanged"},
            requireAll = false)
    public static void setListeners(CalendarView view, final OnDateChangeListener onDayChange,
            final InverseBindingListener attrChange) {
        if (attrChange == null) {
            view.setOnDateChangeListener(onDayChange);
        } else {
            view.setOnDateChangeListener(new OnDateChangeListener() {
                @Override
                public void onSelectedDayChange(CalendarView view, int year, int month,
                        int dayOfMonth) {
                    if (onDayChange != null) {
                        onDayChange.onSelectedDayChange(view, year, month, dayOfMonth);
                    }
                    attrChange.onChange();
                }
            });
        }
    }
}
