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


import android.databinding.testapp.databinding.StaticAccessTestBinding;
import android.databinding.testapp.vo.StaticTestsVo;
import android.test.UiThreadTest;
import android.widget.TextView;

import java.util.UUID;

public class StaticAccessTest extends BaseDataBinderTest<StaticAccessTestBinding> {

    public StaticAccessTest() {
        super(StaticAccessTestBinding.class);
    }

    @UiThreadTest
    public void testAccessStatics() {
        initBinder();
        StaticTestsVo vo = new StaticTestsVo();
        mBinder.setVo(vo);
        assertStaticContents();
    }

    private void assertStaticContents() {
        mBinder.executePendingBindings();
        assertText(StaticTestsVo.ourStaticField, mBinder.staticFieldOverVo);
        assertText(StaticTestsVo.ourStaticMethod(), mBinder.staticMethodOverVo);
        assertText(StaticTestsVo.ourStaticObservable.get(), mBinder.obsStaticOverVo);

        assertText(StaticTestsVo.ourStaticField, mBinder.staticFieldOverClass);
        assertText(StaticTestsVo.ourStaticMethod(), mBinder.staticMethodOverClass);
        assertText(StaticTestsVo.ourStaticObservable.get(), mBinder.obsStaticOverClass);

        String newValue = UUID.randomUUID().toString();
        StaticTestsVo.ourStaticObservable.set(newValue);
        mBinder.executePendingBindings();
        assertText(StaticTestsVo.ourStaticObservable.get(), mBinder.obsStaticOverVo);
        assertText(StaticTestsVo.ourStaticObservable.get(), mBinder.obsStaticOverClass);
    }

    @UiThreadTest
    public void testAccessStaticsVoInstance() {
        initBinder();
        mBinder.setVo(null);
        assertStaticContents();
    }

    private void assertText(String contents, TextView textView) {
        assertEquals(contents, textView.getText().toString());
    }
}
