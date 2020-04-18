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
import android.widget.DatePicker;
import android.widget.DatePicker.OnDateChangedListener;
import com.android.databinding.library.baseAdapters.R;

@InverseBindingMethods({
        @InverseBindingMethod(type = DatePicker.class, attribute = "android:year"),
        @InverseBindingMethod(type = DatePicker.class, attribute = "android:month"),
        @InverseBindingMethod(type = DatePicker.class, attribute = "android:day", method = "getDayOfMonth"),
})
public class DatePickerBindingAdapter {
    @BindingAdapter(value = {"android:year", "android:month", "android:day",
            "android:onDateChanged", "android:yearAttrChanged",
            "android:monthAttrChanged", "android:dayAttrChanged"}, requireAll = false)
    public static void setListeners(DatePicker view, int year, int month, int day,
            final OnDateChangedListener listener, final InverseBindingListener yearChanged,
            final InverseBindingListener monthChanged, final InverseBindingListener dayChanged) {
        if (year == 0) {
            year = view.getYear();
        }
        if (day == 0) {
            day = view.getDayOfMonth();
        }
        if (yearChanged == null && monthChanged == null && dayChanged == null) {
            view.init(year, month, day, listener);
        } else {
            DateChangedListener oldListener = ListenerUtil.getListener(view, R.id.onDateChanged);
            if (oldListener == null) {
                oldListener = new DateChangedListener();
                ListenerUtil.trackListener(view, oldListener, R.id.onDateChanged);
            }
            oldListener.setListeners(listener, yearChanged, monthChanged, dayChanged);
            view.init(year, month, day, oldListener);
        }
    }

    private static class DateChangedListener implements OnDateChangedListener {
        OnDateChangedListener mListener;
        InverseBindingListener mYearChanged;
        InverseBindingListener mMonthChanged;
        InverseBindingListener mDayChanged;

        public void setListeners(OnDateChangedListener listener, InverseBindingListener yearChanged,
                InverseBindingListener monthChanged, InverseBindingListener dayChanged) {
            mListener = listener;
            mYearChanged = yearChanged;
            mMonthChanged = monthChanged;
            mDayChanged = dayChanged;
        }

        @Override
        public void onDateChanged(DatePicker view, int year, int monthOfYear, int dayOfMonth) {
            if (mListener != null) {
                mListener.onDateChanged(view, year, monthOfYear, dayOfMonth);
            }
            if (mYearChanged != null) {
                mYearChanged.onChange();
            }
            if (mMonthChanged != null) {
                mMonthChanged.onChange();
            }
            if (mDayChanged != null) {
                mDayChanged.onChange();
            }
        }
    }
}
