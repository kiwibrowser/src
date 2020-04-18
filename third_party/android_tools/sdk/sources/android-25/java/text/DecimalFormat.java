/*
 * Copyright (C) 2014 The Android Open Source Project
 * Copyright (c) 1996, 2010, Oracle and/or its affiliates. All rights reserved.
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

/*
 * (C) Copyright Taligent, Inc. 1996, 1997 - All Rights Reserved
 * (C) Copyright IBM Corp. 1996 - 1998 - All Rights Reserved
 *
 *   The original version of this source code and documentation is copyrighted
 * and owned by Taligent, Inc., a wholly-owned subsidiary of IBM. These
 * materials are provided under terms of a License Agreement between Taligent
 * and Sun. This technology is protected by multiple US and International
 * patents. This notice and attribution to Taligent may not be removed.
 *   Taligent is a registered trademark of Taligent, Inc.
 *
 */

package java.text;

import java.io.IOException;
import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;
import java.io.ObjectStreamField;
import java.math.BigDecimal;
import java.math.BigInteger;
import java.math.RoundingMode;
import java.util.Currency;
import java.util.Locale;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ConcurrentMap;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.atomic.AtomicLong;
import libcore.icu.LocaleData;

import android.icu.math.MathContext;

/**
 * <code>DecimalFormat</code> is a concrete subclass of
 * <code>NumberFormat</code> that formats decimal numbers. It has a variety of
 * features designed to make it possible to parse and format numbers in any
 * locale, including support for Western, Arabic, and Indic digits.  It also
 * supports different kinds of numbers, including integers (123), fixed-point
 * numbers (123.4), scientific notation (1.23E4), percentages (12%), and
 * currency amounts ($123).  All of these can be localized.
 *
 * <p>To obtain a <code>NumberFormat</code> for a specific locale, including the
 * default locale, call one of <code>NumberFormat</code>'s factory methods, such
 * as <code>getInstance()</code>.  In general, do not call the
 * <code>DecimalFormat</code> constructors directly, since the
 * <code>NumberFormat</code> factory methods may return subclasses other than
 * <code>DecimalFormat</code>. If you need to customize the format object, do
 * something like this:
 *
 * <blockquote><pre>
 * NumberFormat f = NumberFormat.getInstance(loc);
 * if (f instanceof DecimalFormat) {
 *     ((DecimalFormat) f).setDecimalSeparatorAlwaysShown(true);
 * }
 * </pre></blockquote>
 *
 * <p>A <code>DecimalFormat</code> comprises a <em>pattern</em> and a set of
 * <em>symbols</em>.  The pattern may be set directly using
 * <code>applyPattern()</code>, or indirectly using the API methods.  The
 * symbols are stored in a <code>DecimalFormatSymbols</code> object.  When using
 * the <code>NumberFormat</code> factory methods, the pattern and symbols are
 * read from localized <code>ResourceBundle</code>s.
 *
 * <h4>Patterns</h4>
 *
 * <code>DecimalFormat</code> patterns have the following syntax:
 * <blockquote><pre>
 * <i>Pattern:</i>
 *         <i>PositivePattern</i>
 *         <i>PositivePattern</i> ; <i>NegativePattern</i>
 * <i>PositivePattern:</i>
 *         <i>Prefix<sub>opt</sub></i> <i>Number</i> <i>Suffix<sub>opt</sub></i>
 * <i>NegativePattern:</i>
 *         <i>Prefix<sub>opt</sub></i> <i>Number</i> <i>Suffix<sub>opt</sub></i>
 * <i>Prefix:</i>
 *         any Unicode characters except &#92;uFFFE, &#92;uFFFF, and special characters
 * <i>Suffix:</i>
 *         any Unicode characters except &#92;uFFFE, &#92;uFFFF, and special characters
 * <i>Number:</i>
 *         <i>Integer</i> <i>Exponent<sub>opt</sub></i>
 *         <i>Integer</i> . <i>Fraction</i> <i>Exponent<sub>opt</sub></i>
 * <i>Integer:</i>
 *         <i>MinimumInteger</i>
 *         #
 *         # <i>Integer</i>
 *         # , <i>Integer</i>
 * <i>MinimumInteger:</i>
 *         0
 *         0 <i>MinimumInteger</i>
 *         0 , <i>MinimumInteger</i>
 * <i>Fraction:</i>
 *         <i>MinimumFraction<sub>opt</sub></i> <i>OptionalFraction<sub>opt</sub></i>
 * <i>MinimumFraction:</i>
 *         0 <i>MinimumFraction<sub>opt</sub></i>
 * <i>OptionalFraction:</i>
 *         # <i>OptionalFraction<sub>opt</sub></i>
 * <i>Exponent:</i>
 *         E <i>MinimumExponent</i>
 * <i>MinimumExponent:</i>
 *         0 <i>MinimumExponent<sub>opt</sub></i>
 * </pre></blockquote>
 *
 * <p>A <code>DecimalFormat</code> pattern contains a positive and negative
 * subpattern, for example, <code>"#,##0.00;(#,##0.00)"</code>.  Each
 * subpattern has a prefix, numeric part, and suffix. The negative subpattern
 * is optional; if absent, then the positive subpattern prefixed with the
 * localized minus sign (<code>'-'</code> in most locales) is used as the
 * negative subpattern. That is, <code>"0.00"</code> alone is equivalent to
 * <code>"0.00;-0.00"</code>.  If there is an explicit negative subpattern, it
 * serves only to specify the negative prefix and suffix; the number of digits,
 * minimal digits, and other characteristics are all the same as the positive
 * pattern. That means that <code>"#,##0.0#;(#)"</code> produces precisely
 * the same behavior as <code>"#,##0.0#;(#,##0.0#)"</code>.
 *
 * <p>The prefixes, suffixes, and various symbols used for infinity, digits,
 * thousands separators, decimal separators, etc. may be set to arbitrary
 * values, and they will appear properly during formatting.  However, care must
 * be taken that the symbols and strings do not conflict, or parsing will be
 * unreliable.  For example, either the positive and negative prefixes or the
 * suffixes must be distinct for <code>DecimalFormat.parse()</code> to be able
 * to distinguish positive from negative values.  (If they are identical, then
 * <code>DecimalFormat</code> will behave as if no negative subpattern was
 * specified.)  Another example is that the decimal separator and thousands
 * separator should be distinct characters, or parsing will be impossible.
 *
 * <p>The grouping separator is commonly used for thousands, but in some
 * countries it separates ten-thousands. The grouping size is a constant number
 * of digits between the grouping characters, such as 3 for 100,000,000 or 4 for
 * 1,0000,0000.  If you supply a pattern with multiple grouping characters, the
 * interval between the last one and the end of the integer is the one that is
 * used. So <code>"#,##,###,####"</code> == <code>"######,####"</code> ==
 * <code>"##,####,####"</code>.
 *
 * <h4>Special Pattern Characters</h4>
 *
 * <p>Many characters in a pattern are taken literally; they are matched during
 * parsing and output unchanged during formatting.  Special characters, on the
 * other hand, stand for other characters, strings, or classes of characters.
 * They must be quoted, unless noted otherwise, if they are to appear in the
 * prefix or suffix as literals.
 *
 * <p>The characters listed here are used in non-localized patterns.  Localized
 * patterns use the corresponding characters taken from this formatter's
 * <code>DecimalFormatSymbols</code> object instead, and these characters lose
 * their special status.  Two exceptions are the currency sign and quote, which
 * are not localized.
 *
 * <blockquote>
 * <table border=0 cellspacing=3 cellpadding=0 summary="Chart showing symbol,
 *  location, localized, and meaning.">
 *     <tr bgcolor="#ccccff">
 *          <th align=left>Symbol
 *          <th align=left>Location
 *          <th align=left>Localized?
 *          <th align=left>Meaning
 *     <tr valign=top>
 *          <td><code>0</code>
 *          <td>Number
 *          <td>Yes
 *          <td>Digit
 *     <tr valign=top bgcolor="#eeeeff">
 *          <td><code>#</code>
 *          <td>Number
 *          <td>Yes
 *          <td>Digit, zero shows as absent
 *     <tr valign=top>
 *          <td><code>.</code>
 *          <td>Number
 *          <td>Yes
 *          <td>Decimal separator or monetary decimal separator
 *     <tr valign=top bgcolor="#eeeeff">
 *          <td><code>-</code>
 *          <td>Number
 *          <td>Yes
 *          <td>Minus sign
 *     <tr valign=top>
 *          <td><code>,</code>
 *          <td>Number
 *          <td>Yes
 *          <td>Grouping separator
 *     <tr valign=top bgcolor="#eeeeff">
 *          <td><code>E</code>
 *          <td>Number
 *          <td>Yes
 *          <td>Separates mantissa and exponent in scientific notation.
 *              <em>Need not be quoted in prefix or suffix.</em>
 *     <tr valign=top>
 *          <td><code>;</code>
 *          <td>Subpattern boundary
 *          <td>Yes
 *          <td>Separates positive and negative subpatterns
 *     <tr valign=top bgcolor="#eeeeff">
 *          <td><code>%</code>
 *          <td>Prefix or suffix
 *          <td>Yes
 *          <td>Multiply by 100 and show as percentage
 *     <tr valign=top>
 *          <td><code>&#92;u2030</code>
 *          <td>Prefix or suffix
 *          <td>Yes
 *          <td>Multiply by 1000 and show as per mille value
 *     <tr valign=top bgcolor="#eeeeff">
 *          <td><code>&#164;</code> (<code>&#92;u00A4</code>)
 *          <td>Prefix or suffix
 *          <td>No
 *          <td>Currency sign, replaced by currency symbol.  If
 *              doubled, replaced by international currency symbol.
 *              If present in a pattern, the monetary decimal separator
 *              is used instead of the decimal separator.
 *     <tr valign=top>
 *          <td><code>'</code>
 *          <td>Prefix or suffix
 *          <td>No
 *          <td>Used to quote special characters in a prefix or suffix,
 *              for example, <code>"'#'#"</code> formats 123 to
 *              <code>"#123"</code>.  To create a single quote
 *              itself, use two in a row: <code>"# o''clock"</code>.
 * </table>
 * </blockquote>
 *
 * <h4>Scientific Notation</h4>
 *
 * <p>Numbers in scientific notation are expressed as the product of a mantissa
 * and a power of ten, for example, 1234 can be expressed as 1.234 x 10^3.  The
 * mantissa is often in the range 1.0 <= x < 10.0, but it need not be.
 * <code>DecimalFormat</code> can be instructed to format and parse scientific
 * notation <em>only via a pattern</em>; there is currently no factory method
 * that creates a scientific notation format.  In a pattern, the exponent
 * character immediately followed by one or more digit characters indicates
 * scientific notation.  Example: <code>"0.###E0"</code> formats the number
 * 1234 as <code>"1.234E3"</code>.
 *
 * <ul>
 * <li>The number of digit characters after the exponent character gives the
 * minimum exponent digit count.  There is no maximum.  Negative exponents are
 * formatted using the localized minus sign, <em>not</em> the prefix and suffix
 * from the pattern.  This allows patterns such as <code>"0.###E0 m/s"</code>.
 *
 * <li>The minimum and maximum number of integer digits are interpreted
 * together:
 *
 * <ul>
 * <li>If the maximum number of integer digits is greater than their minimum number
 * and greater than 1, it forces the exponent to be a multiple of the maximum
 * number of integer digits, and the minimum number of integer digits to be
 * interpreted as 1.  The most common use of this is to generate
 * <em>engineering notation</em>, in which the exponent is a multiple of three,
 * e.g., <code>"##0.#####E0"</code>. Using this pattern, the number 12345
 * formats to <code>"12.345E3"</code>, and 123456 formats to
 * <code>"123.456E3"</code>.
 *
 * <li>Otherwise, the minimum number of integer digits is achieved by adjusting the
 * exponent.  Example: 0.00123 formatted with <code>"00.###E0"</code> yields
 * <code>"12.3E-4"</code>.
 * </ul>
 *
 * <li>The number of significant digits in the mantissa is the sum of the
 * <em>minimum integer</em> and <em>maximum fraction</em> digits, and is
 * unaffected by the maximum integer digits.  For example, 12345 formatted with
 * <code>"##0.##E0"</code> is <code>"12.3E3"</code>. To show all digits, set
 * the significant digits count to zero.  The number of significant digits
 * does not affect parsing.
 *
 * <li>Exponential patterns may not contain grouping separators.
 * </ul>
 *
 * <h4>Rounding</h4>
 *
 * <code>DecimalFormat</code> provides rounding modes defined in
 * {@link java.math.RoundingMode} for formatting.  By default, it uses
 * {@link java.math.RoundingMode#HALF_EVEN RoundingMode.HALF_EVEN}.
 *
 * <h4>Digits</h4>
 *
 * For formatting, <code>DecimalFormat</code> uses the ten consecutive
 * characters starting with the localized zero digit defined in the
 * <code>DecimalFormatSymbols</code> object as digits. For parsing, these
 * digits as well as all Unicode decimal digits, as defined by
 * {@link Character#digit Character.digit}, are recognized.
 *
 * <h4>Special Values</h4>
 *
 * <p><code>NaN</code> is formatted as a string, which typically has a single character
 * <code>&#92;uFFFD</code>.  This string is determined by the
 * <code>DecimalFormatSymbols</code> object.  This is the only value for which
 * the prefixes and suffixes are not used.
 *
 * <p>Infinity is formatted as a string, which typically has a single character
 * <code>&#92;u221E</code>, with the positive or negative prefixes and suffixes
 * applied.  The infinity string is determined by the
 * <code>DecimalFormatSymbols</code> object.
 *
 * <p>Negative zero (<code>"-0"</code>) parses to
 * <ul>
 * <li><code>BigDecimal(0)</code> if <code>isParseBigDecimal()</code> is
 * true,
 * <li><code>Long(0)</code> if <code>isParseBigDecimal()</code> is false
 *     and <code>isParseIntegerOnly()</code> is true,
 * <li><code>Double(-0.0)</code> if both <code>isParseBigDecimal()</code>
 * and <code>isParseIntegerOnly()</code> are false.
 * </ul>
 *
 * <h4><a name="synchronization">Synchronization</a></h4>
 *
 * <p>
 * Decimal formats are generally not synchronized.
 * It is recommended to create separate format instances for each thread.
 * If multiple threads access a format concurrently, it must be synchronized
 * externally.
 *
 * <h4>Example</h4>
 *
 * <blockquote><pre>
 * <strong>// Print out a number using the localized number, integer, currency,
 * // and percent format for each locale</strong>
 * Locale[] locales = NumberFormat.getAvailableLocales();
 * double myNumber = -1234.56;
 * NumberFormat form;
 * for (int j=0; j<4; ++j) {
 *     System.out.println("FORMAT");
 *     for (int i = 0; i < locales.length; ++i) {
 *         if (locales[i].getCountry().length() == 0) {
 *            continue; // Skip language-only locales
 *         }
 *         System.out.print(locales[i].getDisplayName());
 *         switch (j) {
 *         case 0:
 *             form = NumberFormat.getInstance(locales[i]); break;
 *         case 1:
 *             form = NumberFormat.getIntegerInstance(locales[i]); break;
 *         case 2:
 *             form = NumberFormat.getCurrencyInstance(locales[i]); break;
 *         default:
 *             form = NumberFormat.getPercentInstance(locales[i]); break;
 *         }
 *         if (form instanceof DecimalFormat) {
 *             System.out.print(": " + ((DecimalFormat) form).toPattern());
 *         }
 *         System.out.print(" -> " + form.format(myNumber));
 *         try {
 *             System.out.println(" -> " + form.parse(form.format(myNumber)));
 *         } catch (ParseException e) {}
 *     }
 * }
 * </pre></blockquote>
 *
 * @see          <a href="http://java.sun.com/docs/books/tutorial/i18n/format/decimalFormat.html">Java Tutorial</a>
 * @see          NumberFormat
 * @see          DecimalFormatSymbols
 * @see          ParsePosition
 * @author       Mark Davis
 * @author       Alan Liu
 */
