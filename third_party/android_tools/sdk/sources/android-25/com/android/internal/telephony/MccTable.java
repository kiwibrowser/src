/*
 * Copyright (C) 2006 The Android Open Source Project
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

package com.android.internal.telephony;

import android.app.ActivityManagerNative;
import android.app.AlarmManager;
import android.content.Context;
import android.content.res.Configuration;
import android.net.wifi.WifiManager;
import android.os.Build;
import android.os.RemoteException;
import android.os.SystemProperties;
import android.telephony.TelephonyManager;
import android.text.TextUtils;
import android.util.Slog;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Locale;
import java.util.Map;

import libcore.icu.ICU;
import libcore.icu.TimeZoneNames;

/**
 * Mobile Country Code
 *
 * {@hide}
 */
public final class MccTable {
    static final String LOG_TAG = "MccTable";

    static ArrayList<MccEntry> sTable;

    static class MccEntry implements Comparable<MccEntry> {
        final int mMcc;
        final String mIso;
        final int mSmallestDigitsMnc;

        MccEntry(int mnc, String iso, int smallestDigitsMCC) {
            if (iso == null) {
                throw new NullPointerException();
            }
            mMcc = mnc;
            mIso = iso;
            mSmallestDigitsMnc = smallestDigitsMCC;
        }

        @Override
        public int compareTo(MccEntry o) {
            return mMcc - o.mMcc;
        }
    }

    private static MccEntry entryForMcc(int mcc) {
        MccEntry m = new MccEntry(mcc, "", 0);

        int index = Collections.binarySearch(sTable, m);

        if (index < 0) {
            return null;
        } else {
            return sTable.get(index);
        }
    }

    /**
     * Returns a default time zone ID for the given MCC.
     * @param mcc Mobile Country Code
     * @return default TimeZone ID, or null if not specified
     */
    public static String defaultTimeZoneForMcc(int mcc) {
        MccEntry entry = entryForMcc(mcc);
        if (entry == null) {
            return null;
        }
        Locale locale = new Locale("", entry.mIso);
        String[] tz = TimeZoneNames.forLocale(locale);
        if (tz.length == 0) return null;
        return tz[0];
    }

    /**
     * Given a GSM Mobile Country Code, returns
     * an ISO two-character country code if available.
     * Returns "" if unavailable.
     */
    public static String countryCodeForMcc(int mcc) {
        MccEntry entry = entryForMcc(mcc);

        if (entry == null) {
            return "";
        } else {
            return entry.mIso;
        }
    }

    /**
     * Given a GSM Mobile Country Code, returns
     * an ISO 2-3 character language code if available.
     * Returns null if unavailable.
     */
    public static String defaultLanguageForMcc(int mcc) {
        MccEntry entry = entryForMcc(mcc);
        if (entry == null) {
            Slog.d(LOG_TAG, "defaultLanguageForMcc(" + mcc + "): no country for mcc");
            return null;
        }

        final String country = entry.mIso;

        // Choose English as the default language for India.
        if ("in".equals(country)) {
            return "en";
        }

        // Ask CLDR for the language this country uses...
        Locale likelyLocale = ICU.addLikelySubtags(new Locale("und", country));
        String likelyLanguage = likelyLocale.getLanguage();
        Slog.d(LOG_TAG, "defaultLanguageForMcc(" + mcc + "): country " + country + " uses " +
               likelyLanguage);
        return likelyLanguage;
    }

    /**
     * Given a GSM Mobile Country Code, returns
     * the smallest number of digits that M if available.
     * Returns 2 if unavailable.
     */
    public static int smallestDigitsMccForMnc(int mcc) {
        MccEntry entry = entryForMcc(mcc);

        if (entry == null) {
            return 2;
        } else {
            return entry.mSmallestDigitsMnc;
        }
    }

