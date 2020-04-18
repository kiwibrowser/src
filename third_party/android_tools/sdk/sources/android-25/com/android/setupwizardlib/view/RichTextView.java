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

package com.android.setupwizardlib.view;

import android.content.Context;
import android.support.v4.view.ViewCompat;
import android.text.Annotation;
import android.text.SpannableString;
import android.text.Spanned;
import android.text.method.LinkMovementMethod;
import android.text.style.ClickableSpan;
import android.text.style.TextAppearanceSpan;
import android.util.AttributeSet;
import android.util.Log;
import android.view.MotionEvent;
import android.widget.TextView;

import com.android.setupwizardlib.span.LinkSpan;
import com.android.setupwizardlib.span.SpanHelper;
import com.android.setupwizardlib.util.LinkAccessibilityHelper;

/**
 * An extension of TextView that automatically replaces the annotation tags as specified in
 * {@link SpanHelper#replaceSpan(android.text.Spannable, Object, Object)}
 */
public class RichTextView extends TextView {

    /* static section */

    private static final String TAG = "RichTextView";

    private static final String ANNOTATION_LINK = "link";
    private static final String ANNOTATION_TEXT_APPEARANCE = "textAppearance";

    /**
     * Replace &lt;annotation&gt; tags in strings to become their respective types. Currently 2
     * types are supported:
     * <ol>
     *     <li>&lt;annotation link="foobar"&gt; will create a
     *     {@link com.android.setupwizardlib.span.LinkSpan} that broadcasts with the key
     *     "foobar"</li>
     *     <li>&lt;annotation textAppearance="TextAppearance.FooBar"&gt; will create a
     *     {@link android.text.style.TextAppearanceSpan} with @style/TextAppearance.FooBar</li>
     * </ol>
     */
    public static CharSequence getRichText(Context context, CharSequence text) {
        if (text instanceof Spanned) {
            final SpannableString spannable = new SpannableString(text);
            final Annotation[] spans = spannable.getSpans(0, spannable.length(), Annotation.class);
            for (Annotation span : spans) {
                final String key = span.getKey();
                if (ANNOTATION_TEXT_APPEARANCE.equals(key)) {
                    String textAppearance = span.getValue();
                    final int style = context.getResources()
                            .getIdentifier(textAppearance, "style", context.getPackageName());
                    if (style == 0) {
                        Log.w(TAG, "Cannot find resource: " + style);
                    }
                    final TextAppearanceSpan textAppearanceSpan =
                            new TextAppearanceSpan(context, style);
                    SpanHelper.replaceSpan(spannable, span, textAppearanceSpan);
                } else if (ANNOTATION_LINK.equals(key)) {
                    LinkSpan link = new LinkSpan(span.getValue());
                    SpanHelper.replaceSpan(spannable, span, link);
                }
            }
            return spannable;
        }
        return text;
    }

    /* non-static section */

    private LinkAccessibilityHelper mAccessibilityHelper;

    public RichTextView(Context context) {
        super(context);
        init();
    }

    public RichTextView(Context context, AttributeSet attrs) {
        super(context, attrs);
        init();
    }

    private void init() {
        mAccessibilityHelper = new LinkAccessibilityHelper(this);
        ViewCompat.setAccessibilityDelegate(this, mAccessibilityHelper);
    }

    @Override
    public void setText(CharSequence text, BufferType type) {
        text = getRichText(getContext(), text);
        // Set text first before doing anything else because setMovementMethod internally calls
        // setText. This in turn ends up calling this method with mText as the first parameter
        super.setText(text, type);
        boolean hasLinks = hasLinks(text);

        if (hasLinks) {
            // When a TextView has a movement method, it will set the view to clickable. This makes
            // View.onTouchEvent always return true and consumes the touch event, essentially
            // nullifying any return values of MovementMethod.onTouchEvent.
            // To still allow propagating touch events to the parent when this view doesn't have
            // links, we only set the movement method here if the text contains links.
            setMovementMethod(LinkMovementMethod.getInstance());
        } else {
            setMovementMethod(null);
        }
        // ExploreByTouchHelper automatically enables focus for RichTextView
        // even though it may not have any links. Causes problems during talkback
        // as individual TextViews consume touch events and thereby reducing the focus window
        // shown by Talkback. Disable focus if there are no links
        setFocusable(hasLinks);
    }

    private boolean hasLinks(CharSequence text) {
        if (text instanceof Spanned) {
            final ClickableSpan[] spans =
                    ((Spanned) text).getSpans(0, text.length(), ClickableSpan.class);
            return spans.length > 0;
        }
        return false;
    }

    @Override
    protected boolean dispatchHoverEvent(MotionEvent event) {
        if (mAccessibilityHelper != null && mAccessibilityHelper.dispatchHoverEvent(event)) {
            return true;
        }
        return super.dispatchHoverEvent(event);
    }
}
