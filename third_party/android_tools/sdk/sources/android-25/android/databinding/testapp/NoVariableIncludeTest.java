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

import android.databinding.testapp.databinding.IncludeNoVariablesBinding;
import android.test.UiThreadTest;
import android.view.ViewGroup;
import android.widget.TextView;

public class NoVariableIncludeTest extends BaseDataBinderTest<IncludeNoVariablesBinding> {

    public NoVariableIncludeTest() {
        super(IncludeNoVariablesBinding.class);
    }

    @UiThreadTest
    public void testInclude() {
        initBinder();
        mBinder.executePendingBindings();
        assertNotNull(mBinder.included);
        assertNotNull(mBinder.included.textView);
        String expectedValue = getActivity().getResources().getString(R.string.app_name);
        assertEquals(expectedValue, mBinder.included.textView.getText().toString());
        TextView noIdInclude = (TextView) ((ViewGroup) mBinder.getRoot()).getChildAt(1);
        assertEquals(expectedValue, noIdInclude.getText().toString());
    }
}