public class DecimalFormat extends NumberFormat {

    private transient android.icu.text.DecimalFormat icuDecimalFormat;

    /**
     * Creates a DecimalFormat using the default pattern and symbols
     * for the default locale. This is a convenient way to obtain a
     * DecimalFormat when internationalization is not the main concern.
     * <p>
     * To obtain standard formats for a given locale, use the factory methods
     * on NumberFormat such as getNumberInstance. These factories will
     * return the most appropriate sub-class of NumberFormat for a given
     * locale.
     *
     * @see java.text.NumberFormat#getInstance
     * @see java.text.NumberFormat#getNumberInstance
     * @see java.text.NumberFormat#getCurrencyInstance
     * @see java.text.NumberFormat#getPercentInstance
     */
    public DecimalFormat() {
        Locale def = Locale.getDefault(Locale.Category.FORMAT);
        // try to get the pattern from the cache
        String pattern = cachedLocaleData.get(def);
        if (pattern == null) {  /* cache miss */
            // Get the pattern for the default locale.
            pattern = LocaleData.get(def).numberPattern;
            /* update cache */
            cachedLocaleData.putIfAbsent(def, pattern);
        }
        this.symbols = new DecimalFormatSymbols(def);
        init(pattern);
    }


    /**
     * Creates a DecimalFormat using the given pattern and the symbols
     * for the default locale. This is a convenient way to obtain a
     * DecimalFormat when internationalization is not the main concern.
     * <p>
     * To obtain standard formats for a given locale, use the factory methods
     * on NumberFormat such as getNumberInstance. These factories will
     * return the most appropriate sub-class of NumberFormat for a given
     * locale.
     *
     * @param pattern A non-localized pattern string.
     * @exception NullPointerException if <code>pattern</code> is null
     * @exception IllegalArgumentException if the given pattern is invalid.
     * @see java.text.NumberFormat#getInstance
     * @see java.text.NumberFormat#getNumberInstance
     * @see java.text.NumberFormat#getCurrencyInstance
     * @see java.text.NumberFormat#getPercentInstance
     */
    public DecimalFormat(String pattern) {
        this.symbols = new DecimalFormatSymbols(Locale.getDefault(Locale.Category.FORMAT));
        init(pattern);
    }


