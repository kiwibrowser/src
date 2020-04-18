/*
 * Copyright (C) 2010 Google Inc.
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

package com.google.i18n.addressinput.common;

import java.util.HashMap;
import java.util.Locale;
import java.util.Map;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * Utility functions used by the address widget.
 */
public final class Util {
  /**
   * This variable is in upper-case, since we convert the language code to upper case before doing
   * string comparison.
   */
  private static final String LATIN_SCRIPT = "LATN";

  /**
   * Map of countries that have non-latin local names, with the language that their local names
   * are in. We only list a country here if we have the appropriate data. Only language sub-tags
   * are listed.
   * TODO(user): Delete this: the information should be read from RegionDataConstants.java.
   */
  private static final Map<String, String> nonLatinLocalLanguageCountries =
      new HashMap<String, String>();
  static {
    nonLatinLocalLanguageCountries.put("AE", "ar");
    nonLatinLocalLanguageCountries.put("AM", "hy");
    nonLatinLocalLanguageCountries.put("CN", "zh");
    nonLatinLocalLanguageCountries.put("EG", "ar");
    nonLatinLocalLanguageCountries.put("HK", "zh");
    nonLatinLocalLanguageCountries.put("JP", "ja");
    nonLatinLocalLanguageCountries.put("KP", "ko");
    nonLatinLocalLanguageCountries.put("KR", "ko");
    nonLatinLocalLanguageCountries.put("MO", "zh");
    nonLatinLocalLanguageCountries.put("RU", "ru");
    nonLatinLocalLanguageCountries.put("TH", "th");
    nonLatinLocalLanguageCountries.put("TW", "zh");
    nonLatinLocalLanguageCountries.put("UA", "uk");
    nonLatinLocalLanguageCountries.put("VN", "vi");
  }

  /**
   * Cannot instantiate this class - private constructor.
   */
  private Util() {
  }

  /**
   * Returns true if the language code is explicitly marked to be in the latin script. For
   * example, "zh-Latn" would return true, but "zh-TW", "en" and "zh" would all return false.
   */
  public static boolean isExplicitLatinScript(String languageCode) {
    // Convert to upper-case for easier comparison.
    languageCode = toUpperCaseLocaleIndependent(languageCode);
    // Check to see if the language code contains a script modifier.
    final Pattern languageCodePattern = Pattern.compile("\\w{2,3}[-_](\\w{4})");
    Matcher m = languageCodePattern.matcher(languageCode);
    if (m.lookingAt()) {
      String script = m.group(1);
      if (script.equals(LATIN_SCRIPT)) {
        return true;
      }
    }
    return false;
  }

  /**
   * Returns the language subtag of a language code. For example, returns "zh" if given "zh-Hans",
   * "zh-CN" or other "zh" variants. If no language subtag can be found or the language tag is
   * malformed, returns "und".
   */
  public static String getLanguageSubtag(String languageCode) {
    final Pattern languageCodePattern = Pattern.compile("(\\w{2,3})(?:[-_]\\w{4})?(?:[-_]\\w{2})?");
    Matcher m = languageCodePattern.matcher(languageCode);
    if (m.matches()) {
      return toLowerCaseLocaleIndependent(m.group(1));
    }
    return "und";
  }

  /**
   * Trims the string. If the field is empty after trimming, returns null instead. Note that this
   * only trims ASCII white-space.
   */
  static String trimToNull(String originalStr) {
    if (originalStr == null) {
      return null;
    }
    String trimmedString = originalStr.trim();
    return (trimmedString.length() == 0) ? null : trimmedString;
  }

  /**
   * Throws an exception if the object is null, with a generic error message.
   */
  static <T> T checkNotNull(T o) {
    return checkNotNull(o, "This object should not be null.");
  }

  /**
   * Throws an exception if the object is null, with the error message supplied.
   */
  static <T> T checkNotNull(T o, String message) {
    if (o == null) {
      throw new NullPointerException(message);
    }
    return o;
  }

  /**
   * Joins input string with the given separator. If an input string is null, it will be skipped.
   */
  static String joinAndSkipNulls(String separator, String... strings) {
    StringBuilder sb = null;
    for (String s : strings) {
      if (s != null) {
        s = s.trim();
        if (s.length() > 0) {
          if (sb == null) {
            sb = new StringBuilder(s);
          } else {
            sb.append(separator).append(s);
          }
        }
      }
    }
    return sb == null ? null : sb.toString();
  }

