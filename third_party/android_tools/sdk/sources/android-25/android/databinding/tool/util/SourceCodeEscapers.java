/*
 * Copyright (C) 2009 The Guava Authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package android.databinding.tool.util;

import com.google.common.escape.ArrayBasedCharEscaper;
import com.google.common.escape.CharEscaper;
import com.google.common.escape.Escaper;

import java.util.HashMap;
import java.util.Map;

/**
 * This file has been copied from the google internal implementation of guava. Some unused parts of
 * the file have been removed.
 */

/**
 * A factory for Escaper instances used to escape strings for safe use in
 * various common programming languages.
 *
 * @author Alex Matevossian
 * @author David Beaumont
 */
public final class SourceCodeEscapers {
    private SourceCodeEscapers() {}

    // For each xxxEscaper() method, please add links to external reference pages
    // that are considered authoritative for the behavior of that escaper.

    // From: http://en.wikipedia.org/wiki/ASCII#ASCII_printable_characters
    private static final char PRINTABLE_ASCII_MIN = 0x20;  // ' '
    private static final char PRINTABLE_ASCII_MAX = 0x7E;  // '~'

    private static final char[] HEX_DIGITS = "0123456789abcdef".toCharArray();

    /**
     * Returns an {@link Escaper} instance that escapes special characters in a
     * string so it can safely be included in either a Java character literal or
     * string literal. This is the preferred way to escape Java characters for
     * use in String or character literals.
     *
     * <p>For more details, see <a href="http://goo.gl/NsGW7">Escape Sequences for
     * Character and String Literals</a> in The Java Language Specification.
     */
    public static Escaper javaCharEscaper() {
        return JAVA_CHAR_ESCAPER;
    }

    /**
     * Returns an {@link Escaper} instance that escapes special characters in a
     * string so it can safely be included in either a Java character literal or
     * string literal. The behavior of this escaper is the same as that of the
     * {@link #javaStringEscaperWithOctal()} except it also escapes single quotes.
     *
     * <p>Unlike {@link #javaCharEscaper} this escaper produces octal escape
     * sequences ({@literal \}nnn) for characters with values less than 256. While
     * the escaped output can be shorter than when the standard Unicode escape
     * sequence ({@literal \}uxxxx) is used, the Java Language Specification
     * discourages the use of octal for escaping Java strings. It is strongly
     * recommended that, if possible, you use {@code javaCharEscaper()} in
     * preference to this method.
     *
     * <p>For more details, see <a href="http://goo.gl/NsGW7">Escape Sequences for
     * Character and String Literals</a> in The Java Language Specification.
     */
    public static Escaper javaCharEscaperWithOctal() {
        return JAVA_CHAR_ESCAPER_WITH_OCTAL;
    }

    /**
     * Returns an {@link Escaper} instance that escapes special characters in a
     * string so it can safely be included in a Java string literal.
     *
     * <p><b>Note:</b> Single quotes are not escaped, so it is <b>not safe</b> to
     * use this escaper for escaping character literals.
     *
     * <p>Unlike {@link #javaCharEscaper} this escaper produces octal escape
     * sequences ({@literal \}nnn) for characters with values less than 256. While
     * the escaped output can be shorter than when the standard Unicode escape
     * sequence ({@literal \}uxxxx) is used, the Java Language Specification
     * discourages the use of octal for escaping Java strings. It is strongly
     * recommended that, if possible, you use {@code javaCharEscaper()} in
     * preference to this method.
     *
     * <p>For more details, see <a href="http://goo.gl/NsGW7">Escape Sequences for
     * Character and String Literals</a> in The Java Language Specification.
     */
    public static Escaper javaStringEscaperWithOctal() {
        return JAVA_STRING_ESCAPER_WITH_OCTAL;
    }