    /**
     * Creates a DecimalFormat using the given pattern and symbols.
     * Use this constructor when you need to completely customize the
     * behavior of the format.
     * <p>
     * To obtain standard formats for a given
     * locale, use the factory methods on NumberFormat such as
     * getInstance or getCurrencyInstance. If you need only minor adjustments
     * to a standard format, you can modify the format returned by
     * a NumberFormat factory method.
     *
     * @param pattern a non-localized pattern string
     * @param symbols the set of symbols to be used
     * @exception NullPointerException if any of the given arguments is null
     * @exception IllegalArgumentException if the given pattern is invalid
     * @see java.text.NumberFormat#getInstance
     * @see java.text.NumberFormat#getNumberInstance
     * @see java.text.NumberFormat#getCurrencyInstance
     * @see java.text.NumberFormat#getPercentInstance
     * @see java.text.DecimalFormatSymbols
     */
    public DecimalFormat (String pattern, DecimalFormatSymbols symbols) {
        // Always applyPattern after the symbols are set
        this.symbols = (DecimalFormatSymbols)symbols.clone();
        init(pattern);
    }

    private void init(String pattern) {
        this.icuDecimalFormat =  new android.icu.text.DecimalFormat(pattern,
                symbols.getIcuDecimalFormatSymbols());
        updateFieldsFromIcu();
    }

    /**
     * Converts between field positions used by Java/ICU.
     * @param fp The java.text.NumberFormat.Field field position
     * @return The android.icu.text.NumberFormat.Field field position
     */
    private static FieldPosition getIcuFieldPosition(FieldPosition fp) {
        if (fp.getFieldAttribute() == null) return fp;

        android.icu.text.NumberFormat.Field attribute;
        if (fp.getFieldAttribute() == Field.INTEGER) {
            attribute = android.icu.text.NumberFormat.Field.INTEGER;
        } else if (fp.getFieldAttribute() == Field.FRACTION) {
            attribute = android.icu.text.NumberFormat.Field.FRACTION;
        } else if (fp.getFieldAttribute() == Field.DECIMAL_SEPARATOR) {
            attribute = android.icu.text.NumberFormat.Field.DECIMAL_SEPARATOR;
        } else if (fp.getFieldAttribute() == Field.EXPONENT_SYMBOL) {
            attribute = android.icu.text.NumberFormat.Field.EXPONENT_SYMBOL;
        } else if (fp.getFieldAttribute() == Field.EXPONENT_SIGN) {
            attribute = android.icu.text.NumberFormat.Field.EXPONENT_SIGN;
        } else if (fp.getFieldAttribute() == Field.EXPONENT) {
            attribute = android.icu.text.NumberFormat.Field.EXPONENT;
        } else if (fp.getFieldAttribute() == Field.GROUPING_SEPARATOR) {
            attribute = android.icu.text.NumberFormat.Field.GROUPING_SEPARATOR;
        } else if (fp.getFieldAttribute() == Field.CURRENCY) {
            attribute = android.icu.text.NumberFormat.Field.CURRENCY;
        } else if (fp.getFieldAttribute() == Field.PERCENT) {
            attribute = android.icu.text.NumberFormat.Field.PERCENT;
        } else if (fp.getFieldAttribute() == Field.PERMILLE) {
            attribute = android.icu.text.NumberFormat.Field.PERMILLE;
        } else if (fp.getFieldAttribute() == Field.SIGN) {
            attribute = android.icu.text.NumberFormat.Field.SIGN;
        } else {
            throw new IllegalArgumentException("Unexpected field position attribute type.");
        }

        FieldPosition icuFieldPosition = new FieldPosition(attribute);
        icuFieldPosition.setBeginIndex(fp.getBeginIndex());
        icuFieldPosition.setEndIndex(fp.getEndIndex());
        return icuFieldPosition;
    }

    /**
     * Converts the Attribute that ICU returns in its AttributedCharacterIterator
     * responses to the type that java uses.
     * @param icuAttribute The AttributedCharacterIterator.Attribute field.
     * @return Field converted to a java.text.NumberFormat.Field field.
     */
    private static Field toJavaFieldAttribute(AttributedCharacterIterator.Attribute icuAttribute) {
        if (icuAttribute.getName().equals(Field.INTEGER.getName())) {
            return Field.INTEGER;
        }
        if (icuAttribute.getName().equals(Field.CURRENCY.getName())) {
            return Field.CURRENCY;
        }
        if (icuAttribute.getName().equals(Field.DECIMAL_SEPARATOR.getName())) {
            return Field.DECIMAL_SEPARATOR;
        }
        if (icuAttribute.getName().equals(Field.EXPONENT.getName())) {
            return Field.EXPONENT;
        }
        if (icuAttribute.getName().equals(Field.EXPONENT_SIGN.getName())) {
            return Field.EXPONENT_SIGN;
        }
        if (icuAttribute.getName().equals(Field.EXPONENT_SYMBOL.getName())) {
            return Field.EXPONENT_SYMBOL;
        }
        if (icuAttribute.getName().equals(Field.FRACTION.getName())) {
            return Field.FRACTION;
        }
        if (icuAttribute.getName().equals(Field.GROUPING_SEPARATOR.getName())) {
            return Field.GROUPING_SEPARATOR;
        }
        if (icuAttribute.getName().equals(Field.SIGN.getName())) {
            return Field.SIGN;
        }
        if (icuAttribute.getName().equals(Field.PERCENT.getName())) {
            return Field.PERCENT;
        }
        if (icuAttribute.getName().equals(Field.PERMILLE.getName())) {
            return Field.PERMILLE;
        }
        throw new IllegalArgumentException("Unrecognized attribute: " + icuAttribute.getName());
   }

    // Overrides
    /**
     * Formats a number and appends the resulting text to the given string
     * buffer.
     * The number can be of any subclass of {@link java.lang.Number}.
     * <p>
     * This implementation uses the maximum precision permitted.
     * @param number     the number to format
     * @param toAppendTo the <code>StringBuffer</code> to which the formatted
     *                   text is to be appended
     * @param pos        On input: an alignment field, if desired.
     *                   On output: the offsets of the alignment field.
     * @return           the value passed in as <code>toAppendTo</code>
     * @exception        IllegalArgumentException if <code>number</code> is
     *                   null or not an instance of <code>Number</code>.
     * @exception        NullPointerException if <code>toAppendTo</code> or
     *                   <code>pos</code> is null
     * @exception        ArithmeticException if rounding is needed with rounding
     *                   mode being set to RoundingMode.UNNECESSARY
     * @see              java.text.FieldPosition
     */
    public final StringBuffer format(Object number,
                                     StringBuffer toAppendTo,
                                     FieldPosition pos) {
        if (number instanceof Long || number instanceof Integer ||
                   number instanceof Short || number instanceof Byte ||
                   number instanceof AtomicInteger ||
                   number instanceof AtomicLong ||
                   (number instanceof BigInteger &&
                    ((BigInteger)number).bitLength () < 64)) {
            return format(((Number)number).longValue(), toAppendTo, pos);
        } else if (number instanceof BigDecimal) {
            return format((BigDecimal)number, toAppendTo, pos);
        } else if (number instanceof BigInteger) {
            return format((BigInteger)number, toAppendTo, pos);
        } else if (number instanceof Number) {
            return format(((Number)number).doubleValue(), toAppendTo, pos);
        } else {
            throw new IllegalArgumentException("Cannot format given Object as a Number");
        }
    }

