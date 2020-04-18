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

import android.databinding.testapp.databinding.FindFieldTestBinding;
import android.databinding.testapp.vo.FindFieldBindingObject;
import android.test.UiThreadTest;

public class FindFieldTest extends BaseDataBinderTest<FindFieldTestBinding> {
    public FindFieldTest() {
        super(FindFieldTestBinding.class);
    }

    @UiThreadTest
    public void test() {
        initBinder();
        FindFieldBindingObject obj = new FindFieldBindingObject();
        obj.mPublicField = "foo";
        mBinder.setObj(obj);
        mBinder.executePendingBindings();
        assertEquals(obj.mPublicField, mBinder.textView1.getText().toString());
    }

    @UiThreadTest
    public void testFieldOnGeneric() {
        initBinder();
        mBinder.executePendingBindings();
        assertEquals("Hello", mBinder.textView2.getText().toString());
    }
}
