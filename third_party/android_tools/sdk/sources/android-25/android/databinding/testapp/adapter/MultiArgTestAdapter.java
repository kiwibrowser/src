/*
 * Copyright (C) 2015 The Android Open Source Project
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *      http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package android.databinding.testapp.adapter;


import android.databinding.BaseObservable;
import android.databinding.Bindable;
import android.databinding.BindingAdapter;
import android.view.View;
import android.databinding.testapp.BR;
import android.widget.TextView;

public class MultiArgTestAdapter {

    public static String join(BaseMultiBindingClass... classes) {
        StringBuilder sb = new StringBuilder();
        for(BaseMultiBindingClass instance : classes) {
            sb.append(instance == null ? "??" : instance.getValue());
        }
        return sb.toString();
    }

    public static String join(String... strings) {
        StringBuilder sb = new StringBuilder();
        for(String str : strings) {
            sb.append(str == null ? "??" : str);
        }
        return sb.toString();

    }

    @BindingAdapter({"android:class1", "android:class2"})
    public static void setBoth(TextView view, MultiBindingClass1 class1,
                                        MultiBindingClass2 class2) {
        view.setText(join(class1, class2));
    }

    @BindingAdapter({"android:class1str", "android:class2str"})
    public static void setBoth(TextView view, String str1,
                               String str2) {
        view.setText(join(str1, str2));
    }

    @BindingAdapter({"android:class1"})
    public static void setClass1(TextView view, MultiBindingClass1 class1) {
        view.setText(class1.getValue());
    }

    @BindingAdapter({"android:classStr"})
    public static void setClassStr(TextView view, String str) {
        view.setText(str);
    }

    @BindingAdapter("android:class2")
    public static void setClass2(TextView view, MultiBindingClass2 class2) {
        view.setText(class2.getValue());
    }

    @BindingAdapter("android:val3")
    public static void setWithOldValue(TextView view, String oldValue, String newValue) {
        view.setText(String.format("%s -> %s", oldValue, newValue));
    }

    @BindingAdapter({"android:val3", "android:val4"})
    public static void set2WithOldValues(TextView view, String oldValue1, String oldValue2,
            String newValue1, String newValue2) {
        view.setText(String.format("%s, %s -> %s, %s", oldValue1, oldValue2, newValue1, newValue2));
    }

    public static class MultiBindingClass1 extends BaseMultiBindingClass {

    }

    public static class MultiBindingClass2 extends BaseMultiBindingClass {

    }

    public static class BaseMultiBindingClass extends BaseObservable {
        View mSetOn;
        @Bindable
        String mValue;
        public View getSetOn() {
            return mSetOn;
        }

        public String getValue() {
            return mValue;
        }

        public void setValue(String value, boolean notify) {
            mValue = value;
            if (notify) {
                notifyPropertyChanged(BR.value);
            }
        }

        public void clear() {
            mSetOn = null;
        }
    }
}
