/*
 * Copyright (C) 2014 The Android Open Source Project
 * Copyright (c) 2005, Oracle and/or its affiliates. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.  Oracle designates this
 * particular file as subject to the "Classpath" exception as provided
 * by Oracle in the LICENSE file that accompanied this code.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * version 2 for more details (a copy is included in the LICENSE file that
 * accompanied this code).
 *
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Please contact Oracle, 500 Oracle Parkway, Redwood Shores, CA 94065 USA
 * or visit www.oracle.com if you need additional information or have any
 * questions.
 */
package java.net;

import android.icu.text.IDNA;

/**
 * Provides methods to convert internationalized domain names (IDNs) between
 * a normal Unicode representation and an ASCII Compatible Encoding (ACE) representation.
 * Internationalized domain names can use characters from the entire range of
 * Unicode, while traditional domain names are restricted to ASCII characters.
 * ACE is an encoding of Unicode strings that uses only ASCII characters and
 * can be used with software (such as the Domain Name System) that only
 * understands traditional domain names.
 *
 * <p>Internationalized domain names are defined in <a href="http://www.ietf.org/rfc/rfc3490.txt">RFC 3490</a>.
 * RFC 3490 defines two operations: ToASCII and ToUnicode. These 2 operations employ
 * <a href="http://www.ietf.org/rfc/rfc3491.txt">Nameprep</a> algorithm, which is a
 * profile of <a href="http://www.ietf.org/rfc/rfc3454.txt">Stringprep</a>, and
 * <a href="http://www.ietf.org/rfc/rfc3492.txt">Punycode</a> algorithm to convert
 * domain name string back and forth.
 *
 * <p>The behavior of aforementioned conversion process can be adjusted by various flags:
 *   <ul>
 *     <li>If the ALLOW_UNASSIGNED flag is used, the domain name string to be converted
 *         can contain code points that are unassigned in Unicode 3.2, which is the
 *         Unicode version on which IDN conversion is based. If the flag is not used,
 *         the presence of such unassigned code points is treated as an error.
 *     <li>If the USE_STD3_ASCII_RULES flag is used, ASCII strings are checked against <a href="http://www.ietf.org/rfc/rfc1122.txt">RFC 1122</a> and <a href="http://www.ietf.org/rfc/rfc1123.txt">RFC 1123</a>.
 *         It is an error if they don't meet the requirements.
 *   </ul>
 * These flags can be logically OR'ed together.
 *
 * <p>The security consideration is important with respect to internationalization
 * domain name support. For example, English domain names may be <i>homographed</i>
 * - maliciously misspelled by substitution of non-Latin letters.
 * <a href="http://www.unicode.org/reports/tr36/">Unicode Technical Report #36</a>
 * discusses security issues of IDN support as well as possible solutions.
 * Applications are responsible for taking adequate security measures when using
 * international domain names.
 *
 * @author Edward Wang
 * @since 1.6
 *
 */
public final class IDN {
    /**
     * Flag to allow processing of unassigned code points
     */
    public static final int ALLOW_UNASSIGNED = 0x01;

    /**
     * Flag to turn on the check against STD-3 ASCII rules
     */
    public static final int USE_STD3_ASCII_RULES = 0x02;

    private IDN() {
    }


