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

import android.databinding.testapp.databinding.ViewWithTagBinding;
import android.support.annotation.UiThread;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

public class ViewWithTagTest extends BaseDataBinderTest<ViewWithTagBinding> {
    public ViewWithTagTest() {
        super(ViewWithTagBinding.class);
    }

    @UiThread
    public void test() {
        ViewWithTagBinding binder = initBinder();
        binder.setStr("i don't have tag");
        binder.executePendingBindings();
        ViewGroup root = (ViewGroup) binder.getRoot();
        View view1 = root.getChildAt(0);
        View view2 = root.getChildAt(1);
        assertTrue(view2 instanceof TextView);
        assertEquals("i don't have tag", ((TextView) view2).getText().toString());
        assertEquals("i have a tag", view1.getTag().toString());
        assertEquals("Hello", view2.getTag(R.id.customTag));
    }
}
