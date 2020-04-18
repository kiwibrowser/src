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

import android.databinding.testapp.databinding.NameMappingTestBinding;
import android.databinding.testapp.vo.BasicObject;
import android.test.UiThreadTest;
import android.databinding.testapp.BR;

import java.util.concurrent.atomic.AtomicBoolean;

public class NameMappingTest extends BaseDataBinderTest<NameMappingTestBinding> {

    public NameMappingTest() {
        super(NameMappingTestBinding.class);
    }

    @UiThreadTest
    public void testChanges() {
        initBinder();
        final AtomicBoolean f1 = new AtomicBoolean(false);
        final AtomicBoolean f2 = new AtomicBoolean(false);
        BasicObject object = new BasicObject() {
            @Override
            public boolean isThisNameDoesNotMatchAnythingElse1() {
                return f1.get();
            }

            @Override
            public boolean getThisNameDoesNotMatchAnythingElse2() {
                return f2.get();
            }
        };
        mBinder.setObj(object);
        mBinder.executePendingBindings();
        for (int i = 0; i < 5; i ++) {
            boolean f1New = (i & 1) != 0;
            boolean f2New = (i & 1 << 1) != 0;
            if (f1New != f1.get()) {
                f1.set(f1New);
                object.notifyPropertyChanged(BR.thisNameDoesNotMatchAnythingElse1);
            }
            if (f2New != f2.get()) {
                f1.set(f2New);
                object.notifyPropertyChanged(BR.thisNameDoesNotMatchAnythingElse2);
            }
            mBinder.executePendingBindings();
            assertEquals(f2.get(), mBinder.textView.isEnabled());
            assertEquals(f2.get(), mBinder.textView2.isEnabled());
            assertEquals(false, mBinder.textView3.isEnabled());

            assertEquals(f1.get(), mBinder.textView.isFocusable());
            assertEquals(f1.get(), mBinder.textView2.isFocusable());
            assertEquals(false, mBinder.textView3.isFocusable());
        }
    }
}
