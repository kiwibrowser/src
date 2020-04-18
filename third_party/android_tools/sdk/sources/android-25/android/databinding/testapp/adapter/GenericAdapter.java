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
import android.databinding.testapp.GenericView;
import android.widget.TextView;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.List;

public class GenericAdapter {

    @BindingAdapter("textList1")
    public static <T> void setListText(TextView view, List<T> list) {
        setText(view, list);
    }

    @BindingAdapter("textList2")
    public static <T> void setCollectionText(TextView view, Collection<T> list) {
        setText(view, list);
    }

    @BindingAdapter("textArray")
    public static <T> void setArrayText(TextView view, T[] values) {
        setText(view, Arrays.asList(values));
    }

    @BindingAdapter({"textList1", "textArray"})
    public static <T> void setListAndArray(TextView view, List<T> list, T[] values) {
        setText(view, list);
    }

    @BindingAdapter("list")
    public static <T> void setGenericViewValue(GenericView<T> view, List<T> value) {
        view.setList(value);
    }

    @BindingAdapter({"list", "array"})
    public static <T> void setGenericListAndArray(GenericView<T> view, List<T> list, T[] values) {
        view.setList(list);
    }

    @BindingAdapter("textList3")
    public static void setGenericList(TextView view, List<String> list) {
        setText(view, list);
    }

    @BindingAdapter("textList3")
    public static void setGenericIntegerList(TextView view, List<Integer> list) {
    }

    private static <T> void setText(TextView view, Collection<T> collection) {
        StringBuilder stringBuilder = new StringBuilder();
        boolean isFirst = true;
        for (T val : collection) {
            if (isFirst) {
                isFirst = false;
            } else {
                stringBuilder.append(' ');
            }
            stringBuilder.append(val.toString());
        }
        view.setText(stringBuilder.toString());
    }
}
