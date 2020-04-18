/*
 * Copyright (C) 2014 The Android Open Source Project
 * Copyright (c) 2000, 2011, Oracle and/or its affiliates. All rights reserved.
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

package java.util;

import java.io.Serializable;

import libcore.icu.ICU;

/**
 * Represents a currency. Currencies are identified by their ISO 4217 currency
 * codes. Visit the <a href="http://www.iso.org/iso/en/prods-services/popstds/currencycodes.html">
 * ISO web site</a> for more information, including a table of
 * currency codes.
 * <p>
 * The class is designed so that there's never more than one
 * <code>Currency</code> instance for any given currency. Therefore, there's
 * no public constructor. You obtain a <code>Currency</code> instance using
 * the <code>getInstance</code> methods.
 * <p>
 * Users can supersede the Java runtime currency data by creating a properties
 * file named <code>&lt;JAVA_HOME&gt;/lib/currency.properties</code>.  The contents
 * of the properties file are key/value pairs of the ISO 3166 country codes
 * and the ISO 4217 currency data respectively.  The value part consists of
 * three ISO 4217 values of a currency, i.e., an alphabetic code, a numeric
 * code, and a minor unit.  Those three ISO 4217 values are separated by commas.
 * The lines which start with '#'s are considered comment lines.  For example,
 * <p>
 * <code>
 * #Sample currency properties<br>
 * JP=JPZ,999,0
 * </code>
 * <p>
 * will supersede the currency data for Japan.
 *
 * @since 1.4
 */
public final class Currency implements Serializable {

    private static final long serialVersionUID = -158308464356906721L;

    private static HashMap<String, Currency> instances = new HashMap<>();

    private static HashSet<Currency> available;

    private transient final android.icu.util.Currency icuCurrency;

    private final String currencyCode;

    /**
     * Constructs a <code>Currency</code> instance. The constructor is private
     * so that we can insure that there's never more than one instance for a
     * given currency.
     */
    private Currency(android.icu.util.Currency icuCurrency) {
        this.icuCurrency = icuCurrency;
        this.currencyCode = icuCurrency.getCurrencyCode();
    }

    /**
     * Returns the <code>Currency</code> instance for the given currency code.
     *
     * @param currencyCode the ISO 4217 code of the currency
     * @return the <code>Currency</code> instance for the given currency code
     * @exception NullPointerException if <code>currencyCode</code> is null
     * @exception IllegalArgumentException if <code>currencyCode</code> is not
     * a supported ISO 4217 code.
     */
    public static Currency getInstance(String currencyCode) {
        synchronized (instances) {
            Currency instance = instances.get(currencyCode);
            if (instance == null) {
                android.icu.util.Currency icuInstance =
                        android.icu.util.Currency.getInstance(currencyCode);
                if (icuInstance == null) {
                    return null;
                }
                instance = new Currency(icuInstance);
                instances.put(currencyCode, instance);
            }
            return instance;
        }
    }

    /**
     * Returns the <code>Currency</code> instance for the country of the
     * given locale. The language and variant components of the locale
     * are ignored. The result may vary over time, as countries change their
     * currencies. For example, for the original member countries of the
     * European Monetary Union, the method returns the old national currencies
     * until December 31, 2001, and the Euro from January 1, 2002, local time
     * of the respective countries.
     * <p>
     * The method returns <code>null</code> for territories that don't
     * have a currency, such as Antarctica.
     *
     * @param locale the locale for whose country a <code>Currency</code>
     * instance is needed
     * @return the <code>Currency</code> instance for the country of the given
     * locale, or null
     * @exception NullPointerException if <code>locale</code> or its country
     * code is null
     * @exception IllegalArgumentException if the country of the given locale
     * is not a supported ISO 3166 country code.
     */
    public static Currency getInstance(Locale locale) {
        android.icu.util.Currency icuInstance =
                android.icu.util.Currency.getInstance(locale);
        String variant = locale.getVariant();
        String country = locale.getCountry();
        if (!variant.isEmpty() && (variant.equals("EURO") || variant.equals("HK") ||
                variant.equals("PREEURO"))) {
            country = country + "_" + variant;
        }
        String currencyCode = ICU.getCurrencyCode(country);
        if (currencyCode == null) {
            throw new IllegalArgumentException("Unsupported ISO 3166 country: " + locale);
        }
        if (icuInstance == null || icuInstance.getCurrencyCode().equals("XXX")) {
            return null;
        }
        return getInstance(currencyCode);
    }