    /**
     * Updates MCC and MNC device configuration information for application retrieving
     * correct version of resources.  If MCC is 0, MCC and MNC will be ignored (not set).
     * @param context Context to act on.
     * @param mccmnc truncated imsi with just the MCC and MNC - MNC assumed to be from 4th to end
     * @param fromServiceState true if coming from the radio service state, false if from SIM
     */
    public static void updateMccMncConfiguration(Context context, String mccmnc,
            boolean fromServiceState) {
        Slog.d(LOG_TAG, "updateMccMncConfiguration mccmnc='" + mccmnc + "' fromServiceState=" + fromServiceState);

        if (Build.IS_DEBUGGABLE) {
            String overrideMcc = SystemProperties.get("persist.sys.override_mcc");
            if (!TextUtils.isEmpty(overrideMcc)) {
                mccmnc = overrideMcc;
                Slog.d(LOG_TAG, "updateMccMncConfiguration overriding mccmnc='" + mccmnc + "'");
            }
        }

        if (!TextUtils.isEmpty(mccmnc)) {
            int mcc, mnc;

            String defaultMccMnc = TelephonyManager.getDefault().getSimOperatorNumeric();
            Slog.d(LOG_TAG, "updateMccMncConfiguration defaultMccMnc=" + defaultMccMnc);
            //Update mccmnc only for default subscription in case of MultiSim.
//            if (!defaultMccMnc.equals(mccmnc)) {
//                Slog.d(LOG_TAG, "Not a Default subscription, ignoring mccmnc config update.");
//                return;
//            }

            try {
                mcc = Integer.parseInt(mccmnc.substring(0,3));
                mnc = Integer.parseInt(mccmnc.substring(3));
            } catch (NumberFormatException e) {
                Slog.e(LOG_TAG, "Error parsing IMSI: " + mccmnc);
                return;
            }

            Slog.d(LOG_TAG, "updateMccMncConfiguration: mcc=" + mcc + ", mnc=" + mnc);
            if (mcc != 0) {
                setTimezoneFromMccIfNeeded(context, mcc);
            }
            if (fromServiceState) {
                setWifiCountryCodeFromMcc(context, mcc);
            } else {
                // from SIM
                try {
                    Configuration config = new Configuration();
                    boolean updateConfig = false;
                    if (mcc != 0) {
                        config.mcc = mcc;
                        config.mnc = mnc == 0 ? Configuration.MNC_ZERO : mnc;
                        updateConfig = true;
                    }

                    if (updateConfig) {
                        Slog.d(LOG_TAG, "updateMccMncConfiguration updateConfig config=" + config);
                        ActivityManagerNative.getDefault().updateConfiguration(config);
                    } else {
                        Slog.d(LOG_TAG, "updateMccMncConfiguration nothing to update");
                    }
                } catch (RemoteException e) {
                    Slog.e(LOG_TAG, "Can't update configuration", e);
                }
            }
        } else {
            if (fromServiceState) {
                // an empty mccmnc means no signal - tell wifi we don't know
                setWifiCountryCodeFromMcc(context, 0);
            }
        }
    }

    /**
     * Maps a given locale to a fallback locale that approximates it. This is a hack.
     */
    private static final Map<Locale, Locale> FALLBACKS = new HashMap<Locale, Locale>();

    static {
        // If we have English (without a country) explicitly prioritize en_US. http://b/28998094
        FALLBACKS.put(Locale.ENGLISH, Locale.US);
        FALLBACKS.put(Locale.CANADA, Locale.US);
    }

    /**
     * Find the best match we actually have a localization for. This function assumes we
     * couldn't find an exact match.
     *
     * TODO: This should really follow the CLDR chain of parent locales! That might be a bit
     * of a problem because we don't really have an en-001 locale on android.
     */
    private static Locale chooseBestFallback(Locale target, List<Locale> candidates) {
        if (candidates.isEmpty()) {
            return null;
        }

        Locale fallback = target;
        while ((fallback = FALLBACKS.get(fallback)) != null) {
            if (candidates.contains(fallback)) {
                return fallback;
            }
        }

        // Somewhat arbitrarily take the first locale for the language,
        // unless we get a perfect match later. Note that these come back in no
        // particular order, so there's no reason to think the first match is
        // a particularly good match.
        return candidates.get(0);
    }

