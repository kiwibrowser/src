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
import android.databinding.InverseBindingAdapter;
import android.databinding.InverseBindingListener;
import android.os.Build.VERSION;
import android.os.Build.VERSION_CODES;
import android.widget.TimePicker;
import android.widget.TimePicker.OnTimeChangedListener;

public class TimePickerBindingAdapter {

    @SuppressWarnings("deprecation")
    @BindingAdapter("android:hour")
    public static void setHour(TimePicker view, int hour) {
        if (VERSION.SDK_INT >= VERSION_CODES.M) {
            if (view.getHour() != hour) {
                view.setHour(hour);
            }
        } else {
            if (view.getCurrentHour() != hour) {
                view.setCurrentHour(hour);
            }
        }
    }

    @SuppressWarnings("deprecation")
    @BindingAdapter("android:minute")
    public static void setMinute(TimePicker view, int minute) {
        if (VERSION.SDK_INT >= VERSION_CODES.M) {
            if (view.getMinute() != minute) {
                view.setMinute(minute);
            }
        } else {
            if (view.getCurrentMinute() != minute) {
                view.setCurrentHour(minute);
            }
        }
    }

    @InverseBindingAdapter(attribute = "android:hour")
    public static int getHour(TimePicker view) {
        if (VERSION.SDK_INT >= VERSION_CODES.M) {
            return view.getHour();
        } else {
            @SuppressWarnings("deprecation")
            Integer hour = view.getCurrentHour();
            if (hour == null) {
                return 0;
            } else {
                return hour;
            }
        }
    }

    @InverseBindingAdapter(attribute = "android:minute")
    public static int getMinute(TimePicker view) {
        if (VERSION.SDK_INT >= VERSION_CODES.M) {
            return view.getMinute();
        } else {
            @SuppressWarnings("deprecation")
            Integer minute = view.getCurrentMinute();
            if (minute == null) {
                return 0;
            } else {
                return minute;
            }
        }
    }

    @BindingAdapter(value = {"android:onTimeChanged", "android:hourAttrChanged",
            "android:minuteAttrChanged"}, requireAll = false)
    public static void setListeners(TimePicker view, final OnTimeChangedListener listener,
            final InverseBindingListener hourChange, final InverseBindingListener minuteChange) {
        if (hourChange == null && minuteChange == null) {
            view.setOnTimeChangedListener(listener);
        } else {
            view.setOnTimeChangedListener(new OnTimeChangedListener() {
                @Override
                public void onTimeChanged(TimePicker view, int hourOfDay, int minute) {
                    if (listener != null) {
                        listener.onTimeChanged(view, hourOfDay, minute);
                    }
                    if (hourChange != null) {
                        hourChange.onChange();
                    }
                    if (minuteChange != null) {
                        minuteChange.onChange();
                    }
                }
            });
        }
    }
}