    /**
     * Formats a double to produce a string.
     * @param number    The double to format
     * @param result    where the text is to be appended
     * @param fieldPosition    On input: an alignment field, if desired.
     * On output: the offsets of the alignment field.
     * @exception ArithmeticException if rounding is needed with rounding
     *            mode being set to RoundingMode.UNNECESSARY
     * @return The formatted number string
     * @see java.text.FieldPosition
     */
    public StringBuffer format(double number, StringBuffer result,
                               FieldPosition fieldPosition) {
        FieldPosition icuFieldPosition = getIcuFieldPosition(fieldPosition);
        icuDecimalFormat.format(number, result, icuFieldPosition);
        fieldPosition.setBeginIndex(icuFieldPosition.getBeginIndex());
        fieldPosition.setEndIndex(icuFieldPosition.getEndIndex());
        return result;
    }

    /**
     * Format a long to produce a string.
     * @param number    The long to format
     * @param result    where the text is to be appended
     * @param fieldPosition    On input: an alignment field, if desired.
     * On output: the offsets of the alignment field.
     * @exception       ArithmeticException if rounding is needed with rounding
     *                  mode being set to RoundingMode.UNNECESSARY
     * @return The formatted number string
     * @see java.text.FieldPosition
     */
    public StringBuffer format(long number, StringBuffer result,
                               FieldPosition fieldPosition) {
        FieldPosition icuFieldPosition = getIcuFieldPosition(fieldPosition);
        icuDecimalFormat.format(number, result, icuFieldPosition);
        fieldPosition.setBeginIndex(icuFieldPosition.getBeginIndex());
        fieldPosition.setEndIndex(icuFieldPosition.getEndIndex());
        return result;
    }

    /**
     * Formats a BigDecimal to produce a string.
     * @param number    The BigDecimal to format
     * @param result    where the text is to be appended
     * @param fieldPosition    On input: an alignment field, if desired.
     * On output: the offsets of the alignment field.
     * @return The formatted number string
     * @exception        ArithmeticException if rounding is needed with rounding
     *                   mode being set to RoundingMode.UNNECESSARY
     * @see java.text.FieldPosition
     */
    private StringBuffer format(BigDecimal number, StringBuffer result,
                                FieldPosition fieldPosition) {
        FieldPosition icuFieldPosition = getIcuFieldPosition(fieldPosition);
        icuDecimalFormat.format(number, result, fieldPosition);
        fieldPosition.setBeginIndex(icuFieldPosition.getBeginIndex());
        fieldPosition.setEndIndex(icuFieldPosition.getEndIndex());
        return result;
    }

    /**
     * Format a BigInteger to produce a string.
     * @param number    The BigInteger to format
     * @param result    where the text is to be appended
     * @param fieldPosition    On input: an alignment field, if desired.
     * On output: the offsets of the alignment field.
     * @return The formatted number string
     * @exception        ArithmeticException if rounding is needed with rounding
     *                   mode being set to RoundingMode.UNNECESSARY
     * @see java.text.FieldPosition
     */
    private StringBuffer format(BigInteger number, StringBuffer result,
                               FieldPosition fieldPosition) {
        FieldPosition icuFieldPosition = getIcuFieldPosition(fieldPosition);
        icuDecimalFormat.format(number, result, fieldPosition);
        fieldPosition.setBeginIndex(icuFieldPosition.getBeginIndex());
        fieldPosition.setEndIndex(icuFieldPosition.getEndIndex());
        return result;
    }

    /**
     * Formats an Object producing an <code>AttributedCharacterIterator</code>.
     * You can use the returned <code>AttributedCharacterIterator</code>
     * to build the resulting String, as well as to determine information
     * about the resulting String.
     * <p>
     * Each attribute key of the AttributedCharacterIterator will be of type
     * <code>NumberFormat.Field</code>, with the attribute value being the
     * same as the attribute key.
     *
     * @exception NullPointerException if obj is null.
     * @exception IllegalArgumentException when the Format cannot format the
     *            given object.
     * @exception        ArithmeticException if rounding is needed with rounding
     *                   mode being set to RoundingMode.UNNECESSARY
     * @param obj The object to format
     * @return AttributedCharacterIterator describing the formatted value.
     * @since 1.4
     */
    public AttributedCharacterIterator formatToCharacterIterator(Object obj) {
        if (obj == null) {
            throw new NullPointerException("object == null");
        }
        // Note: formatToCharacterIterator cannot be used directly because it returns attributes
        // in terms of its own class: icu.text.NumberFormat instead of java.text.NumberFormat.
        // http://bugs.icu-project.org/trac/ticket/11931 Proposes to use the NumberFormat constants.

        AttributedCharacterIterator original = icuDecimalFormat.formatToCharacterIterator(obj);

        // Extract the text out of the ICU iterator.
        StringBuilder textBuilder = new StringBuilder(
                original.getEndIndex() - original.getBeginIndex());

        for (int i = original.getBeginIndex(); i < original.getEndIndex(); i++) {
            textBuilder.append(original.current());
            original.next();
        }

        AttributedString result = new AttributedString(textBuilder.toString());

        for (int i = original.getBeginIndex(); i < original.getEndIndex(); i++) {
            original.setIndex(i);

            for (AttributedCharacterIterator.Attribute attribute
                    : original.getAttributes().keySet()) {
                    int start = original.getRunStart();
                    int end = original.getRunLimit();
                    Field javaAttr = toJavaFieldAttribute(attribute);
                    result.addAttribute(javaAttr, javaAttr, start, end);
            }
        }

        return result.getIterator();
    }

    /**
     * Parses text from a string to produce a <code>Number</code>.
     * <p>
     * The method attempts to parse text starting at the index given by
     * <code>pos</code>.
     * If parsing succeeds, then the index of <code>pos</code> is updated
     * to the index after the last character used (parsing does not necessarily
     * use all characters up to the end of the string), and the parsed
     * number is returned. The updated <code>pos</code> can be used to
     * indicate the starting point for the next call to this method.
     * If an error occurs, then the index of <code>pos</code> is not
     * changed, the error index of <code>pos</code> is set to the index of
     * the character where the error occurred, and null is returned.
     * <p>
     * The subclass returned depends on the value of {@link #isParseBigDecimal}
     * as well as on the string being parsed.
     * <ul>
     *   <li>If <code>isParseBigDecimal()</code> is false (the default),
     *       most integer values are returned as <code>Long</code>
     *       objects, no matter how they are written: <code>"17"</code> and
     *       <code>"17.000"</code> both parse to <code>Long(17)</code>.
     *       Values that cannot fit into a <code>Long</code> are returned as
     *       <code>Double</code>s. This includes values with a fractional part,
     *       infinite values, <code>NaN</code>, and the value -0.0.
     *       <code>DecimalFormat</code> does <em>not</em> decide whether to
     *       return a <code>Double</code> or a <code>Long</code> based on the
     *       presence of a decimal separator in the source string. Doing so
     *       would prevent integers that overflow the mantissa of a double,
     *       such as <code>"-9,223,372,036,854,775,808.00"</code>, from being
     *       parsed accurately.
     *       <p>
     *       Callers may use the <code>Number</code> methods
     *       <code>doubleValue</code>, <code>longValue</code>, etc., to obtain
     *       the type they want.
     *   <li>If <code>isParseBigDecimal()</code> is true, values are returned
     *       as <code>BigDecimal</code> objects. The values are the ones
     *       constructed by {@link java.math.BigDecimal#BigDecimal(String)}
     *       for corresponding strings in locale-independent format. The
     *       special cases negative and positive infinity and NaN are returned
     *       as <code>Double</code> instances holding the values of the
     *       corresponding <code>Double</code> constants.
     * </ul>
     * <p>
     * <code>DecimalFormat</code> parses all Unicode characters that represent
     * decimal digits, as defined by <code>Character.digit()</code>. In
     * addition, <code>DecimalFormat</code> also recognizes as digits the ten
     * consecutive characters starting with the localized zero digit defined in
     * the <code>DecimalFormatSymbols</code> object.
     *
     * @param text the string to be parsed
     * @param pos  A <code>ParsePosition</code> object with index and error
     *             index information as described above.
     * @return     the parsed value, or <code>null</code> if the parse fails
     * @exception  NullPointerException if <code>text</code> or
     *             <code>pos</code> is null.
     */
    public Number parse(String text, ParsePosition pos) {
        // Return early if the parse position is bogus.
        if (pos.index < 0 || pos.index >= text.length()) {
            return null;
        }

        // This might return android.icu.math.BigDecimal, java.math.BigInteger or a primitive type.
        Number number = icuDecimalFormat.parse(text, pos);
        if (number == null) {
            return null;
        }
        if (isParseBigDecimal()) {
            if (number instanceof Long) {
                return new BigDecimal(number.longValue());
            }
            if ((number instanceof Double) && !((Double) number).isInfinite()
                    && !((Double) number).isNaN()) {
                return new BigDecimal(number.toString());
            }
            if ((number instanceof Double) &&
                    (((Double) number).isNaN() || ((Double) number).isInfinite())) {
                return number;
            }
            if (number instanceof android.icu.math.BigDecimal) {
                return ((android.icu.math.BigDecimal) number).toBigDecimal();
            }
        }
        if ((number instanceof android.icu.math.BigDecimal) || (number instanceof BigInteger)) {
            return number.doubleValue();
        }
        if (isParseIntegerOnly() && number.equals(new Double(-0.0))) {
            return 0L;
        }
        return number;
    }