    /**
     * Return Locale for the language and country or null if no good match.
     *
     * @param context Context to act on.
     * @param language Two character language code desired
     * @param country Two character country code desired
     *
     * @return Locale or null if no appropriate value
     */
    private static Locale getLocaleForLanguageCountry(Context context, String language,
            String country) {
        if (language == null) {
            Slog.d(LOG_TAG, "getLocaleForLanguageCountry: skipping no language");
            return null; // no match possible
        }
        if (country == null) {
            country = ""; // The Locale constructor throws if passed null.
        }

        final Locale target = new Locale(language, country);
        try {
            String[] localeArray = context.getAssets().getLocales();
            List<String> locales = new ArrayList<>(Arrays.asList(localeArray));

            // Even in developer mode, you don't want the pseudolocales.
            locales.remove("ar-XB");
            locales.remove("en-XA");

            List<Locale> languageMatches = new ArrayList<>();
            for (String locale : locales) {
                final Locale l = Locale.forLanguageTag(locale.replace('_', '-'));

                // Only consider locales with both language and country.
                if (l == null || "und".equals(l.getLanguage()) ||
                        l.getLanguage().isEmpty() || l.getCountry().isEmpty()) {
                    continue;
                }
                if (l.getLanguage().equals(target.getLanguage())) {
                    // If we got a perfect match, we're done.
                    if (l.getCountry().equals(target.getCountry())) {
                        Slog.d(LOG_TAG, "getLocaleForLanguageCountry: got perfect match: " +
                               l.toLanguageTag());
                        return l;
                    }

                    // We've only matched the language, not the country.
                    languageMatches.add(l);
                }
            }

            Locale bestMatch = chooseBestFallback(target, languageMatches);
            if (bestMatch != null) {
                Slog.d(LOG_TAG, "getLocaleForLanguageCountry: got a language-only match: " +
                       bestMatch.toLanguageTag());
                return bestMatch;
            } else {
                Slog.d(LOG_TAG, "getLocaleForLanguageCountry: no locales for language " +
                       language);
            }
        } catch (Exception e) {
            Slog.d(LOG_TAG, "getLocaleForLanguageCountry: exception", e);
        }

        return null;
    }

    /**
     * If the timezone is not already set, set it based on the MCC of the SIM.
     * @param context Context to act on.
     * @param mcc Mobile Country Code of the SIM or SIM-like entity (build prop on CDMA)
     */
    private static void setTimezoneFromMccIfNeeded(Context context, int mcc) {
        String timezone = SystemProperties.get(ServiceStateTracker.TIMEZONE_PROPERTY);
        if (timezone == null || timezone.length() == 0) {
            String zoneId = defaultTimeZoneForMcc(mcc);
            if (zoneId != null && zoneId.length() > 0) {
                // Set time zone based on MCC
                AlarmManager alarm =
                        (AlarmManager) context.getSystemService(Context.ALARM_SERVICE);
                alarm.setTimeZone(zoneId);
                Slog.d(LOG_TAG, "timezone set to "+zoneId);
            }
        }
    }

    /**
     * Get Locale based on the MCC of the SIM.
     *
     * @param context Context to act on.
     * @param mcc Mobile Country Code of the SIM or SIM-like entity (build prop on CDMA)
     * @param simLanguage (nullable) the language from the SIM records (if present).
     *
     * @return locale for the mcc or null if none
     */
    public static Locale getLocaleFromMcc(Context context, int mcc, String simLanguage) {
        String language = (simLanguage == null) ? MccTable.defaultLanguageForMcc(mcc) : simLanguage;
        String country = MccTable.countryCodeForMcc(mcc);

        Slog.d(LOG_TAG, "getLocaleFromMcc(" + language + ", " + country + ", " + mcc);
        final Locale locale = getLocaleForLanguageCountry(context, language, country);

        // If we couldn't find a locale that matches the SIM language, give it a go again
        // with the "likely" language for the given country.
        if (locale == null && simLanguage != null) {
            language = MccTable.defaultLanguageForMcc(mcc);
            Slog.d(LOG_TAG, "[retry ] getLocaleFromMcc(" + language + ", " + country + ", " + mcc);
            return getLocaleForLanguageCountry(context, null, country);
        }

        return locale;
    }

    /**
     * Set the country code for wifi.  This sets allowed wifi channels based on the
     * country of the carrier we see.  If we can't see any, reset to 0 so we don't
     * broadcast on forbidden channels.
     * @param context Context to act on.
     * @param mcc Mobile Country Code of the operator.  0 if not known
     */
    private static void setWifiCountryCodeFromMcc(Context context, int mcc) {
        String country = MccTable.countryCodeForMcc(mcc);
        Slog.d(LOG_TAG, "WIFI_COUNTRY_CODE set to " + country);
        WifiManager wM = (WifiManager) context.getSystemService(Context.WIFI_SERVICE);
        wM.setCountryCode(country, false);
    }

