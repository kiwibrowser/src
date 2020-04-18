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

package android.databinding;


import android.test.AndroidTestCase;
import android.view.View;

public class DataBindingMapperTest extends AndroidTestCase {
    public void testNotDataBindingId() {
        View view = new View(getContext());
        view.setTag("layout/unexpected");
        android.databinding.DataBinderMapper mapper = new android.databinding.DataBinderMapper();
        ViewDataBinding binding = mapper.getDataBinder(null, view, 1);
        assertNull(binding);
    }
    public void testInvalidView() {
        View view = new View(getContext());
        view.setTag("layout/unexpected");
        android.databinding.DataBinderMapper mapper = new android.databinding.DataBinderMapper();
        Throwable error = null;
        try {
            mapper.getDataBinder(null, view, android.databinding.testapp.R.layout.multi_res_layout);
        } catch (Throwable t) {
            error = t;
        }
        assertNotNull(error);
        assertEquals("The tag for multi_res_layout is invalid. Received: layout/unexpected",
                error.getMessage());

    }
}
