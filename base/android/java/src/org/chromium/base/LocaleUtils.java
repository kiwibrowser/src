// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.base;

import android.annotation.TargetApi;
import android.os.Build;
import android.os.LocaleList;
import android.text.TextUtils;

import org.chromium.base.annotations.CalledByNative;

import java.util.ArrayList;
import java.util.Locale;

/**
 * This class provides the locale related methods.
 */
public class LocaleUtils {
    /**
     * Guards this class from being instantiated.
     */
    private LocaleUtils() {
    }

    /**
     * Java keeps deprecated language codes for Hebrew, Yiddish and Indonesian but Chromium uses
     * updated ones. Similarly, Android uses "tl" while Chromium uses "fil" for Tagalog/Filipino.
     * So apply a mapping here.
     * See http://developer.android.com/reference/java/util/Locale.html
     * @return a updated language code for Chromium with given language string.
     */
    public static String getUpdatedLanguageForChromium(String language) {
        // IMPORTANT: Keep in sync with the mapping found in:
        // build/android/gyp/util/resource_utils.py
        switch (language) {
            case "iw":
                return "he"; // Hebrew
            case "ji":
                return "yi"; // Yiddish
            case "in":
                return "id"; // Indonesian
            case "tl":
                return "fil"; // Filipino
            default:
                return language;
        }
    }

    /**
     * @return a locale with updated language codes for Chromium, with translated modern language
     *         codes used by Chromium.
     */
    @TargetApi(Build.VERSION_CODES.LOLLIPOP)
    @VisibleForTesting
    public static Locale getUpdatedLocaleForChromium(Locale locale) {
        String language = locale.getLanguage();
        String languageForChrome = getUpdatedLanguageForChromium(language);
        if (languageForChrome.equals(language)) {
            return locale;
        }
        return new Locale.Builder().setLocale(locale).setLanguage(languageForChrome).build();
    }

    /**
     * Android uses "tl" while Chromium uses "fil" for Tagalog/Filipino.
     * So apply a mapping here.
     * See http://developer.android.com/reference/java/util/Locale.html
     * @return a updated language code for Android with given language string.
     */
    public static String getUpdatedLanguageForAndroid(String language) {
        // IMPORTANT: Keep in sync with the mapping found in:
        // build/android/gyp/util/resource_utils.py
        switch (language) {
            case "und":
                return ""; // Undefined
            case "fil":
                return "tl"; // Filipino
            default:
                return language;
        }
    }

    /**
     * @return a locale with updated language codes for Android, from translated modern language
     *         codes used by Chromium.
     */
    @TargetApi(Build.VERSION_CODES.LOLLIPOP)
    @VisibleForTesting
    public static Locale getUpdatedLocaleForAndroid(Locale locale) {
        String language = locale.getLanguage();
        String languageForAndroid = getUpdatedLanguageForAndroid(language);
        if (languageForAndroid.equals(language)) {
            return locale;
        }
        return new Locale.Builder().setLocale(locale).setLanguage(languageForAndroid).build();
    }

    /**
     * This function creates a Locale object from xx-XX style string where xx is language code
     * and XX is a country code. This works for API level lower than 21.
     * @return the locale that best represents the language tag.
     */
    public static Locale forLanguageTagCompat(String languageTag) {
        String[] tag = languageTag.split("-");
        if (tag.length == 0) {
            return new Locale("");
        }
        String language = getUpdatedLanguageForAndroid(tag[0]);
        if ((language.length() != 2 && language.length() != 3)) {
            return new Locale("");
        }
        if (tag.length == 1) {
            return new Locale(language);
        }
        String country = tag[1];
        if (country.length() != 2 && country.length() != 3) {
            return new Locale(language);
        }
        return new Locale(language, country);
    }

    /**
     * This function creates a Locale object from xx-XX style string where xx is language code
     * and XX is a country code.
     * @return the locale that best represents the language tag.
     */
    public static Locale forLanguageTag(String languageTag) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            Locale locale = Locale.forLanguageTag(languageTag);
            return getUpdatedLocaleForAndroid(locale);
        }
        return forLanguageTagCompat(languageTag);
    }

    /**
     * Converts Locale object to the BCP 47 compliant string format.
     * This works for API level lower than 24.
     *
     * Note that for Android M or before, we cannot use Locale.getLanguage() and
     * Locale.toLanguageTag() for this purpose. Since Locale.getLanguage() returns deprecated
     * language code even if the Locale object is constructed with updated language code. As for
     * Locale.toLanguageTag(), it does a special conversion from deprecated language code to updated
     * one, but it is only usable for Android N or after.
     * @return a well-formed IETF BCP 47 language tag with language and country code that
     *         represents this locale.
     */
    public static String toLanguageTag(Locale locale) {
        String language = getUpdatedLanguageForChromium(locale.getLanguage());
        String country = locale.getCountry();
        if (language.equals("no") && country.equals("NO") && locale.getVariant().equals("NY")) {
            return "nn-NO";
        }
        return country.isEmpty() ? language : language + "-" + country;
    }

    /**
     * Converts LocaleList object to the comma separated BCP 47 compliant string format.
     *
     * @return a well-formed IETF BCP 47 language tag with language and country code that
     *         represents this locale list.
     */
    @TargetApi(Build.VERSION_CODES.N)
    public static String toLanguageTags(LocaleList localeList) {
        ArrayList<String> newLocaleList = new ArrayList<>();
        for (int i = 0; i < localeList.size(); i++) {
            Locale locale = getUpdatedLocaleForChromium(localeList.get(i));
            newLocaleList.add(toLanguageTag(locale));
        }
        return TextUtils.join(",", newLocaleList);
    }

    /**
     * @return a comma separated language tags string that represents a default locale.
     *         Each language tag is well-formed IETF BCP 47 language tag with language and country
     *         code.
     */
    @CalledByNative
    public static String getDefaultLocaleString() {
        return toLanguageTag(Locale.getDefault());
    }

    /**
     * @return a comma separated language tags string that represents a default locale or locales.
     *         Each language tag is well-formed IETF BCP 47 language tag with language and country
     *         code.
     */
    public static String getDefaultLocaleListString() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N) {
            return toLanguageTags(LocaleList.getDefault());
        }
        return getDefaultLocaleString();
    }

    /**
     * @return The default country code set during install.
     */
    @CalledByNative
    private static String getDefaultCountryCode() {
        CommandLine commandLine = CommandLine.getInstance();
        return commandLine.hasSwitch(BaseSwitches.DEFAULT_COUNTRY_CODE_AT_INSTALL)
                ? commandLine.getSwitchValue(BaseSwitches.DEFAULT_COUNTRY_CODE_AT_INSTALL)
                : Locale.getDefault().getCountry();
    }

}