    static {
        sTable = new ArrayList<MccEntry>(240);


        /*
         * The table below is built from two resources:
         *
         * 1) ITU "Mobile Network Code (MNC) for the international
         *   identification plan for mobile terminals and mobile users"
         *   which is available as an annex to the ITU operational bulletin
         *   available here: http://www.itu.int/itu-t/bulletin/annex.html
         *
         * 2) The ISO 3166 country codes list, available here:
         *    http://www.iso.org/iso/en/prods-services/iso3166ma/02iso-3166-code-lists/index.html
         *
         * This table has not been verified.
         */

		sTable.add(new MccEntry(202,"gr",2));	//Greece
		sTable.add(new MccEntry(204,"nl",2));	//Netherlands (Kingdom of the)
		sTable.add(new MccEntry(206,"be",2));	//Belgium
		sTable.add(new MccEntry(208,"fr",2));	//France
		sTable.add(new MccEntry(212,"mc",2));	//Monaco (Principality of)
		sTable.add(new MccEntry(213,"ad",2));	//Andorra (Principality of)
		sTable.add(new MccEntry(214,"es",2));	//Spain
		sTable.add(new MccEntry(216,"hu",2));	//Hungary (Republic of)
		sTable.add(new MccEntry(218,"ba",2));	//Bosnia and Herzegovina
		sTable.add(new MccEntry(219,"hr",2));	//Croatia (Republic of)
		sTable.add(new MccEntry(220,"rs",2));	//Serbia and Montenegro
		sTable.add(new MccEntry(222,"it",2));	//Italy
		sTable.add(new MccEntry(225,"va",2));	//Vatican City State
		sTable.add(new MccEntry(226,"ro",2));	//Romania
		sTable.add(new MccEntry(228,"ch",2));	//Switzerland (Confederation of)
		sTable.add(new MccEntry(230,"cz",2));	//Czech Republic
		sTable.add(new MccEntry(231,"sk",2));	//Slovak Republic
		sTable.add(new MccEntry(232,"at",2));	//Austria
		sTable.add(new MccEntry(234,"gb",2));	//United Kingdom of Great Britain and Northern Ireland
		sTable.add(new MccEntry(235,"gb",2));	//United Kingdom of Great Britain and Northern Ireland
		sTable.add(new MccEntry(238,"dk",2));	//Denmark
		sTable.add(new MccEntry(240,"se",2));	//Sweden
		sTable.add(new MccEntry(242,"no",2));	//Norway
		sTable.add(new MccEntry(244,"fi",2));	//Finland
		sTable.add(new MccEntry(246,"lt",2));	//Lithuania (Republic of)
		sTable.add(new MccEntry(247,"lv",2));	//Latvia (Republic of)
		sTable.add(new MccEntry(248,"ee",2));	//Estonia (Republic of)
		sTable.add(new MccEntry(250,"ru",2));	//Russian Federation
		sTable.add(new MccEntry(255,"ua",2));	//Ukraine
		sTable.add(new MccEntry(257,"by",2));	//Belarus (Republic of)
		sTable.add(new MccEntry(259,"md",2));	//Moldova (Republic of)
		sTable.add(new MccEntry(260,"pl",2));	//Poland (Republic of)
		sTable.add(new MccEntry(262,"de",2));	//Germany (Federal Republic of)
		sTable.add(new MccEntry(266,"gi",2));	//Gibraltar
		sTable.add(new MccEntry(268,"pt",2));	//Portugal
		sTable.add(new MccEntry(270,"lu",2));	//Luxembourg
		sTable.add(new MccEntry(272,"ie",2));	//Ireland
		sTable.add(new MccEntry(274,"is",2));	//Iceland
		sTable.add(new MccEntry(276,"al",2));	//Albania (Republic of)
		sTable.add(new MccEntry(278,"mt",2));	//Malta
		sTable.add(new MccEntry(280,"cy",2));	//Cyprus (Republic of)
		sTable.add(new MccEntry(282,"ge",2));	//Georgia
		sTable.add(new MccEntry(283,"am",2));	//Armenia (Republic of)
		sTable.add(new MccEntry(284,"bg",2));	//Bulgaria (Republic of)
		sTable.add(new MccEntry(286,"tr",2));	//Turkey
		sTable.add(new MccEntry(288,"fo",2));	//Faroe Islands
                sTable.add(new MccEntry(289,"ge",2));    //Abkhazia (Georgia)
		sTable.add(new MccEntry(290,"gl",2));	//Greenland (Denmark)
		sTable.add(new MccEntry(292,"sm",2));	//San Marino (Republic of)
		sTable.add(new MccEntry(293,"si",2));	//Slovenia (Republic of)
                sTable.add(new MccEntry(294,"mk",2));   //The Former Yugoslav Republic of Macedonia
		sTable.add(new MccEntry(295,"li",2));	//Liechtenstein (Principality of)
                sTable.add(new MccEntry(297,"me",2));    //Montenegro (Republic of)
		sTable.add(new MccEntry(302,"ca",3));	//Canada
		sTable.add(new MccEntry(308,"pm",2));	//Saint Pierre and Miquelon (Collectivit territoriale de la Rpublique franaise)
		sTable.add(new MccEntry(310,"us",3));	//United States of America
		sTable.add(new MccEntry(311,"us",3));	//United States of America
		sTable.add(new MccEntry(312,"us",3));	//United States of America
		sTable.add(new MccEntry(313,"us",3));	//United States of America
		sTable.add(new MccEntry(314,"us",3));	//United States of America
		sTable.add(new MccEntry(315,"us",3));	//United States of America
		sTable.add(new MccEntry(316,"us",3));	//United States of America
		sTable.add(new MccEntry(330,"pr",2));	//Puerto Rico
		sTable.add(new MccEntry(332,"vi",2));	//United States Virgin Islands
		sTable.add(new MccEntry(334,"mx",3));	//Mexico
		sTable.add(new MccEntry(338,"jm",3));	//Jamaica
		sTable.add(new MccEntry(340,"gp",2));	//Guadeloupe (French Department of)
		sTable.add(new MccEntry(342,"bb",3));	//Barbados
		sTable.add(new MccEntry(344,"ag",3));	//Antigua and Barbuda
		sTable.add(new MccEntry(346,"ky",3));	//Cayman Islands
		sTable.add(new MccEntry(348,"vg",3));	//British Virgin Islands
		sTable.add(new MccEntry(350,"bm",2));	//Bermuda
		sTable.add(new MccEntry(352,"gd",2));	//Grenada
		sTable.add(new MccEntry(354,"ms",2));	//Montserrat
		sTable.add(new MccEntry(356,"kn",2));	//Saint Kitts and Nevis
		sTable.add(new MccEntry(358,"lc",2));	//Saint Lucia
		sTable.add(new MccEntry(360,"vc",2));	//Saint Vincent and the Grenadines
		sTable.add(new MccEntry(362,"ai",2));	//Netherlands Antilles
		sTable.add(new MccEntry(363,"aw",2));	//Aruba
		sTable.add(new MccEntry(364,"bs",2));	//Bahamas (Commonwealth of the)
		sTable.add(new MccEntry(365,"ai",3));	//Anguilla
		sTable.add(new MccEntry(366,"dm",2));	//Dominica (Commonwealth of)
		sTable.add(new MccEntry(368,"cu",2));	//Cuba
		sTable.add(new MccEntry(370,"do",2));	//Dominican Republic
		sTable.add(new MccEntry(372,"ht",2));	//Haiti (Republic of)
		sTable.add(new MccEntry(374,"tt",2));	//Trinidad and Tobago
		sTable.add(new MccEntry(376,"tc",2));	//Turks and Caicos Islands
		sTable.add(new MccEntry(400,"az",2));	//Azerbaijani Republic
		sTable.add(new MccEntry(401,"kz",2));	//Kazakhstan (Republic of)
		sTable.add(new MccEntry(402,"bt",2));	//Bhutan (Kingdom of)
		sTable.add(new MccEntry(404,"in",2));	//India (Republic of)
		sTable.add(new MccEntry(405,"in",2));	//India (Republic of)
		sTable.add(new MccEntry(406,"in",2));	//India (Republic of)
		sTable.add(new MccEntry(410,"pk",2));	//Pakistan (Islamic Republic of)
		sTable.add(new MccEntry(412,"af",2));	//Afghanistan
		sTable.add(new MccEntry(413,"lk",2));	//Sri Lanka (Democratic Socialist Republic of)
		sTable.add(new MccEntry(414,"mm",2));	//Myanmar (Union of)
		sTable.add(new MccEntry(415,"lb",2));	//Lebanon
		sTable.add(new MccEntry(416,"jo",2));	//Jordan (Hashemite Kingdom of)
		sTable.add(new MccEntry(417,"sy",2));	//Syrian Arab Republic
		sTable.add(new MccEntry(418,"iq",2));	//Iraq (Republic of)
		sTable.add(new MccEntry(419,"kw",2));	//Kuwait (State of)
		sTable.add(new MccEntry(420,"sa",2));	//Saudi Arabia (Kingdom of)
		sTable.add(new MccEntry(421,"ye",2));	//Yemen (Republic of)
		sTable.add(new MccEntry(422,"om",2));	//Oman (Sultanate of)
                sTable.add(new MccEntry(423,"ps",2));    //Palestine
		sTable.add(new MccEntry(424,"ae",2));	//United Arab Emirates
		sTable.add(new MccEntry(425,"il",2));	//Israel (State of)
		sTable.add(new MccEntry(426,"bh",2));	//Bahrain (Kingdom of)
		sTable.add(new MccEntry(427,"qa",2));	//Qatar (State of)
		sTable.add(new MccEntry(428,"mn",2));	//Mongolia
		sTable.add(new MccEntry(429,"np",2));	//Nepal
		sTable.add(new MccEntry(430,"ae",2));	//United Arab Emirates
		sTable.add(new MccEntry(431,"ae",2));	//United Arab Emirates
		sTable.add(new MccEntry(432,"ir",2));	//Iran (Islamic Republic of)
		sTable.add(new MccEntry(434,"uz",2));	//Uzbekistan (Republic of)
		sTable.add(new MccEntry(436,"tj",2));	//Tajikistan (Republic of)
		sTable.add(new MccEntry(437,"kg",2));	//Kyrgyz Republic
		sTable.add(new MccEntry(438,"tm",2));	//Turkmenistan
		sTable.add(new MccEntry(440,"jp",2));	//Japan
		sTable.add(new MccEntry(441,"jp",2));	//Japan
		sTable.add(new MccEntry(450,"kr",2));	//Korea (Republic of)
		sTable.add(new MccEntry(452,"vn",2));	//Viet Nam (Socialist Republic of)
		sTable.add(new MccEntry(454,"hk",2));	//"Hong Kong, China"
		sTable.add(new MccEntry(455,"mo",2));	//"Macao, China"
		sTable.add(new MccEntry(456,"kh",2));	//Cambodia (Kingdom of)
		sTable.add(new MccEntry(457,"la",2));	//Lao People's Democratic Republic
		sTable.add(new MccEntry(460,"cn",2));	//China (People's Republic of)
		sTable.add(new MccEntry(461,"cn",2));	//China (People's Republic of)
		sTable.add(new MccEntry(466,"tw",2));	//Taiwan
		sTable.add(new MccEntry(467,"kp",2));	//Democratic People's Republic of Korea
		sTable.add(new MccEntry(470,"bd",2));	//Bangladesh (People's Republic of)
		sTable.add(new MccEntry(472,"mv",2));	//Maldives (Republic of)
		sTable.add(new MccEntry(502,"my",2));	//Malaysia
		sTable.add(new MccEntry(505,"au",2));	//Australia
		sTable.add(new MccEntry(510,"id",2));	//Indonesia (Republic of)
		sTable.add(new MccEntry(514,"tl",2));	//Democratic Republic of Timor-Leste
		sTable.add(new MccEntry(515,"ph",2));	//Philippines (Republic of the)
		sTable.add(new MccEntry(520,"th",2));	//Thailand
		sTable.add(new MccEntry(525,"sg",2));	//Singapore (Republic of)
		sTable.add(new MccEntry(528,"bn",2));	//Brunei Darussalam
		sTable.add(new MccEntry(530,"nz",2));	//New Zealand
		sTable.add(new MccEntry(534,"mp",2));	//Northern Mariana Islands (Commonwealth of the)
		sTable.add(new MccEntry(535,"gu",2));	//Guam
		sTable.add(new MccEntry(536,"nr",2));	//Nauru (Republic of)
		sTable.add(new MccEntry(537,"pg",2));	//Papua New Guinea
		sTable.add(new MccEntry(539,"to",2));	//Tonga (Kingdom of)
		sTable.add(new MccEntry(540,"sb",2));	//Solomon Islands
		sTable.add(new MccEntry(541,"vu",2));	//Vanuatu (Republic of)
		sTable.add(new MccEntry(542,"fj",2));	//Fiji (Republic of)
		sTable.add(new MccEntry(543,"wf",2));	//Wallis and Futuna (Territoire franais d'outre-mer)
		sTable.add(new MccEntry(544,"as",2));	//American Samoa
		sTable.add(new MccEntry(545,"ki",2));	//Kiribati (Republic of)
		sTable.add(new MccEntry(546,"nc",2));	//New Caledonia (Territoire franais d'outre-mer)
		sTable.add(new MccEntry(547,"pf",2));	//French Polynesia (Territoire franais d'outre-mer)
		sTable.add(new MccEntry(548,"ck",2));	//Cook Islands
		sTable.add(new MccEntry(549,"ws",2));	//Samoa (Independent State of)
		sTable.add(new MccEntry(550,"fm",2));	//Micronesia (Federated States of)
		sTable.add(new MccEntry(551,"mh",2));	//Marshall Islands (Republic of the)
		sTable.add(new MccEntry(552,"pw",2));	//Palau (Republic of)
		sTable.add(new MccEntry(553,"tv",2));	//Tuvalu
		sTable.add(new MccEntry(555,"nu",2));	//Niue
		sTable.add(new MccEntry(602,"eg",2));	//Egypt (Arab Republic of)
		sTable.add(new MccEntry(603,"dz",2));	//Algeria (People's Democratic Republic of)
		sTable.add(new MccEntry(604,"ma",2));	//Morocco (Kingdom of)
		sTable.add(new MccEntry(605,"tn",2));	//Tunisia
		sTable.add(new MccEntry(606,"ly",2));	//Libya (Socialist People's Libyan Arab Jamahiriya)
		sTable.add(new MccEntry(607,"gm",2));	//Gambia (Republic of the)
		sTable.add(new MccEntry(608,"sn",2));	//Senegal (Republic of)
		sTable.add(new MccEntry(609,"mr",2));	//Mauritania (Islamic Republic of)
		sTable.add(new MccEntry(610,"ml",2));	//Mali (Republic of)
		sTable.add(new MccEntry(611,"gn",2));	//Guinea (Republic of)
		sTable.add(new MccEntry(612,"ci",2));	//Côte d'Ivoire (Republic of)
		sTable.add(new MccEntry(613,"bf",2));	//Burkina Faso
		sTable.add(new MccEntry(614,"ne",2));	//Niger (Republic of the)
		sTable.add(new MccEntry(615,"tg",2));	//Togolese Republic
		sTable.add(new MccEntry(616,"bj",2));	//Benin (Republic of)
		sTable.add(new MccEntry(617,"mu",2));	//Mauritius (Republic of)
		sTable.add(new MccEntry(618,"lr",2));	//Liberia (Republic of)
		sTable.add(new MccEntry(619,"sl",2));	//Sierra Leone
		sTable.add(new MccEntry(620,"gh",2));	//Ghana
		sTable.add(new MccEntry(621,"ng",2));	//Nigeria (Federal Republic of)
		sTable.add(new MccEntry(622,"td",2));	//Chad (Republic of)
		sTable.add(new MccEntry(623,"cf",2));	//Central African Republic
		sTable.add(new MccEntry(624,"cm",2));	//Cameroon (Republic of)
		sTable.add(new MccEntry(625,"cv",2));	//Cape Verde (Republic of)
		sTable.add(new MccEntry(626,"st",2));	//Sao Tome and Principe (Democratic Republic of)
		sTable.add(new MccEntry(627,"gq",2));	//Equatorial Guinea (Republic of)
		sTable.add(new MccEntry(628,"ga",2));	//Gabonese Republic
		sTable.add(new MccEntry(629,"cg",2));	//Congo (Republic of the)
		sTable.add(new MccEntry(630,"cd",2));	//Democratic Republic of the Congo
		sTable.add(new MccEntry(631,"ao",2));	//Angola (Republic of)
		sTable.add(new MccEntry(632,"gw",2));	//Guinea-Bissau (Republic of)
		sTable.add(new MccEntry(633,"sc",2));	//Seychelles (Republic of)
		sTable.add(new MccEntry(634,"sd",2));	//Sudan (Republic of the)
		sTable.add(new MccEntry(635,"rw",2));	//Rwanda (Republic of)
		sTable.add(new MccEntry(636,"et",2));	//Ethiopia (Federal Democratic Republic of)
		sTable.add(new MccEntry(637,"so",2));	//Somali Democratic Republic
		sTable.add(new MccEntry(638,"dj",2));	//Djibouti (Republic of)
		sTable.add(new MccEntry(639,"ke",2));	//Kenya (Republic of)
		sTable.add(new MccEntry(640,"tz",2));	//Tanzania (United Republic of)
		sTable.add(new MccEntry(641,"ug",2));	//Uganda (Republic of)
		sTable.add(new MccEntry(642,"bi",2));	//Burundi (Republic of)
		sTable.add(new MccEntry(643,"mz",2));	//Mozambique (Republic of)
		sTable.add(new MccEntry(645,"zm",2));	//Zambia (Republic of)
		sTable.add(new MccEntry(646,"mg",2));	//Madagascar (Republic of)
		sTable.add(new MccEntry(647,"re",2));	//Reunion (French Department of)
		sTable.add(new MccEntry(648,"zw",2));	//Zimbabwe (Republic of)
		sTable.add(new MccEntry(649,"na",2));	//Namibia (Republic of)
		sTable.add(new MccEntry(650,"mw",2));	//Malawi
		sTable.add(new MccEntry(651,"ls",2));	//Lesotho (Kingdom of)
		sTable.add(new MccEntry(652,"bw",2));	//Botswana (Republic of)
		sTable.add(new MccEntry(653,"sz",2));	//Swaziland (Kingdom of)
		sTable.add(new MccEntry(654,"km",2));	//Comoros (Union of the)
		sTable.add(new MccEntry(655,"za",2));	//South Africa (Republic of)
		sTable.add(new MccEntry(657,"er",2));	//Eritrea
		sTable.add(new MccEntry(658,"sh",2));	//Saint Helena, Ascension and Tristan da Cunha
		sTable.add(new MccEntry(659,"ss",2));	//South Sudan (Republic of)
		sTable.add(new MccEntry(702,"bz",2));	//Belize
		sTable.add(new MccEntry(704,"gt",2));	//Guatemala (Republic of)
		sTable.add(new MccEntry(706,"sv",2));	//El Salvador (Republic of)
		sTable.add(new MccEntry(708,"hn",3));	//Honduras (Republic of)
		sTable.add(new MccEntry(710,"ni",2));	//Nicaragua
		sTable.add(new MccEntry(712,"cr",2));	//Costa Rica
		sTable.add(new MccEntry(714,"pa",2));	//Panama (Republic of)
		sTable.add(new MccEntry(716,"pe",2));	//Peru
		sTable.add(new MccEntry(722,"ar",3));	//Argentine Republic
		sTable.add(new MccEntry(724,"br",2));	//Brazil (Federative Republic of)
		sTable.add(new MccEntry(730,"cl",2));	//Chile
		sTable.add(new MccEntry(732,"co",3));	//Colombia (Republic of)
		sTable.add(new MccEntry(734,"ve",2));	//Venezuela (Bolivarian Republic of)
		sTable.add(new MccEntry(736,"bo",2));	//Bolivia (Republic of)
		sTable.add(new MccEntry(738,"gy",2));	//Guyana
		sTable.add(new MccEntry(740,"ec",2));	//Ecuador
		sTable.add(new MccEntry(742,"gf",2));	//French Guiana (French Department of)
		sTable.add(new MccEntry(744,"py",2));	//Paraguay (Republic of)
		sTable.add(new MccEntry(746,"sr",2));	//Suriname (Republic of)
		sTable.add(new MccEntry(748,"uy",2));	//Uruguay (Eastern Republic of)
		sTable.add(new MccEntry(750,"fk",2));	//Falkland Islands (Malvinas)
        //table.add(new MccEntry(901,"",2));	//"International Mobile, shared code"

        Collections.sort(sTable);
    }
}