    /**
     * Translates a string from Unicode to ASCII Compatible Encoding (ACE),
     * as defined by the ToASCII operation of <a href="http://www.ietf.org/rfc/rfc3490.txt">RFC 3490</a>.
     *
     * <p>ToASCII operation can fail. ToASCII fails if any step of it fails.
     * If ToASCII operation fails, an IllegalArgumentException will be thrown.
     * In this case, the input string should not be used in an internationalized domain name.
     *
     * <p> A label is an individual part of a domain name. The original ToASCII operation,
     * as defined in RFC 3490, only operates on a single label. This method can handle
     * both label and entire domain name, by assuming that labels in a domain name are
     * always separated by dots. The following characters are recognized as dots:
     * &#0092;u002E (full stop), &#0092;u3002 (ideographic full stop), &#0092;uFF0E (fullwidth full stop),
     * and &#0092;uFF61 (halfwidth ideographic full stop). if dots are
     * used as label separators, this method also changes all of them to &#0092;u002E (full stop)
     * in output translated string.
     *
     * @param input     the string to be processed
     * @param flag      process flag; can be 0 or any logical OR of possible flags
     *
     * @return the translated <tt>String</tt>
     *
     * @throws IllegalArgumentException   if the input string doesn't conform to RFC 3490 specification
     */
    public static String toASCII(String input, int flag) {
        try {
            return IDNA.convertIDNToASCII(input, flag).toString();
        } catch (android.icu.text.StringPrepParseException e) {
            throw new IllegalArgumentException("Invalid input to toASCII: " + input, e);
        }
    }


    /**
     * Translates a string from Unicode to ASCII Compatible Encoding (ACE),
     * as defined by the ToASCII operation of <a href="http://www.ietf.org/rfc/rfc3490.txt">RFC 3490</a>.
     *
     * <p> This convenience method works as if by invoking the
     * two-argument counterpart as follows:
     * <blockquote><tt>
     * {@link #toASCII(String, int) toASCII}(input,&nbsp;0);
     * </tt></blockquote>
     *
     * @param input     the string to be processed
     *
     * @return the translated <tt>String</tt>
     *
     * @throws IllegalArgumentException   if the input string doesn't conform to RFC 3490 specification
     */
    public static String toASCII(String input) {
        return toASCII(input, 0);
    }


    /**
     * Translates a string from ASCII Compatible Encoding (ACE) to Unicode,
     * as defined by the ToUnicode operation of <a href="http://www.ietf.org/rfc/rfc3490.txt">RFC 3490</a>.
     *
     * <p>ToUnicode never fails. In case of any error, the input string is returned unmodified.
     *
     * <p> A label is an individual part of a domain name. The original ToUnicode operation,
     * as defined in RFC 3490, only operates on a single label. This method can handle
     * both label and entire domain name, by assuming that labels in a domain name are
     * always separated by dots. The following characters are recognized as dots:
     * &#0092;u002E (full stop), &#0092;u3002 (ideographic full stop), &#0092;uFF0E (fullwidth full stop),
     * and &#0092;uFF61 (halfwidth ideographic full stop).
     *
     * @param input     the string to be processed
     * @param flag      process flag; can be 0 or any logical OR of possible flags
     *
     * @return the translated <tt>String</tt>
     */
    public static String toUnicode(String input, int flag) {
        try {
            // ICU only translates separators to ASCII for toASCII.
            // Java expects the translation for toUnicode too.
            return convertFullStop(IDNA.convertIDNToUnicode(input, flag)).toString();
        } catch (android.icu.text.StringPrepParseException e) {
            // The RI documentation explicitly states that if the conversion was unsuccessful
            // the original string is returned.
            return input;
        }
    }

    private static boolean isLabelSeperator(char c) {
        return (c == '\u3002' || c == '\uff0e' || c == '\uff61');
    }

    private static StringBuffer convertFullStop(StringBuffer input) {
        for (int i = 0; i < input.length(); i++) {
            if (isLabelSeperator(input.charAt(i))) {
                input.setCharAt(i, '.');
            }
        }
        return input;
    }


    /**
     * Translates a string from ASCII Compatible Encoding (ACE) to Unicode,
     * as defined by the ToUnicode operation of <a href="http://www.ietf.org/rfc/rfc3490.txt">RFC 3490</a>.
     *
     * <p> This convenience method works as if by invoking the
     * two-argument counterpart as follows:
     * <blockquote><tt>
     * {@link #toUnicode(String, int) toUnicode}(input,&nbsp;0);
     * </tt></blockquote>
     *
     * @param input     the string to be processed
     *
     * @return the translated <tt>String</tt>
     */
    public static String toUnicode(String input) {
        return toUnicode(input, 0);
    }
}
