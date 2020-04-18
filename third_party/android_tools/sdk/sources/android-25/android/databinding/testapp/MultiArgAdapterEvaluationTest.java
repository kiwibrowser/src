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

import android.databinding.testapp.BR;
import android.databinding.testapp.databinding.MultiArgAdapterEvaluationTestBinding;
import android.databinding.testapp.databinding.MultiArgAdapterTestBinding;
import android.test.UiThreadTest;

import static android.databinding.testapp.adapter.MultiArgTestAdapter.MultiBindingClass1;
import static android.databinding.testapp.adapter.MultiArgTestAdapter.MultiBindingClass2;
import static android.databinding.testapp.adapter.MultiArgTestAdapter.join;

public class MultiArgAdapterEvaluationTest extends BaseDataBinderTest<MultiArgAdapterEvaluationTestBinding> {

    public MultiArgAdapterEvaluationTest() {
        super(MultiArgAdapterEvaluationTestBinding.class);
    }

    @UiThreadTest
    public void testMultiArgIsCalled() {
        initBinder();
        MultiBindingClass1 obj1 = new MultiBindingClass1();
        MultiBindingClass2 obj2 = new MultiBindingClass2();
        obj1.setValue("a", false);
        obj2.setValue("b", false);
        mBinder.setObj1(obj1);
        mBinder.setObj2(obj2);
        mBinder.executePendingBindings();

        assertEquals(mBinder.merged.getText().toString(), join(obj1.getValue(), obj2.getValue()));
        assertEquals(mBinder.view2.getText().toString(), join(obj2.getValue()));
        assertEquals(mBinder.view2text.getText().toString(), obj2.getValue());

        String prev2 = mBinder.view2.getText().toString();
        String prevValue = mBinder.merged.getText().toString();
        obj1.setValue("o", false);
        mBinder.executePendingBindings();
        assertEquals(prevValue, mBinder.merged.getText().toString());
        obj2.setValue("p", false);
        mBinder.executePendingBindings();
        assertEquals(prevValue, mBinder.merged.getText().toString());
        // now invalidate obj1 only, obj2 should be evaluated as well
        obj1.setValue("o2", true);
        mBinder.executePendingBindings();
        assertEquals(join(obj1, obj2), mBinder.merged.getText().toString());
        assertEquals("obj2 should not be re-evaluated", prev2, mBinder.view2.getText().toString());
        assertEquals("obj2 should not be re-evaluated", prev2,
                mBinder.view2text.getText().toString());
    }
}
