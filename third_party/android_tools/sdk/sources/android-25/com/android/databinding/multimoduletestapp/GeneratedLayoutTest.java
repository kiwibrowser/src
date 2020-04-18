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

package com.android.databinding.multimoduletestapp;

import android.databinding.DataBindingUtil;
import android.databinding.ViewDataBinding;
import android.databinding.multimoduletestapp.R;
import android.test.AndroidTestCase;
import android.view.LayoutInflater;
import android.view.View;

public class GeneratedLayoutTest extends AndroidTestCase {
    public void testBindToGeneratedLayout() {
        LayoutInflater inflater = LayoutInflater.from(getContext());
        View view = inflater.inflate(R.layout.library_layout, null);
        // force override tag
        view.setTag("layout-sw600dp-land-v13/library_layout_0");
        ViewDataBinding bind = DataBindingUtil.bind(view);
        assertEquals("IndependentLibraryBindingSw600dpLandV13Impl",
                bind.getClass().getSimpleName());
    }

    public void testBindToDefault() {
        LayoutInflater inflater = LayoutInflater.from(getContext());
        View view = inflater.inflate(R.layout.library_layout, null);
        // force override tag
        view.setTag("layout/library_layout_0");
        ViewDataBinding bind = DataBindingUtil.bind(view);
        assertEquals("IndependentLibraryBindingImpl",
                bind.getClass().getSimpleName());
    }

    public void testBindToSw600() {
        LayoutInflater inflater = LayoutInflater.from(getContext());
        View view = inflater.inflate(R.layout.library_layout, null);
        // force override tag
        view.setTag("layout-sw600dp-land/library_layout_0");
        ViewDataBinding bind = DataBindingUtil.bind(view);
        assertEquals("IndependentLibraryBindingSw600dpLandImpl",
                bind.getClass().getSimpleName());
    }
}
