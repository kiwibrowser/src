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

package android.databinding.testapp;

import android.databinding.testapp.databinding.ConstantBindingWithConditionalBinding;
import android.databinding.testapp.vo.BasicObject;
import android.databinding.testapp.vo.ConstantBindingTestObject;
import android.test.UiThreadTest;

import java.util.ArrayList;

public class ConstantWithConditionalTest extends BaseDataBinderTest<ConstantBindingWithConditionalBinding>{

    public ConstantWithConditionalTest() {
        super(ConstantBindingWithConditionalBinding.class);
    }

    @UiThreadTest
    public void testValues() {
        initBinder();
        mBinder.executePendingBindings();
        BasicObject basicObject = new BasicObject();
        basicObject.setField1("tt");
        basicObject.setField2("blah");
        ConstantBindingTestObject obj = new ConstantBindingTestObject();
        mBinder.setVm(obj);
        mBinder.executePendingBindings();
        assertTrue(mBinder.myTextView.hasFixedSize());
        assertTrue(mBinder.progressBar.isIndeterminate());

        obj.setErrorMessage("blah");
        mBinder.invalidateAll();
        mBinder.executePendingBindings();
        assertFalse(mBinder.progressBar.isIndeterminate());

        obj.setErrorMessage(null);
        ArrayList<String> list = new ArrayList<>();
        obj.setCountryModels(list);
        mBinder.invalidateAll();
        mBinder.executePendingBindings();
        assertTrue(mBinder.progressBar.isIndeterminate());

        list.add("abc");
        mBinder.invalidateAll();
        mBinder.executePendingBindings();
        assertFalse(mBinder.progressBar.isIndeterminate());

    }
}
