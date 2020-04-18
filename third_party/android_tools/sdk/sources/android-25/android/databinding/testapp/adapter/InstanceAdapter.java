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
package android.databinding.testapp.adapter;

import android.databinding.BindingAdapter;
import android.databinding.DataBindingComponent;
import android.databinding.testapp.TestComponent;
import android.widget.TextView;

public class InstanceAdapter {
    private final String format;

    public InstanceAdapter(String format) {
        this.format = format;
    }

    @BindingAdapter("instanceAttr0")
    public void setInstanceAttr0(TextView view, String text) {
        view.setText(String.format(format, text, "foo", "bar", "baz"));
    }

    @BindingAdapter({"instanceAttr1", "instanceAttr2"})
    public void setInstanceAttr1(TextView view, String text, String text2) {
        view.setText(String.format(format, text, text2, "foo", "bar"));
    }

    @BindingAdapter("instanceAttr3")
    public void setInstanceAttr3(TextView view, String oldText, String text) {
        view.setText(String.format(format, oldText, text, "foo", "bar"));
    }

    @BindingAdapter({"instanceAttr4", "instanceAttr5"})
    public void setInstanceAttr4(TextView view, String oldText1, String oldText2,
            String text1, String text2) {
        view.setText(String.format(format, oldText1, oldText2, text1, text2));
    }

    @BindingAdapter("instanceAttr6")
    public static void setInstanceAttr6(DataBindingComponent component, TextView view, String text) {
        view.setText(String.format("%s %s", text, component == null ? "null" : "component"));
    }

    @BindingAdapter("instanceAttr7")
    public void setInstanceAttr7(DataBindingComponent component, TextView view, String text) {
        view.setText(String.format(format, text, component == null ? "null" : "component", "bar", "baz"));
    }

    @BindingAdapter({"instanceAttr8", "instanceAttr9"})
    public void setInstanceAttr8(TestComponent component, TextView view, String text, String text2) {
        view.setText(String.format(format, text, text2, component == null ? "null" : "component",
                "bar"));
    }
}