    /**
     * Returns a copy of the decimal format symbols, which is generally not
     * changed by the programmer or user.
     * @return a copy of the desired DecimalFormatSymbols
     * @see java.text.DecimalFormatSymbols
     */
    public DecimalFormatSymbols getDecimalFormatSymbols() {
        return DecimalFormatSymbols.fromIcuInstance(icuDecimalFormat.getDecimalFormatSymbols());
    }


    /**
     * Sets the decimal format symbols, which is generally not changed
     * by the programmer or user.
     * @param newSymbols desired DecimalFormatSymbols
     * @see java.text.DecimalFormatSymbols
     */
    public void setDecimalFormatSymbols(DecimalFormatSymbols newSymbols) {
        try {
            // don't allow multiple references
            symbols = (DecimalFormatSymbols) newSymbols.clone();
            icuDecimalFormat.setDecimalFormatSymbols(symbols.getIcuDecimalFormatSymbols());
        } catch (Exception foo) {
            // should never happen
        }
    }

    /**
     * Get the positive prefix.
     * <P>Examples: +123, $123, sFr123
     */
    public String getPositivePrefix () {
        return icuDecimalFormat.getPositivePrefix();
    }

    /**
     * Set the positive prefix.
     * <P>Examples: +123, $123, sFr123
     */
    public void setPositivePrefix (String newValue) {
        icuDecimalFormat.setPositivePrefix(newValue);
    }

    /**
     * Get the  prefix.
     * <P>Examples: -123, ($123) (with negative suffix), sFr-123
     */
    public String getNegativePrefix () {
        return icuDecimalFormat.getNegativePrefix();
    }

    /**
     * Set the negative prefix.
     * <P>Examples: -123, ($123) (with negative suffix), sFr-123
     */
    public void setNegativePrefix (String newValue) {
        icuDecimalFormat.setNegativePrefix(newValue);
    }

    /**
     * Get the positive suffix.
     * <P>Example: 123%
     */
    public String getPositiveSuffix () {
        return icuDecimalFormat.getPositiveSuffix();
    }

    /**
     * Set the positive suffix.
     * <P>Example: 123%
     */
    public void setPositiveSuffix (String newValue) {
        icuDecimalFormat.setPositiveSuffix(newValue);
    }

    /**
     * Get the negative suffix.
     * <P>Examples: -123%, ($123) (with positive suffixes)
     */
    public String getNegativeSuffix () {
        return icuDecimalFormat.getNegativeSuffix();
    }

    /**
     * Set the negative suffix.
     * <P>Examples: 123%
     */
    public void setNegativeSuffix (String newValue) {
        icuDecimalFormat.setNegativeSuffix(newValue);
    }

    /**
     * Gets the multiplier for use in percent, per mille, and similar
     * formats.
     *
     * @see #setMultiplier(int)
     */
    public int getMultiplier () {
        return icuDecimalFormat.getMultiplier();
    }

    /**
     * Sets the multiplier for use in percent, per mille, and similar
     * formats.
     * For a percent format, set the multiplier to 100 and the suffixes to
     * have '%' (for Arabic, use the Arabic percent sign).
     * For a per mille format, set the multiplier to 1000 and the suffixes to
     * have '&#92;u2030'.
     *
     * <P>Example: with multiplier 100, 1.23 is formatted as "123", and
     * "123" is parsed into 1.23.
     *
     * @see #getMultiplier
     */
    public void setMultiplier (int newValue) {
        icuDecimalFormat.setMultiplier(newValue);
    }

    /**
     * Return the grouping size. Grouping size is the number of digits between
     * grouping separators in the integer portion of a number.  For example,
     * in the number "123,456.78", the grouping size is 3.
     * @see #setGroupingSize
     * @see java.text.NumberFormat#isGroupingUsed
     * @see java.text.DecimalFormatSymbols#getGroupingSeparator
     */
    public int getGroupingSize () {
        return icuDecimalFormat.getGroupingSize();
    }

    /**
     * Set the grouping size. Grouping size is the number of digits between
     * grouping separators in the integer portion of a number.  For example,
     * in the number "123,456.78", the grouping size is 3.
     * <br>
     * The value passed in is converted to a byte, which may lose information.
     * @see #getGroupingSize
     * @see java.text.NumberFormat#setGroupingUsed
     * @see java.text.DecimalFormatSymbols#setGroupingSeparator
     */
    public void setGroupingSize (int newValue) {
        icuDecimalFormat.setGroupingSize(newValue);
    }

    /**
     * Returns true if grouping is used in this format. For example, in the
     * English locale, with grouping on, the number 1234567 might be formatted
     * as "1,234,567". The grouping separator as well as the size of each group
     * is locale dependant and is determined by sub-classes of NumberFormat.
     * @see #setGroupingUsed
     */
    public boolean isGroupingUsed() {
        return icuDecimalFormat.isGroupingUsed();
    }

    /**
     * Set whether or not grouping will be used in this format.
     * @see #isGroupingUsed
     */
    public void setGroupingUsed(boolean newValue) {
        icuDecimalFormat.setGroupingUsed(newValue);
    }

    /**
     * Allows you to get the behavior of the decimal separator with integers.
     * (The decimal separator will always appear with decimals.)
     * <P>Example: Decimal ON: 12345 -> 12345.; OFF: 12345 -> 12345
     */
    public boolean isDecimalSeparatorAlwaysShown() {
        return icuDecimalFormat.isDecimalSeparatorAlwaysShown();
    }

    /**
     * Allows you to set the behavior of the decimal separator with integers.
     * (The decimal separator will always appear with decimals.)
     * <P>Example: Decimal ON: 12345 -> 12345.; OFF: 12345 -> 12345
     */
    public void setDecimalSeparatorAlwaysShown(boolean newValue) {
        icuDecimalFormat.setDecimalSeparatorAlwaysShown(newValue);
    }

    /**
     * Returns whether the {@link #parse(java.lang.String, java.text.ParsePosition)}
     * method returns <code>BigDecimal</code>. The default value is false.
     * @see #setParseBigDecimal
     * @since 1.5
     */
    public boolean isParseBigDecimal() {
        return icuDecimalFormat.isParseBigDecimal();
    }

    /**
     * Sets whether the {@link #parse(java.lang.String, java.text.ParsePosition)}
     * method returns <code>BigDecimal</code>.
     * @see #isParseBigDecimal
     * @since 1.5
     */
    public void setParseBigDecimal(boolean newValue) {
        icuDecimalFormat.setParseBigDecimal(newValue);
    }

    /**
     * Sets whether or not numbers should be parsed as integers only.
     * @see #isParseIntegerOnly
     */
    public void setParseIntegerOnly(boolean value) {
        super.setParseIntegerOnly(value);
        icuDecimalFormat.setParseIntegerOnly(value);
    }

    /**
     * Returns true if this format will parse numbers as integers only.
     * For example in the English locale, with ParseIntegerOnly true, the
     * string "1234." would be parsed as the integer value 1234 and parsing
     * would stop at the "." character.  Of course, the exact format accepted
     * by the parse operation is locale dependant and determined by sub-classes
     * of NumberFormat.
     */
    public boolean isParseIntegerOnly() {
        return icuDecimalFormat.isParseIntegerOnly();
    }

    /**
     * Standard override; no change in semantics.
     */
    public Object clone() {
        try {
            DecimalFormat other = (DecimalFormat) super.clone();
            other.icuDecimalFormat = (android.icu.text.DecimalFormat) icuDecimalFormat.clone();
            other.symbols = (DecimalFormatSymbols) symbols.clone();
            return other;
        } catch (Exception e) {
            throw new InternalError();
        }
    }