    /**
     * Gets the set of available currencies.  The returned set of currencies
     * contains all of the available currencies, which may include currencies
     * that represent obsolete ISO 4217 codes.  The set can be modified
     * without affecting the available currencies in the runtime.
     *
     * @return the set of available currencies.  If there is no currency
     *    available in the runtime, the returned set is empty.
     * @since 1.7
     */
    public static Set<Currency> getAvailableCurrencies() {
        synchronized (Currency.class) {
            if (available == null) {
                Set<android.icu.util.Currency> icuAvailableCurrencies
                        = android.icu.util.Currency.getAvailableCurrencies();
                available = new HashSet<>();
                for (android.icu.util.Currency icuCurrency : icuAvailableCurrencies) {
                    Currency currency = getInstance(icuCurrency.getCurrencyCode());
                    if (currency == null) {
                        currency = new Currency(icuCurrency);
                        instances.put(currency.currencyCode, currency);
                    }
                    available.add(currency);
                }
            }
            return (Set<Currency>) available.clone();
        }
    }

    /**
     * Gets the ISO 4217 currency code of this currency.
     *
     * @return the ISO 4217 currency code of this currency.
     */
    public String getCurrencyCode() {
        return currencyCode;
    }

    /**
     * Gets the symbol of this currency for the default locale.
     * For example, for the US Dollar, the symbol is "$" if the default
     * locale is the US, while for other locales it may be "US$". If no
     * symbol can be determined, the ISO 4217 currency code is returned.
     *
     * @return the symbol of this currency for the default locale
     */
    public String getSymbol() {
        return icuCurrency.getSymbol();
    }

    /**
     * Gets the symbol of this currency for the specified locale.
     * For example, for the US Dollar, the symbol is "$" if the specified
     * locale is the US, while for other locales it may be "US$". If no
     * symbol can be determined, the ISO 4217 currency code is returned.
     *
     * @param locale the locale for which a display name for this currency is
     * needed
     * @return the symbol of this currency for the specified locale
     * @exception NullPointerException if <code>locale</code> is null
     */
    public String getSymbol(Locale locale) {
        if (locale == null) {
            throw new NullPointerException("locale == null");
        }
        return icuCurrency.getSymbol(locale);
    }

    /**
     * Gets the default number of fraction digits used with this currency.
     * For example, the default number of fraction digits for the Euro is 2,
     * while for the Japanese Yen it's 0.
     * In the case of pseudo-currencies, such as IMF Special Drawing Rights,
     * -1 is returned.
     *
     * @return the default number of fraction digits used with this currency
     */
    public int getDefaultFractionDigits() {
        if (icuCurrency.getCurrencyCode().equals("XXX")) {
            return -1;
        }
        return icuCurrency.getDefaultFractionDigits();
    }

    /**
     * Returns the ISO 4217 numeric code of this currency.
     *
     * @return the ISO 4217 numeric code of this currency
     * @since 1.7
     */
    public int getNumericCode() {
        return icuCurrency.getNumericCode();
    }

    /**
     * Gets the name that is suitable for displaying this currency for
     * the default locale.  If there is no suitable display name found
     * for the default locale, the ISO 4217 currency code is returned.
     *
     * @return the display name of this currency for the default locale
     * @since 1.7
     */
    public String getDisplayName() {
        return icuCurrency.getDisplayName();
    }

    /**
     * Gets the name that is suitable for displaying this currency for
     * the specified locale.  If there is no suitable display name found
     * for the specified locale, the ISO 4217 currency code is returned.
     *
     * @param locale the locale for which a display name for this currency is
     * needed
     * @return the display name of this currency for the specified locale
     * @exception NullPointerException if <code>locale</code> is null
     * @since 1.7
     */
    public String getDisplayName(Locale locale) {
        return icuCurrency.getDisplayName(locale);
    }

    /**
     * Returns the ISO 4217 currency code of this currency.
     *
     * @return the ISO 4217 currency code of this currency
     */
    public String toString() {
        return icuCurrency.toString();
    }

    /**
     * Resolves instances being deserialized to a single instance per currency.
     */
    private Object readResolve() {
        return getInstance(currencyCode);
    }
}
