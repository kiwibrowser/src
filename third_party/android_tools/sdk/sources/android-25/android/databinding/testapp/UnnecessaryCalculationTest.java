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

import android.databinding.testapp.databinding.UnnecessaryCalculationBinding;
import android.databinding.testapp.vo.BasicObject;
import android.support.annotation.UiThread;
import android.test.UiThreadTest;

import java.util.concurrent.atomic.AtomicInteger;

public class UnnecessaryCalculationTest extends BaseDataBinderTest<UnnecessaryCalculationBinding> {

    public UnnecessaryCalculationTest() {
        super(UnnecessaryCalculationBinding.class);
    }

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        initBinder();
    }

    @UiThreadTest
    public void testDontSetUnnecessaryFlags() {
        BasicObjWithCounter obja = new BasicObjWithCounter();
        BasicObjWithCounter objb = new BasicObjWithCounter();
        BasicObjWithCounter objc = new BasicObjWithCounter();
        mBinder.setObja(obja);
        mBinder.setObjb(objb);
        mBinder.setObjc(objc);
        mBinder.setA(true);
        mBinder.setB(true);
        mBinder.setC(false);
        mBinder.executePendingBindings();
        assertEquals("true", mBinder.textView.getText().toString());
        assertEquals("true", mBinder.textView2.getText().toString());
        assertEquals(1, obja.counter);
        assertEquals(1, objb.counter);
        assertEquals(0, objc.counter);
        obja = new BasicObjWithCounter();
        mBinder.setObja(obja);
        mBinder.executePendingBindings();
        assertEquals("true", mBinder.textView.getText().toString());
        assertEquals("true", mBinder.textView2.getText().toString());
        assertEquals(1, obja.counter);
        assertEquals(1, objb.counter);
        assertEquals(0, objc.counter);
    }

    private static class BasicObjWithCounter extends BasicObject {
        int counter = 0;

        @Override
        public String boolMethod(boolean value) {
            counter ++;
            return super.boolMethod(value);
        }
    }

}