    /**
     * Overrides equals
     */
    public boolean equals(Object obj)
    {
        if (obj == null) {
            return false;
        }
        if (this == obj) {
            return true;
        }
        if (!(obj instanceof DecimalFormat)) {
            return false;
        }
        DecimalFormat other = (DecimalFormat) obj;
        return icuDecimalFormat.equals(other.icuDecimalFormat)
            && compareIcuRoundingIncrement(other.icuDecimalFormat);
    }

    private boolean compareIcuRoundingIncrement(android.icu.text.DecimalFormat other) {
        BigDecimal increment = this.icuDecimalFormat.getRoundingIncrement();
        if (increment != null) {
            return (other.getRoundingIncrement() != null)
                && increment.equals(other.getRoundingIncrement());
        }
        return other.getRoundingIncrement() == null;
    }

    /**
     * Overrides hashCode
     */
    public int hashCode() {
        return super.hashCode() * 37 + getPositivePrefix().hashCode();
        // just enough fields for a reasonable distribution
    }

    /**
     * Synthesizes a pattern string that represents the current state
     * of this Format object.
     * @see #applyPattern
     */
    public String toPattern() {
        return icuDecimalFormat.toPattern();
    }

    /**
     * Synthesizes a localized pattern string that represents the current
     * state of this Format object.
     * @see #applyPattern
     */
    public String toLocalizedPattern() {
        return icuDecimalFormat.toLocalizedPattern();
    }

    /**
     * Apply the given pattern to this Format object.  A pattern is a
     * short-hand specification for the various formatting properties.
     * These properties can also be changed individually through the
     * various setter methods.
     * <p>
     * There is no limit to integer digits set
     * by this routine, since that is the typical end-user desire;
     * use setMaximumInteger if you want to set a real value.
     * For negative numbers, use a second pattern, separated by a semicolon
     * <P>Example <code>"#,#00.0#"</code> -> 1,234.56
     * <P>This means a minimum of 2 integer digits, 1 fraction digit, and
     * a maximum of 2 fraction digits.
     * <p>Example: <code>"#,#00.0#;(#,#00.0#)"</code> for negatives in
     * parentheses.
     * <p>In negative patterns, the minimum and maximum counts are ignored;
     * these are presumed to be set in the positive pattern.
     *
     * @exception NullPointerException if <code>pattern</code> is null
     * @exception IllegalArgumentException if the given pattern is invalid.
     */
    public void applyPattern(String pattern) {
        icuDecimalFormat.applyPattern(pattern);
        updateFieldsFromIcu();
    }


    /**
     * Apply the given pattern to this Format object.  The pattern
     * is assumed to be in a localized notation. A pattern is a
     * short-hand specification for the various formatting properties.
     * These properties can also be changed individually through the
     * various setter methods.
     * <p>
     * There is no limit to integer digits set
     * by this routine, since that is the typical end-user desire;
     * use setMaximumInteger if you want to set a real value.
     * For negative numbers, use a second pattern, separated by a semicolon
     * <P>Example <code>"#,#00.0#"</code> -> 1,234.56
     * <P>This means a minimum of 2 integer digits, 1 fraction digit, and
     * a maximum of 2 fraction digits.
     * <p>Example: <code>"#,#00.0#;(#,#00.0#)"</code> for negatives in
     * parentheses.
     * <p>In negative patterns, the minimum and maximum counts are ignored;
     * these are presumed to be set in the positive pattern.
     *
     * @exception NullPointerException if <code>pattern</code> is null
     * @exception IllegalArgumentException if the given pattern is invalid.
     */
    public void applyLocalizedPattern(String pattern) {
        icuDecimalFormat.applyLocalizedPattern(pattern);
        updateFieldsFromIcu();
    }

    private void updateFieldsFromIcu() {
        // Imitate behaviour of ICU4C NumberFormat that Android used up to M.
        // If the pattern doesn't enforce a different value (some exponential
        // patterns do), then set the maximum integer digits to 2 billion.
        if (icuDecimalFormat.getMaximumIntegerDigits() == DOUBLE_INTEGER_DIGITS) {
            icuDecimalFormat.setMaximumIntegerDigits(2000000000);
        }
        maximumIntegerDigits = icuDecimalFormat.getMaximumIntegerDigits();
        minimumIntegerDigits = icuDecimalFormat.getMinimumIntegerDigits();
        maximumFractionDigits = icuDecimalFormat.getMaximumFractionDigits();
        minimumFractionDigits = icuDecimalFormat.getMinimumFractionDigits();
    }

    /**
     * Sets the maximum number of digits allowed in the integer portion of a
     * number.
     * For formatting numbers other than <code>BigInteger</code> and
     * <code>BigDecimal</code> objects, the lower of <code>newValue</code> and
     * 309 is used. Negative input values are replaced with 0.
     * @see NumberFormat#setMaximumIntegerDigits
     */
    public void setMaximumIntegerDigits(int newValue) {
        maximumIntegerDigits = Math.min(Math.max(0, newValue), MAXIMUM_INTEGER_DIGITS);
        super.setMaximumIntegerDigits((maximumIntegerDigits > DOUBLE_INTEGER_DIGITS) ?
            DOUBLE_INTEGER_DIGITS : maximumIntegerDigits);
        if (minimumIntegerDigits > maximumIntegerDigits) {
            minimumIntegerDigits = maximumIntegerDigits;
            super.setMinimumIntegerDigits((minimumIntegerDigits > DOUBLE_INTEGER_DIGITS) ?
                DOUBLE_INTEGER_DIGITS : minimumIntegerDigits);
        }
        icuDecimalFormat.setMaximumIntegerDigits(getMaximumIntegerDigits());
    }

    /**
     * Sets the minimum number of digits allowed in the integer portion of a
     * number.
     * For formatting numbers other than <code>BigInteger</code> and
     * <code>BigDecimal</code> objects, the lower of <code>newValue</code> and
     * 309 is used. Negative input values are replaced with 0.
     * @see NumberFormat#setMinimumIntegerDigits
     */
    public void setMinimumIntegerDigits(int newValue) {
        minimumIntegerDigits = Math.min(Math.max(0, newValue), MAXIMUM_INTEGER_DIGITS);
        super.setMinimumIntegerDigits((minimumIntegerDigits > DOUBLE_INTEGER_DIGITS) ?
            DOUBLE_INTEGER_DIGITS : minimumIntegerDigits);
        if (minimumIntegerDigits > maximumIntegerDigits) {
            maximumIntegerDigits = minimumIntegerDigits;
            super.setMaximumIntegerDigits((maximumIntegerDigits > DOUBLE_INTEGER_DIGITS) ?
                DOUBLE_INTEGER_DIGITS : maximumIntegerDigits);
        }
        icuDecimalFormat.setMinimumIntegerDigits(getMinimumIntegerDigits());
    }

    /**
     * Sets the maximum number of digits allowed in the fraction portion of a
     * number.
     * For formatting numbers other than <code>BigInteger</code> and
     * <code>BigDecimal</code> objects, the lower of <code>newValue</code> and
     * 340 is used. Negative input values are replaced with 0.
     * @see NumberFormat#setMaximumFractionDigits
     */
    public void setMaximumFractionDigits(int newValue) {
        maximumFractionDigits = Math.min(Math.max(0, newValue), MAXIMUM_FRACTION_DIGITS);
        super.setMaximumFractionDigits((maximumFractionDigits > DOUBLE_FRACTION_DIGITS) ?
            DOUBLE_FRACTION_DIGITS : maximumFractionDigits);
        if (minimumFractionDigits > maximumFractionDigits) {
            minimumFractionDigits = maximumFractionDigits;
            super.setMinimumFractionDigits((minimumFractionDigits > DOUBLE_FRACTION_DIGITS) ?
                DOUBLE_FRACTION_DIGITS : minimumFractionDigits);
        }
        icuDecimalFormat.setMaximumFractionDigits(getMaximumFractionDigits());
    }

    /**
     * Sets the minimum number of digits allowed in the fraction portion of a
     * number.
     * For formatting numbers other than <code>BigInteger</code> and
     * <code>BigDecimal</code> objects, the lower of <code>newValue</code> and
     * 340 is used. Negative input values are replaced with 0.
     * @see NumberFormat#setMinimumFractionDigits
     */
    public void setMinimumFractionDigits(int newValue) {
        minimumFractionDigits = Math.min(Math.max(0, newValue), MAXIMUM_FRACTION_DIGITS);
        super.setMinimumFractionDigits((minimumFractionDigits > DOUBLE_FRACTION_DIGITS) ?
            DOUBLE_FRACTION_DIGITS : minimumFractionDigits);
        if (minimumFractionDigits > maximumFractionDigits) {
            maximumFractionDigits = minimumFractionDigits;
            super.setMaximumFractionDigits((maximumFractionDigits > DOUBLE_FRACTION_DIGITS) ?
                DOUBLE_FRACTION_DIGITS : maximumFractionDigits);
        }
        icuDecimalFormat.setMinimumFractionDigits(getMinimumFractionDigits());
    }

