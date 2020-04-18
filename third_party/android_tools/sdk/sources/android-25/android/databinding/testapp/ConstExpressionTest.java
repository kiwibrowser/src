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

import android.databinding.testapp.databinding.ConstExpressionTestBinding;
import android.databinding.testapp.databinding.ExpressionTestBinding;
import android.test.UiThreadTest;

public class ConstExpressionTest extends BaseDataBinderTest<ConstExpressionTestBinding> {
    public ConstExpressionTest() {
        super(ConstExpressionTestBinding.class);
    }

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        initBinder();
    }


    @UiThreadTest
    public void testConstantExpression() throws Throwable {
        mBinder.setVar1(1000);
        mBinder.setVar2(2000);
        mBinder.executePendingBindings();
        assertEquals("1000", mBinder.textView.getText().toString());
    }
}