  /**
   * Builds a map of the lower-cased values of the keys, names and local names provided. Each name
   * and local name is mapped to its respective key in the map.
   *
   * @throws IllegalStateException if the names or lnames array is greater than the keys array.
   */
  static Map<String, String> buildNameToKeyMap(String[] keys, String[] names, String[] lnames) {
    if (keys == null) {
      return null;
    }

    Map<String, String> nameToKeyMap = new HashMap<String, String>();

    int keyLength = keys.length;
    for (String k : keys) {
      nameToKeyMap.put(toLowerCaseLocaleIndependent(k), k);
    }
    if (names != null) {
      if (names.length > keyLength) {
        throw new IllegalStateException("names length (" + names.length
            + ") is greater than keys length (" + keys.length + ")");
      }
      for (int i = 0; i < keyLength; i++) {
        // If we have less names than keys, we ignore all missing names. This happens
        // generally because reg-ex splitting methods on different platforms (java, js etc)
        // behave differently in the default case. Since missing names are fine, we opt to
        // be more robust here.
        if (i < names.length && names[i].length() > 0) {
          nameToKeyMap.put(toLowerCaseLocaleIndependent(names[i]), keys[i]);
        }
      }
    }
    if (lnames != null) {
      if (lnames.length > keyLength) {
        throw new IllegalStateException("lnames length (" + lnames.length
            + ") is greater than keys length (" + keys.length + ")");
      }
      for (int i = 0; i < keyLength; i++) {
        if (i < lnames.length && lnames[i].length() > 0) {
          nameToKeyMap.put(toLowerCaseLocaleIndependent(lnames[i]), keys[i]);
        }
      }
    }
    return nameToKeyMap;
  }

  /**
   * Returns a language code that the widget can use when fetching data, based on a {@link
   * java.util.Locale} language and the current selected country in the address widget. This
   * method is necessary since we have to determine later whether a language is "local" or "latin"
   * for certain countries.
   *
   * @param language the current user language
   * @param currentCountry the current selected country
   * @return a language code string in BCP-47 format (e.g. "en", "zh-Latn", "zh-Hans" or
   * "en-US").
   */
  public static String getWidgetCompatibleLanguageCode(Locale language, String currentCountry) {
    String country = toUpperCaseLocaleIndependent(currentCountry);
    // Only do something if the country is one of those where we have names in the local
    // language as well as in latin script.
    if (nonLatinLocalLanguageCountries.containsKey(country)) {
      String languageTag = language.getLanguage();
      // Only do something if the language tag is _not_ the local language.
      if (!languageTag.equals(nonLatinLocalLanguageCountries.get(country))) {
        // Build up the language tag with the country and language specified, and add in the
        // script-tag of "Latn" explicitly, since this is _not_ a local language. This means
        // that we might create a language tag of "th-Latn", which is not what the actual
        // language being used is, but it indicates that we prefer "Latn" names to whatever
        // the local alternative was.
        StringBuilder languageTagBuilder = new StringBuilder(languageTag);
        languageTagBuilder.append("_latn");
        if (language.getCountry().length() > 0) {
          languageTagBuilder.append("_");
          languageTagBuilder.append(language.getCountry());
        }
        return languageTagBuilder.toString();
      }
    }
    return language.toString();
  }

  /**
   * Converts all of the characters in this String to lower case using the rules of English. This is
   * equivalent to calling toLowerCase(Locale.ENGLISH). Thus avoiding locale-sensitive case folding
   * such as the Turkish i, which could mess e.g. with lookup keys and country codes.
   */
  public static String toLowerCaseLocaleIndependent(String value) {
    return (value != null) ? value.toLowerCase(Locale.ENGLISH) : null;
  }

  /**
   * Converts all of the characters in this String to upper case using the rules of English. This is
   * equivalent to calling toUpperCase(Locale.ENGLISH). Thus avoiding locale-sensitive case folding
   * such as the Turkish i, which could mess e.g. with lookup keys and country codes.
   */
  public static String toUpperCaseLocaleIndependent(String value) {
    return (value != null) ? value.toUpperCase(Locale.ENGLISH) : null;
  }
}