    private static final Escaper JAVA_CHAR_ESCAPER;
    private static final Escaper JAVA_CHAR_ESCAPER_WITH_OCTAL;
    private static final Escaper JAVA_STRING_ESCAPER_WITH_OCTAL;
    static {
        Map<Character, String> javaMap = new HashMap<Character, String>();
        javaMap.put('\b', "\\b");
        javaMap.put('\f', "\\f");
        javaMap.put('\n', "\\n");
        javaMap.put('\r', "\\r");
        javaMap.put('\t', "\\t");
        javaMap.put('\"', "\\\"");
        javaMap.put('\\', "\\\\");
        JAVA_STRING_ESCAPER_WITH_OCTAL = new JavaCharEscaperWithOctal(javaMap);
        // The only difference is that the char escaper also escapes single quotes.
        javaMap.put('\'', "\\'");
        JAVA_CHAR_ESCAPER = new JavaCharEscaper(javaMap);
        JAVA_CHAR_ESCAPER_WITH_OCTAL = new JavaCharEscaperWithOctal(javaMap);
    }

    // This escaper does not produce octal escape sequences. See:
    // http://goo.gl/NsGW7
    //  "Octal escapes are provided for compatibility with C, but can express
    //   only Unicode values \u0000 through \u00FF, so Unicode escapes are
    //   usually preferred."
    private static class JavaCharEscaper extends ArrayBasedCharEscaper {
        JavaCharEscaper(Map<Character, String> replacements) {
            super(replacements, PRINTABLE_ASCII_MIN, PRINTABLE_ASCII_MAX);
        }

        @Override protected char[] escapeUnsafe(char c) {
            return asUnicodeHexEscape(c);
        }
    }

    private static class JavaCharEscaperWithOctal extends ArrayBasedCharEscaper {
        JavaCharEscaperWithOctal(Map<Character, String> replacements) {
            super(replacements, PRINTABLE_ASCII_MIN, PRINTABLE_ASCII_MAX);
        }

        @Override protected char[] escapeUnsafe(char c) {
            if (c < 0x100) {
                return asOctalEscape(c);
            } else {
                return asUnicodeHexEscape(c);
            }
        }
    }

    /**
     * Returns an {@link Escaper} instance that replaces non-ASCII characters
     * in a string with their Unicode escape sequences ({@code \\uxxxx} where
     * {@code xxxx} is a hex number). Existing escape sequences won't be affected.
     *
     * <p>As existing escape sequences are not re-escaped, this escaper is
     * idempotent. However this means that there can be no well defined inverse
     * function for this escaper.
     *
     * <p><b>Note:</b> the returned escaper is still a {@code CharEscaper} and
     * will not combine surrogate pairs into a single code point before escaping.
     */
    public static Escaper javaStringUnicodeEscaper() {
        return JAVA_STRING_UNICODE_ESCAPER;
    }

    private static final Escaper JAVA_STRING_UNICODE_ESCAPER
            = new CharEscaper() {
        @Override protected char[] escape(char c) {
            if (c < 0x80) {
                return null;
            }
            return asUnicodeHexEscape(c);
        }
    };

    // Helper for common case of escaping a single char.
    private static char[] asUnicodeHexEscape(char c) {
        // Equivalent to String.format("\\u%04x", (int) c);
        char[] r = new char[6];
        r[0] = '\\';
        r[1] = 'u';
        r[5] = HEX_DIGITS[c & 0xF];
        c >>>= 4;
        r[4] = HEX_DIGITS[c & 0xF];
        c >>>= 4;
        r[3] = HEX_DIGITS[c & 0xF];
        c >>>= 4;
        r[2] = HEX_DIGITS[c & 0xF];
        return r;
    }

    // Helper for backward compatible octal escape sequences (c < 256)
    private static char[] asOctalEscape(char c) {
        char[] r = new char[4];
        r[0] = '\\';
        r[3] = HEX_DIGITS[c & 0x7];
        c >>>= 3;
        r[2] = HEX_DIGITS[c & 0x7];
        c >>>= 3;
        r[1] = HEX_DIGITS[c & 0x3];
        return r;
    }
}
