/*
 * Copyright (C) 2016 The Android Open Source Project
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
import android.content.ContextWrapper;
import android.test.AndroidTestCase;
import android.test.suitebuilder.annotation.SmallTest;
import android.widget.TextView;

import com.android.setupwizardlib.span.LinkSpan;

public class LinkSpanTest extends AndroidTestCase {

    @SmallTest
    public void testOnClick() {
        final TestContext context = new TestContext(getContext());
        final TextView textView = new TextView(context);
        final LinkSpan linkSpan = new LinkSpan("test_id");

        linkSpan.onClick(textView);

        assertSame("Clicked LinkSpan should be passed to setup", linkSpan, context.clickedSpan);
    }

    @SmallTest
    public void testNonImplementingContext() {
        final Context context = getContext();
        final TextView textView = new TextView(context);
        final LinkSpan linkSpan = new LinkSpan("test_id");

        linkSpan.onClick(textView);

        // This would be no-op, because the context doesn't implement LinkSpan.OnClickListener.
        // Just check that no uncaught exception here.
    }

    private static class TestContext extends ContextWrapper implements LinkSpan.OnClickListener {

        public LinkSpan clickedSpan = null;

        public TestContext(Context base) {
            super(base);
        }

        @Override
        public void onClick(LinkSpan span) {
            clickedSpan = span;
        }
    }
}
