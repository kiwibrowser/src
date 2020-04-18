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

import android.test.AndroidTestCase;
import android.test.suitebuilder.annotation.SmallTest;
import android.text.Annotation;
import android.text.SpannableStringBuilder;
import android.text.Spanned;
import android.text.style.TextAppearanceSpan;

import com.android.setupwizardlib.span.LinkSpan;
import com.android.setupwizardlib.view.RichTextView;

import java.util.Arrays;

public class RichTextViewTest extends AndroidTestCase {

    @SmallTest
    public void testLinkAnnotation() {
        Annotation link = new Annotation("link", "foobar");
        SpannableStringBuilder ssb = new SpannableStringBuilder("Hello world");
        ssb.setSpan(link, 1, 2, 0 /* flags */);

        RichTextView textView = new RichTextView(getContext());
        textView.setText(ssb);

        final CharSequence text = textView.getText();
        assertTrue("Text should be spanned", text instanceof Spanned);

        Object[] spans = ((Spanned) text).getSpans(0, text.length(), Annotation.class);
        assertEquals("Annotation should be removed " + Arrays.toString(spans), 0, spans.length);

        spans = ((Spanned) text).getSpans(0, text.length(), LinkSpan.class);
        assertEquals("There should be one span " + Arrays.toString(spans), 1, spans.length);
        assertTrue("The span should be a LinkSpan", spans[0] instanceof LinkSpan);
        assertEquals("The LinkSpan should have id \"foobar\"",
                "foobar", ((LinkSpan) spans[0]).getId());
    }

    @SmallTest
    public void testTextStyle() {
        Annotation link = new Annotation("textAppearance", "foobar");
        SpannableStringBuilder ssb = new SpannableStringBuilder("Hello world");
        ssb.setSpan(link, 1, 2, 0 /* flags */);

        RichTextView textView = new RichTextView(getContext());
        textView.setText(ssb);

        final CharSequence text = textView.getText();
        assertTrue("Text should be spanned", text instanceof Spanned);

        Object[] spans = ((Spanned) text).getSpans(0, text.length(), Annotation.class);
        assertEquals("Annotation should be removed " + Arrays.toString(spans), 0, spans.length);

        spans = ((Spanned) text).getSpans(0, text.length(), TextAppearanceSpan.class);
        assertEquals("There should be one span " + Arrays.toString(spans), 1, spans.length);
        assertTrue("The span should be a TextAppearanceSpan",
                spans[0] instanceof TextAppearanceSpan);
    }

    @SmallTest
    public void testTextContaininingLinksAreFocusable() {
        Annotation testLink = new Annotation("link", "value");
        SpannableStringBuilder spannableStringBuilder = new SpannableStringBuilder("Linked");
        spannableStringBuilder.setSpan(testLink, 0, 3, 0);

        RichTextView view = new RichTextView(getContext());
        view.setText(spannableStringBuilder);

        assertTrue("TextView should be focusable since it contains spans", view.isFocusable());
    }


    @SmallTest
    public void testTextContainingNoLinksAreNotFocusable() {
        RichTextView textView = new RichTextView(getContext());
        textView.setText("Thou shall not be focusable!");

        assertFalse("TextView should not be focusable since it does not contain any span",
                textView.isFocusable());
    }


    // Based on the text contents of the text view, the "focusable" property of the element
    // should also be automatically changed.
    @SmallTest
    public void testRichTxtViewFocusChangesWithTextChange() {
        RichTextView textView = new RichTextView(getContext());
        textView.setText("Thou shall not be focusable!");

        assertFalse(textView.isFocusable());

        SpannableStringBuilder spannableStringBuilder =
                new SpannableStringBuilder("I am focusable");
        spannableStringBuilder.setSpan(new Annotation("link", "focus:on_me"), 0, 1, 0);
        textView.setText(spannableStringBuilder);
        assertTrue(textView.isFocusable());
    }
}
