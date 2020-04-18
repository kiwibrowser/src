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

import android.databinding.BindingConversion;
import java.util.ArrayList;
import java.util.LinkedList;
import java.util.List;

public class GenericConverter {
    @BindingConversion
    public static <T> String convertArrayList(ArrayList<T> values) {
        return convert(values);
    }

    @BindingConversion
    public static String convertLinkedList(LinkedList<?> values) {
        return convert(values);
    }

    private static <T> String convert(List<T> values) {
        if (values == null) {
            return "";
        }
        StringBuilder vals = new StringBuilder();
        for (T val : values) {
            if (vals.length() != 0) {
                vals.append(' ');
            }
            vals.append(val);
        }
        return vals.toString();
    }
}
