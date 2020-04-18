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

package com.android.setupwizardlib;

import android.content.Context;
import android.util.AttributeSet;

import com.android.setupwizardlib.items.RecyclerItemAdapter;

/**
 * @deprecated Use {@link com.android.setupwizardlib.SetupWizardRecyclerLayout}
 */
@Deprecated
public class SetupWizardRecyclerItemsLayout extends SetupWizardRecyclerLayout {

    public SetupWizardRecyclerItemsLayout(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    public SetupWizardRecyclerItemsLayout(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
    }

    public RecyclerItemAdapter getAdapter() {
        return (RecyclerItemAdapter) super.getAdapter();
    }
}