    /**
     * Gets the maximum number of digits allowed in the integer portion of a
     * number.
     * For formatting numbers other than <code>BigInteger</code> and
     * <code>BigDecimal</code> objects, the lower of the return value and
     * 309 is used.
     * @see #setMaximumIntegerDigits
     */
    public int getMaximumIntegerDigits() {
        return maximumIntegerDigits;
    }

    /**
     * Gets the minimum number of digits allowed in the integer portion of a
     * number.
     * For formatting numbers other than <code>BigInteger</code> and
     * <code>BigDecimal</code> objects, the lower of the return value and
     * 309 is used.
     * @see #setMinimumIntegerDigits
     */
    public int getMinimumIntegerDigits() {
        return minimumIntegerDigits;
    }

    /**
     * Gets the maximum number of digits allowed in the fraction portion of a
     * number.
     * For formatting numbers other than <code>BigInteger</code> and
     * <code>BigDecimal</code> objects, the lower of the return value and
     * 340 is used.
     * @see #setMaximumFractionDigits
     */
    public int getMaximumFractionDigits() {
        return maximumFractionDigits;
    }

    /**
     * Gets the minimum number of digits allowed in the fraction portion of a
     * number.
     * For formatting numbers other than <code>BigInteger</code> and
     * <code>BigDecimal</code> objects, the lower of the return value and
     * 340 is used.
     * @see #setMinimumFractionDigits
     */
    public int getMinimumFractionDigits() {
        return minimumFractionDigits;
    }

    /**
     * Gets the currency used by this decimal format when formatting
     * currency values.
     * The currency is obtained by calling
     * {@link DecimalFormatSymbols#getCurrency DecimalFormatSymbols.getCurrency}
     * on this number format's symbols.
     *
     * @return the currency used by this decimal format, or <code>null</code>
     * @since 1.4
     */
    public Currency getCurrency() {
        return symbols.getCurrency();
    }

    /**
     * Sets the currency used by this number format when formatting
     * currency values. This does not update the minimum or maximum
     * number of fraction digits used by the number format.
     * The currency is set by calling
     * {@link DecimalFormatSymbols#setCurrency DecimalFormatSymbols.setCurrency}
     * on this number format's symbols.
     *
     * @param currency the new currency to be used by this decimal format
     * @exception NullPointerException if <code>currency</code> is null
     * @since 1.4
     */
    public void setCurrency(Currency currency) {
        // Set the international currency symbol, and currency symbol on the DecimalFormatSymbols
        // object and tell ICU to use that.
        if (currency != symbols.getCurrency()
            || !currency.getSymbol().equals(symbols.getCurrencySymbol())) {
            symbols.setCurrency(currency);
            icuDecimalFormat.setDecimalFormatSymbols(symbols.getIcuDecimalFormatSymbols());
            // Giving the icuDecimalFormat a new currency will cause the fractional digits to be
            // updated. This class is specified to not touch the fraction digits, so we re-set them.
            icuDecimalFormat.setMinimumFractionDigits(minimumFractionDigits);
            icuDecimalFormat.setMaximumFractionDigits(maximumFractionDigits);
        }
    }

    /**
     * Gets the {@link java.math.RoundingMode} used in this DecimalFormat.
     *
     * @return The <code>RoundingMode</code> used for this DecimalFormat.
     * @see #setRoundingMode(RoundingMode)
     * @since 1.6
     */
    public RoundingMode getRoundingMode() {
        return roundingMode;
    }

    private static int convertRoundingMode(RoundingMode rm) {
        switch (rm) {
        case UP:
            return MathContext.ROUND_UP;
        case DOWN:
            return MathContext.ROUND_DOWN;
        case CEILING:
            return MathContext.ROUND_CEILING;
        case FLOOR:
            return MathContext.ROUND_FLOOR;
        case HALF_UP:
            return MathContext.ROUND_HALF_UP;
        case HALF_DOWN:
            return MathContext.ROUND_HALF_DOWN;
        case HALF_EVEN:
            return MathContext.ROUND_HALF_EVEN;
        case UNNECESSARY:
            return MathContext.ROUND_UNNECESSARY;
        }
        throw new IllegalArgumentException("Invalid rounding mode specified");
    }

    /**
     * Sets the {@link java.math.RoundingMode} used in this DecimalFormat.
     *
     * @param roundingMode The <code>RoundingMode</code> to be used
     * @see #getRoundingMode()
     * @exception NullPointerException if <code>roundingMode</code> is null.
     * @since 1.6
     */
    public void setRoundingMode(RoundingMode roundingMode) {
        if (roundingMode == null) {
            throw new NullPointerException();
        }

        this.roundingMode = roundingMode;

        icuDecimalFormat.setRoundingMode(convertRoundingMode(roundingMode));
    }

    /**
     * Adjusts the minimum and maximum fraction digits to values that
     * are reasonable for the currency's default fraction digits.
     */
    void adjustForCurrencyDefaultFractionDigits() {
        Currency currency = symbols.getCurrency();
        if (currency == null) {
            try {
                currency = Currency.getInstance(symbols.getInternationalCurrencySymbol());
            } catch (IllegalArgumentException e) {
            }
        }
        if (currency != null) {
            int digits = currency.getDefaultFractionDigits();
            if (digits != -1) {
                int oldMinDigits = getMinimumFractionDigits();
                // Common patterns are "#.##", "#.00", "#".
                // Try to adjust all of them in a reasonable way.
                if (oldMinDigits == getMaximumFractionDigits()) {
                    setMinimumFractionDigits(digits);
                    setMaximumFractionDigits(digits);
                } else {
                    setMinimumFractionDigits(Math.min(digits, oldMinDigits));
                    setMaximumFractionDigits(digits);
                }
            }
        }
    }

    private static final int currentSerialVersion = 4;

    // the fields list to be serialized
    private static final ObjectStreamField[] serialPersistentFields = {
            new ObjectStreamField("positivePrefix", String.class),
            new ObjectStreamField("positiveSuffix", String.class),
            new ObjectStreamField("negativePrefix", String.class),
            new ObjectStreamField("negativeSuffix", String.class),
            new ObjectStreamField("posPrefixPattern", String.class),
            new ObjectStreamField("posSuffixPattern", String.class),
            new ObjectStreamField("negPrefixPattern", String.class),
            new ObjectStreamField("negSuffixPattern", String.class),
            new ObjectStreamField("multiplier", int.class),
            new ObjectStreamField("groupingSize", byte.class),
            new ObjectStreamField("groupingUsed", boolean.class),
            new ObjectStreamField("decimalSeparatorAlwaysShown", boolean.class),
            new ObjectStreamField("parseBigDecimal", boolean.class),
            new ObjectStreamField("roundingMode", RoundingMode.class),
            new ObjectStreamField("symbols", DecimalFormatSymbols.class),
            new ObjectStreamField("useExponentialNotation", boolean.class),
            new ObjectStreamField("minExponentDigits", byte.class),
            new ObjectStreamField("maximumIntegerDigits", int.class),
            new ObjectStreamField("minimumIntegerDigits", int.class),
            new ObjectStreamField("maximumFractionDigits", int.class),
            new ObjectStreamField("minimumFractionDigits", int.class),
            new ObjectStreamField("serialVersionOnStream", int.class),
    };

    private void writeObject(ObjectOutputStream stream) throws IOException, ClassNotFoundException {
        ObjectOutputStream.PutField fields = stream.putFields();
        fields.put("positivePrefix", icuDecimalFormat.getPositivePrefix());
        fields.put("positiveSuffix", icuDecimalFormat.getPositiveSuffix());
        fields.put("negativePrefix", icuDecimalFormat.getNegativePrefix());
        fields.put("negativeSuffix", icuDecimalFormat.getNegativeSuffix());
        fields.put("posPrefixPattern", (String) null);
        fields.put("posSuffixPattern", (String) null);
        fields.put("negPrefixPattern", (String) null);
        fields.put("negSuffixPattern", (String) null);
        fields.put("multiplier", icuDecimalFormat.getMultiplier());
        fields.put("groupingSize", (byte) icuDecimalFormat.getGroupingSize());
        fields.put("groupingUsed", icuDecimalFormat.isGroupingUsed());
        fields.put("decimalSeparatorAlwaysShown", icuDecimalFormat.isDecimalSeparatorAlwaysShown());
        fields.put("parseBigDecimal", icuDecimalFormat.isParseBigDecimal());
        fields.put("roundingMode", roundingMode);
        fields.put("symbols", symbols);
        fields.put("useExponentialNotation", false);
        fields.put("minExponentDigits", (byte) 0);
        fields.put("maximumIntegerDigits", icuDecimalFormat.getMaximumIntegerDigits());
        fields.put("minimumIntegerDigits", icuDecimalFormat.getMinimumIntegerDigits());
        fields.put("maximumFractionDigits", icuDecimalFormat.getMaximumFractionDigits());
        fields.put("minimumFractionDigits", icuDecimalFormat.getMinimumFractionDigits());
        fields.put("serialVersionOnStream", currentSerialVersion);
        stream.writeFields();
    }

