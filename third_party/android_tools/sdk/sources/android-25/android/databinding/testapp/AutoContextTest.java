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

import android.databinding.testapp.databinding.AutoContextBinding;

import android.test.UiThreadTest;
import android.widget.TextView;

public class AutoContextTest extends BaseDataBinderTest<AutoContextBinding> {

    public AutoContextTest() {
        super(AutoContextBinding.class);
    }

    @UiThreadTest
    public void testContext() throws Throwable {
        AutoContextBinding binding = initBinder();
        binding.executePendingBindings();
        assertNotSame("", binding.textView1.getText().toString());
    }
}
