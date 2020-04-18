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

package android.databinding.tool.util;

import com.google.common.base.StandardSystemProperty;
import com.google.common.base.Strings;

import org.antlr.v4.runtime.misc.Nullable;

public class StringUtils {

    public static final String LINE_SEPARATOR = StandardSystemProperty.LINE_SEPARATOR.value();
    /** The entity for the ampersand character */
    private static final String AMP_ENTITY = "&amp;";
    /** The entity for the quote character */
    private static final String QUOT_ENTITY = "&quot;";
    /** The entity for the apostrophe character */
    private static final String APOS_ENTITY = "&apos;";
    /** The entity for the less than character */
    private static final String LT_ENTITY = "&lt;";
    /** The entity for the greater than character */
    private static final String GT_ENTITY = "&gt;";
    /** The entity for the tab character */
    private static final String TAB_ENTITY = "&#x9;";
    /** The entity for the carriage return character */
    private static final String CR_ENTITY = "&#xD;";
    /** The entity for the line feed character */
    private static final String LFEED_ENTITY = "&#xA;";

    public static boolean isNotBlank(@Nullable CharSequence string) {
        if (string == null) {
            return false;
        }
        for (int i = 0, n = string.length(); i < n; i++) {
            if (!Character.isWhitespace(string.charAt(i))) {
                return true;
            }
        }
        return false;
    }

    public static String capitalize(@Nullable String string) {
        if (Strings.isNullOrEmpty(string)) {
            return string;
        }
        char ch = string.charAt(0);
        if (Character.isTitleCase(ch)) {
            return string;
        }
        return Character.toTitleCase(ch) + string.substring(1);
    }

    public static String unescapeXml(String escaped) {
        // TODO: unescape unicode codepoints
        return escaped.replace(QUOT_ENTITY, "\"")
                .replace(LT_ENTITY, "<")
                .replace(GT_ENTITY, ">")
                .replace(APOS_ENTITY, "'")
                .replace(AMP_ENTITY, "&")
                .replace(TAB_ENTITY, "\t")
                .replace(CR_ENTITY, "\r")
                .replace(LFEED_ENTITY, "\n");
    }

    private StringUtils() {
    }

}