    /**
     * Reads the default serializable fields from the stream and performs
     * validations and adjustments for older serialized versions. The
     * validations and adjustments are:
     * <ol>
     * <li>
     * Verify that the superclass's digit count fields correctly reflect
     * the limits imposed on formatting numbers other than
     * <code>BigInteger</code> and <code>BigDecimal</code> objects. These
     * limits are stored in the superclass for serialization compatibility
     * with older versions, while the limits for <code>BigInteger</code> and
     * <code>BigDecimal</code> objects are kept in this class.
     * If, in the superclass, the minimum or maximum integer digit count is
     * larger than <code>DOUBLE_INTEGER_DIGITS</code> or if the minimum or
     * maximum fraction digit count is larger than
     * <code>DOUBLE_FRACTION_DIGITS</code>, then the stream data is invalid
     * and this method throws an <code>InvalidObjectException</code>.
     * <li>
     * If <code>serialVersionOnStream</code> is less than 4, initialize
     * <code>roundingMode</code> to {@link java.math.RoundingMode#HALF_EVEN
     * RoundingMode.HALF_EVEN}.  This field is new with version 4.
     * <li>
     * If <code>serialVersionOnStream</code> is less than 3, then call
     * the setters for the minimum and maximum integer and fraction digits with
     * the values of the corresponding superclass getters to initialize the
     * fields in this class. The fields in this class are new with version 3.
     * <li>
     * If <code>serialVersionOnStream</code> is less than 1, indicating that
     * the stream was written by JDK 1.1, initialize
     * <code>useExponentialNotation</code>
     * to false, since it was not present in JDK 1.1.
     * <li>
     * Set <code>serialVersionOnStream</code> to the maximum allowed value so
     * that default serialization will work properly if this object is streamed
     * out again.
     * </ol>
     *
     * <p>Stream versions older than 2 will not have the affix pattern variables
     * <code>posPrefixPattern</code> etc.  As a result, they will be initialized
     * to <code>null</code>, which means the affix strings will be taken as
     * literal values.  This is exactly what we want, since that corresponds to
     * the pre-version-2 behavior.
     */
    private void readObject(ObjectInputStream stream)
            throws IOException, ClassNotFoundException {
        ObjectInputStream.GetField fields = stream.readFields();
        this.symbols = (DecimalFormatSymbols) fields.get("symbols", null);

        init("");

        icuDecimalFormat.setPositivePrefix((String) fields.get("positivePrefix", ""));
        icuDecimalFormat.setPositiveSuffix((String) fields.get("positiveSuffix", ""));
        icuDecimalFormat.setNegativePrefix((String) fields.get("negativePrefix", "-"));
        icuDecimalFormat.setNegativeSuffix((String) fields.get("negativeSuffix", ""));
        icuDecimalFormat.setMultiplier(fields.get("multiplier", 1));
        icuDecimalFormat.setGroupingSize(fields.get("groupingSize", (byte) 3));
        icuDecimalFormat.setGroupingUsed(fields.get("groupingUsed", true));
        icuDecimalFormat.setDecimalSeparatorAlwaysShown(fields.get("decimalSeparatorAlwaysShown",
                false));

        setRoundingMode((RoundingMode) fields.get("roundingMode", RoundingMode.HALF_EVEN));

        final int maximumIntegerDigits = fields.get("maximumIntegerDigits", 309);
        final int minimumIntegerDigits = fields.get("minimumIntegerDigits", 309);
        final int maximumFractionDigits = fields.get("maximumFractionDigits", 340);
        final int minimumFractionDigits = fields.get("minimumFractionDigits", 340);
        // Tell ICU what we want, then ask it what we can have, and then
        // set that in our Java object. This isn't RI-compatible, but then very little of our
        // behavior in this area is, and it's not obvious how we can second-guess ICU (or tell
        // it to just do exactly what we ask). We only need to do this with maximumIntegerDigits
        // because ICU doesn't seem to have its own ideas about the other options.
        icuDecimalFormat.setMaximumIntegerDigits(maximumIntegerDigits);
        super.setMaximumIntegerDigits(icuDecimalFormat.getMaximumIntegerDigits());

        setMinimumIntegerDigits(minimumIntegerDigits);
        setMinimumFractionDigits(minimumFractionDigits);
        setMaximumFractionDigits(maximumFractionDigits);
        setParseBigDecimal(fields.get("parseBigDecimal", false));

        if (fields.get("serialVersionOnStream", 0) < 3) {
            setMaximumIntegerDigits(super.getMaximumIntegerDigits());
            setMinimumIntegerDigits(super.getMinimumIntegerDigits());
            setMaximumFractionDigits(super.getMaximumFractionDigits());
            setMinimumFractionDigits(super.getMinimumFractionDigits());
        }
    }

    //----------------------------------------------------------------------
    // INSTANCE VARIABLES
    //----------------------------------------------------------------------

    /**
     * The <code>DecimalFormatSymbols</code> object used by this format.
     * It contains the symbols used to format numbers, e.g. the grouping separator,
     * decimal separator, and so on.
     *
     * @serial
     * @see #setDecimalFormatSymbols
     * @see java.text.DecimalFormatSymbols
     */
    private DecimalFormatSymbols symbols;

    /**
     * The maximum number of digits allowed in the integer portion of a
     * <code>BigInteger</code> or <code>BigDecimal</code> number.
     * <code>maximumIntegerDigits</code> must be greater than or equal to
     * <code>minimumIntegerDigits</code>.
     *
     * @serial
     * @see #getMaximumIntegerDigits
     * @since 1.5
     */
    private int    maximumIntegerDigits;

    /**
     * The minimum number of digits allowed in the integer portion of a
     * <code>BigInteger</code> or <code>BigDecimal</code> number.
     * <code>minimumIntegerDigits</code> must be less than or equal to
     * <code>maximumIntegerDigits</code>.
     *
     * @serial
     * @see #getMinimumIntegerDigits
     * @since 1.5
     */
    private int    minimumIntegerDigits;

    /**
     * The maximum number of digits allowed in the fractional portion of a
     * <code>BigInteger</code> or <code>BigDecimal</code> number.
     * <code>maximumFractionDigits</code> must be greater than or equal to
     * <code>minimumFractionDigits</code>.
     *
     * @serial
     * @see #getMaximumFractionDigits
     * @since 1.5
     */
    private int    maximumFractionDigits;

    /**
     * The minimum number of digits allowed in the fractional portion of a
     * <code>BigInteger</code> or <code>BigDecimal</code> number.
     * <code>minimumFractionDigits</code> must be less than or equal to
     * <code>maximumFractionDigits</code>.
     *
     * @serial
     * @see #getMinimumFractionDigits
     * @since 1.5
     */
    private int    minimumFractionDigits;

    /**
     * The {@link java.math.RoundingMode} used in this DecimalFormat.
     *
     * @serial
     * @since 1.6
     */
    private RoundingMode roundingMode = RoundingMode.HALF_EVEN;



    //----------------------------------------------------------------------
    // CONSTANTS
    //----------------------------------------------------------------------

    // Upper limit on integer and fraction digits for a Java double
    static final int DOUBLE_INTEGER_DIGITS  = 309;
    static final int DOUBLE_FRACTION_DIGITS = 340;

    // Upper limit on integer and fraction digits for BigDecimal and BigInteger
    static final int MAXIMUM_INTEGER_DIGITS  = Integer.MAX_VALUE;
    static final int MAXIMUM_FRACTION_DIGITS = Integer.MAX_VALUE;

    // Proclaim JDK 1.1 serial compatibility.
    static final long serialVersionUID = 864413376551465018L;

    /**
     * Cache to hold the NumberPattern of a Locale.
     */
    private static final ConcurrentMap<Locale, String> cachedLocaleData
        = new ConcurrentHashMap<Locale, String>(3);
}
