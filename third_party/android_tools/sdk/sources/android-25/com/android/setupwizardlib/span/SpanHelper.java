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

package com.android.setupwizardlib.span;

import android.text.Spannable;

/**
 * Contains helper methods for dealing with text spans, e.g. the ones in {@code android.text.style}.
 */
public class SpanHelper {

    /**
     * Add {@code newSpan} at the same start and end indices as {@code oldSpan} and remove
     * {@code oldSpan} from the {@code spannable}.
     */
    public static void replaceSpan(Spannable spannable, Object oldSpan, Object newSpan) {
        final int spanStart = spannable.getSpanStart(oldSpan);
        final int spanEnd = spannable.getSpanEnd(oldSpan);
        spannable.removeSpan(oldSpan);
        spannable.setSpan(newSpan, spanStart, spanEnd, 0);
    }
}
