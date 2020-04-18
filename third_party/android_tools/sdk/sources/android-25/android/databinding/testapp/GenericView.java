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
package android.databinding.testapp;

import android.content.Context;
import android.util.AttributeSet;
import android.view.View;

import java.util.List;

public class GenericView<T> extends View {
    private List<T> mList;

    public GenericView(Context context) {
        super(context);
    }

    public GenericView(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    public GenericView(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
    }

    public GenericView(Context context, AttributeSet attrs, int defStyleAttr, int defStyleRes) {
        super(context, attrs, defStyleAttr, defStyleRes);
    }

    public void setList(List<T> list) {
        mList = list;
    }

    public List<T> getList() {
        return mList;
    }
}
