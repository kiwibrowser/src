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

import android.databinding.testapp.databinding.ExpressionTestBinding;
import android.test.UiThreadTest;

public class ExpressionTest extends BaseDataBinderTest<ExpressionTestBinding> {
    public ExpressionTest() {
        super(ExpressionTestBinding.class);
    }

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        initBinder();
    }

    @UiThreadTest
    public void testOr() throws Throwable {
        // var1 == 0 || var2 == 0 ? "hello" : "world"
        mBinder.setVar1(0);
        mBinder.setVar2(0);
        mBinder.executePendingBindings();
        assertEquals("hello", mBinder.textView0.getText().toString());
        mBinder.setVar2(1);
        mBinder.executePendingBindings();
        assertEquals("hello", mBinder.textView0.getText().toString());
        mBinder.setVar1(1);
        mBinder.setVar2(0);
        mBinder.executePendingBindings();
        assertEquals("hello", mBinder.textView0.getText().toString());
        mBinder.setVar2(1);
        mBinder.executePendingBindings();
        assertEquals("world", mBinder.textView0.getText().toString());
    }

    @UiThreadTest
    public void testAnd() throws Throwable {
        // var1 == 0 && var2 == 0 ? "hello" : "world"
        mBinder.setVar1(0);
        mBinder.setVar2(0);
        mBinder.executePendingBindings();
        assertEquals("hello", mBinder.textView1.getText().toString());
        mBinder.setVar2(1);
        mBinder.executePendingBindings();
        assertEquals("world", mBinder.textView1.getText().toString());
        mBinder.setVar1(1);
        mBinder.setVar2(0);
        mBinder.executePendingBindings();
        assertEquals("world", mBinder.textView1.getText().toString());
        mBinder.setVar2(1);
        mBinder.executePendingBindings();
        assertEquals("world", mBinder.textView1.getText().toString());
    }

    @UiThreadTest
    public void testBinary() throws Throwable {
        mBinder.setVar1(0);
        mBinder.setVar2(0);
        mBinder.executePendingBindings();
        assertEquals("0", mBinder.textView2.getText().toString()); // var1 & var2
        assertEquals("0", mBinder.textView3.getText().toString()); // var1 | var2
        assertEquals("0", mBinder.textView4.getText().toString()); // var1 ^ var2
        mBinder.setVar2(1);
        mBinder.executePendingBindings();
        assertEquals("0", mBinder.textView2.getText().toString()); // var1 & var2
        assertEquals("1", mBinder.textView3.getText().toString()); // var1 | var2
        assertEquals("1", mBinder.textView4.getText().toString()); // var1 ^ var2
        mBinder.setVar1(1);
        mBinder.executePendingBindings();
        assertEquals("1", mBinder.textView2.getText().toString()); // var1 & var2
        assertEquals("1", mBinder.textView3.getText().toString()); // var1 | var2
        assertEquals("0", mBinder.textView4.getText().toString()); // var1 ^ var2
    }

    @UiThreadTest
    public void testComparison() throws Throwable {
        mBinder.setVar1(0);
        mBinder.setVar2(0);
        mBinder.executePendingBindings();
        assertEquals("false", mBinder.textView5.getText().toString()); // <
        assertEquals("false", mBinder.textView6.getText().toString()); // >
        assertEquals("true", mBinder.textView7.getText().toString());  // <=
        assertEquals("true", mBinder.textView8.getText().toString());  // >=
        assertEquals("true", mBinder.textView9.getText().toString());  // ==

        mBinder.setVar2(1);
        mBinder.executePendingBindings();
        assertEquals("true", mBinder.textView5.getText().toString()); // <
        assertEquals("false", mBinder.textView6.getText().toString()); // >
        assertEquals("true", mBinder.textView7.getText().toString());  // <=
        assertEquals("false", mBinder.textView8.getText().toString());  // >=
        assertEquals("false", mBinder.textView9.getText().toString());  // ==
        mBinder.setVar1(1);
        mBinder.setVar2(0);
        mBinder.executePendingBindings();
        assertEquals("false", mBinder.textView5.getText().toString()); // <
        assertEquals("true", mBinder.textView6.getText().toString()); // >
        assertEquals("false", mBinder.textView7.getText().toString());  // <=
        assertEquals("true", mBinder.textView8.getText().toString());  // >=
        assertEquals("false", mBinder.textView9.getText().toString());  // ==
    }

    @UiThreadTest
    public void testShift() throws Throwable {
        mBinder.setVar1(-2);
        mBinder.setVar2(1);
        mBinder.executePendingBindings();
        int var1 = -2;
        int var2 = 1;
        assertEquals(String.valueOf(var1 << var2), mBinder.textView10.getText().toString());
        assertEquals(String.valueOf(var1 >> var2), mBinder.textView11.getText().toString());
        assertEquals(String.valueOf(var1 >>> var2), mBinder.textView12.getText().toString());
    }

    @UiThreadTest
    public void testUnary() throws Throwable {
        mBinder.setVar1(2);
        mBinder.setVar2(1);
        mBinder.executePendingBindings();
        int var1 = 2;
        int var2 = 1;
        assertEquals("1", mBinder.textView13.getText().toString()); // 2 + -1
        assertEquals(String.valueOf(var1 + ~var2), mBinder.textView14.getText().toString()); // 2 + ~1
    }
    @UiThreadTest
    public void testInstanceOf() throws Throwable {
        mBinder.executePendingBindings();
        assertEquals("true", mBinder.textView15.getText().toString());
        assertEquals("true", mBinder.textView16.getText().toString());
        assertEquals("false", mBinder.textView17.getText().toString());
    }

    @UiThreadTest
    public void testTernaryChain()  throws Throwable {
        mBinder.setBool1(true);
        mBinder.setBool2(false);
        mBinder.executePendingBindings();
        String appName = getActivity().getResources().getString(R.string.app_name);
        String rain = getActivity().getResources().getString(R.string.rain);
        assertEquals(mBinder.getBool1() ? appName : mBinder.getBool2() ? rain : "",
                mBinder.textView18.getText().toString());
    }

    @UiThreadTest
    public void testBoundTag() throws Throwable {
        mBinder.setBool1(false);
        mBinder.executePendingBindings();
        assertEquals("bar", mBinder.textView19.getTag());
        mBinder.setBool1(true);
        mBinder.executePendingBindings();
        assertEquals("foo", mBinder.textView19.getTag());
    }

    @UiThreadTest
    public void testConstantExpression() throws Throwable {
        mBinder.setVar1(1000);
        mBinder.setVar2(2000);
        mBinder.executePendingBindings();
        assertEquals("1000", mBinder.textView20.getText().toString());
        assertEquals("2000", mBinder.textView21.getText().toString());
    }
}
