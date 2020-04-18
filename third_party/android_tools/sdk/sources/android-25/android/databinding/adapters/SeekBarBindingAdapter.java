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
import android.widget.SeekBar;
import android.widget.SeekBar.OnSeekBarChangeListener;

@InverseBindingMethods({
        @InverseBindingMethod(type = SeekBar.class, attribute = "android:progress"),
})
public class SeekBarBindingAdapter {

    @BindingAdapter("android:progress")
    public static void setProgress(SeekBar view, int progress) {
        if (progress != view.getProgress()) {
            view.setProgress(progress);
        }
    }

    @BindingAdapter(value = {"android:onStartTrackingTouch", "android:onStopTrackingTouch",
            "android:onProgressChanged", "android:progressAttrChanged"}, requireAll = false)
    public static void setOnSeekBarChangeListener(SeekBar view, final OnStartTrackingTouch start,
            final OnStopTrackingTouch stop, final OnProgressChanged progressChanged,
            final InverseBindingListener attrChanged) {
        if (start == null && stop == null && progressChanged == null && attrChanged == null) {
            view.setOnSeekBarChangeListener(null);
        } else {
            view.setOnSeekBarChangeListener(new OnSeekBarChangeListener() {
                @Override
                public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                    if (progressChanged != null) {
                        progressChanged.onProgressChanged(seekBar, progress, fromUser);
                    }
                    if (attrChanged != null) {
                        attrChanged.onChange();
                    }
                }

                @Override
                public void onStartTrackingTouch(SeekBar seekBar) {
                    if (start != null) {
                        start.onStartTrackingTouch(seekBar);
                    }
                }

                @Override
                public void onStopTrackingTouch(SeekBar seekBar) {
                    if (stop != null) {
                        stop.onStopTrackingTouch(seekBar);
                    }
                }
            });
        }
    }

    public interface OnStartTrackingTouch {
        void onStartTrackingTouch(SeekBar seekBar);
    }

    public interface OnStopTrackingTouch {
        void onStopTrackingTouch(SeekBar seekBar);
    }

    public interface OnProgressChanged {
        void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser);
    }
}
