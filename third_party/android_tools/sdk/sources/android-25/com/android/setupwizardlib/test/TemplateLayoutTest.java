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

package com.android.setupwizardlib.test;

import android.content.Context;
import android.test.InstrumentationTestCase;
import android.test.suitebuilder.annotation.SmallTest;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.TextView;

import com.android.setupwizardlib.TemplateLayout;

public class TemplateLayoutTest extends InstrumentationTestCase {

    private Context mContext;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mContext = getInstrumentation().getContext();
    }

    @SmallTest
    public void testAddView() {
        TemplateLayout layout = new TemplateLayout(mContext, R.layout.test_template,
                R.id.suw_layout_content);
        TextView tv = new TextView(mContext);
        tv.setId(R.id.test_view_id);
        layout.addView(tv);
        View view = layout.findViewById(R.id.test_view_id);
        assertSame("The view added should be the same text view", tv, view);
    }

    @SmallTest
    public void testInflateFromXml() {
        LayoutInflater inflater = LayoutInflater.from(mContext);
        TemplateLayout layout =
                (TemplateLayout) inflater.inflate(R.layout.test_template_layout, null);
        View content = layout.findViewById(R.id.test_content);
        assertTrue("@id/test_content should be a TextView", content instanceof TextView);
    }

    @SmallTest
    public void testTemplate() {
        TemplateLayout layout = new TemplateLayout(mContext, R.layout.test_template,
                R.id.suw_layout_content);
        View templateView = layout.findViewById(R.id.test_template_view);
        assertNotNull("@id/test_template_view should exist in template", templateView);

        TextView tv = new TextView(mContext);
        tv.setId(R.id.test_view_id);
        layout.addView(tv);

        templateView = layout.findViewById(R.id.test_template_view);
        assertNotNull("@id/test_template_view should exist in template", templateView);
        View contentView = layout.findViewById(R.id.test_view_id);
        assertSame("The view added should be the same text view", tv, contentView);
    }

    @SmallTest
    public void testNoTemplate() {
        try {
            new TemplateLayout(mContext, 0, 0);
            fail("Inflating TemplateLayout without template should throw exception");
        } catch (IllegalArgumentException e) {
            // Expected IllegalArgumentException
        }
    }
}
