/*
 * Copyright (C) 2013 The Android Open Source Project
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

package com.android.timezonepicker;

import android.content.Context;
import android.content.res.AssetManager;
import android.content.res.Resources;
import android.text.format.DateFormat;
import android.text.format.DateUtils;
import android.util.Log;
import android.util.SparseArray;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Date;
import java.util.HashMap;
import java.util.HashSet;
import java.util.LinkedHashMap;
import java.util.Locale;
import java.util.TimeZone;

public class TimeZoneData {
    private static final String TAG = "TimeZoneData";
    private static final boolean DEBUG = false;
    private static final int OFFSET_ARRAY_OFFSET = 20;

    private static final String PALESTINE_COUNTRY_CODE = "PS";


    ArrayList<TimeZoneInfo> mTimeZones;
    LinkedHashMap<String, ArrayList<Integer>> mTimeZonesByCountry;
    HashSet<String> mTimeZoneNames = new HashSet<String>();

    private long mTimeMillis;
    private HashMap<String, String> mCountryCodeToNameMap = new HashMap<String, String>();

    public String mDefaultTimeZoneId;
    public static boolean is24HourFormat;
    private TimeZoneInfo mDefaultTimeZoneInfo;
    private String mAlternateDefaultTimeZoneId;
    private String mDefaultTimeZoneCountry;
    private HashMap<String, TimeZoneInfo> mTimeZonesById;
    private boolean[] mHasTimeZonesInHrOffset = new boolean[40];
    SparseArray<ArrayList<Integer>> mTimeZonesByOffsets;
    private Context mContext;
    private String mPalestineDisplayName;

    public TimeZoneData(Context context, String defaultTimeZoneId, long timeMillis) {
        mContext = context;
        is24HourFormat = TimeZoneInfo.is24HourFormat = DateFormat.is24HourFormat(context);
        mDefaultTimeZoneId = mAlternateDefaultTimeZoneId = defaultTimeZoneId;
        long now = System.currentTimeMillis();

        if (timeMillis == 0) {
            mTimeMillis = now;
        } else {
            mTimeMillis = timeMillis;
        }

        mPalestineDisplayName = context.getResources().getString(R.string.palestine_display_name);

        loadTzs(context);

        Log.i(TAG, "Time to load time zones (ms): " + (System.currentTimeMillis() - now));

        // now = System.currentTimeMillis();
        // printTz();
        // Log.i(TAG, "Time to print time zones (ms): " +
        // (System.currentTimeMillis() - now));
    }

    public void setTime(long timeMillis) {
        mTimeMillis = timeMillis;
    }

    public TimeZoneInfo get(int position) {
        return mTimeZones.get(position);
    }

    public int size() {
        return mTimeZones.size();
    }

    public int getDefaultTimeZoneIndex() {
        return mTimeZones.indexOf(mDefaultTimeZoneInfo);
    }

    // TODO speed this up
    public int findIndexByTimeZoneIdSlow(String timeZoneId) {
        int idx = 0;
        for (TimeZoneInfo tzi : mTimeZones) {
            if (timeZoneId.equals(tzi.mTzId)) {
                return idx;
            }
            idx++;
        }
        return -1;
    }

    void loadTzs(Context context) {
        mTimeZones = new ArrayList<TimeZoneInfo>();
        HashSet<String> processedTimeZones = loadTzsInZoneTab(context);
        String[] tzIds = TimeZone.getAvailableIDs();

        if (DEBUG) {
            Log.e(TAG, "Available time zones: " + tzIds.length);
        }

        for (String tzId : tzIds) {
            if (processedTimeZones.contains(tzId)) {
                continue;
            }

            /*
             * Dropping non-GMT tzs without a country code. They are not really
             * needed and they are dups but missing proper country codes. e.g.
             * WET CET MST7MDT PST8PDT Asia/Khandyga Asia/Ust-Nera EST
             */
            if (!tzId.startsWith("Etc/GMT")) {
                continue;
            }

            final TimeZone tz = TimeZone.getTimeZone(tzId);
            if (tz == null) {
                Log.e(TAG, "Timezone not found: " + tzId);
                continue;
            }

            TimeZoneInfo tzInfo = new TimeZoneInfo(tz, null);

            if (getIdenticalTimeZoneInTheCountry(tzInfo) == -1) {
                if (DEBUG) {
                    Log.e(TAG, "# Adding time zone from getAvailId: " + tzInfo.toString());
                }
                mTimeZones.add(tzInfo);
            } else {
                if (DEBUG) {
                    Log.e(TAG,
                            "# Dropping identical time zone from getAvailId: " + tzInfo.toString());
                }
                continue;
            }
            //
            // TODO check for dups
            // checkForNameDups(tz, tzInfo.mCountry, false /* dls */,
            // TimeZone.SHORT, groupIdx, !found);
            // checkForNameDups(tz, tzInfo.mCountry, false /* dls */,
            // TimeZone.LONG, groupIdx, !found);
            // if (tz.useDaylightTime()) {
            // checkForNameDups(tz, tzInfo.mCountry, true /* dls */,
            // TimeZone.SHORT, groupIdx,
            // !found);
            // checkForNameDups(tz, tzInfo.mCountry, true /* dls */,
            // TimeZone.LONG, groupIdx,
            // !found);
            // }
        }

        // Don't change the order of mTimeZones after this sort
        Collections.sort(mTimeZones);

        mTimeZonesByCountry = new LinkedHashMap<String, ArrayList<Integer>>();
        mTimeZonesByOffsets = new SparseArray<ArrayList<Integer>>(mHasTimeZonesInHrOffset.length);
        mTimeZonesById = new HashMap<String, TimeZoneInfo>(mTimeZones.size());
        for (TimeZoneInfo tz : mTimeZones) {
            // /////////////////////
            // Lookup map for id -> tz
            mTimeZonesById.put(tz.mTzId, tz);
        }
        populateDisplayNameOverrides(mContext.getResources());

        Date date = new Date(mTimeMillis);
        Locale defaultLocal = Locale.getDefault();

        int idx = 0;
        for (TimeZoneInfo tz : mTimeZones) {
            // /////////////////////
            // Populate display name
            if (tz.mDisplayName == null) {
                tz.mDisplayName = tz.mTz.getDisplayName(tz.mTz.inDaylightTime(date),
                        TimeZone.LONG, defaultLocal);
            }

            // /////////////////////
            // Grouping tz's by country for search by country
            ArrayList<Integer> group = mTimeZonesByCountry.get(tz.mCountry);
            if (group == null) {
                group = new ArrayList<Integer>();
                mTimeZonesByCountry.put(tz.mCountry, group);
            }

            group.add(idx);

            // /////////////////////
            // Grouping tz's by GMT offsets
            indexByOffsets(idx, tz);

            // Skip all the GMT+xx:xx style display names from search
            if (!tz.mDisplayName.endsWith(":00")) {
                mTimeZoneNames.add(tz.mDisplayName);
            } else if (DEBUG) {
                Log.e(TAG, "# Hiding from pretty name search: " +
                        tz.mDisplayName);
            }

            idx++;
        }

        // printTimeZones();
    }

    private void printTimeZones() {
        TimeZoneInfo last = null;
        boolean first = true;
        for (TimeZoneInfo tz : mTimeZones) {
            // All
            if (false) {
                Log.e("ALL", tz.toString());
            }

            // GMT
            if (true) {
                String name = tz.mTz.getDisplayName();
                if (name.startsWith("GMT") && !tz.mTzId.startsWith("Etc/GMT")) {
                    Log.e("GMT", tz.toString());
                }
            }

            // Dups
            if (true && last != null) {
                if (last.compareTo(tz) == 0) {
                    if (first) {
                        Log.e("SAME", last.toString());
                        first = false;
                    }
                    Log.e("SAME", tz.toString());
                } else {
                    first = true;
                }
            }
            last = tz;
        }
        Log.e(TAG, "Total number of tz's = " + mTimeZones.size());
    }

    private void populateDisplayNameOverrides(Resources resources) {
        String[] ids = resources.getStringArray(R.array.timezone_rename_ids);
        String[] labels = resources.getStringArray(R.array.timezone_rename_labels);

        int length = ids.length;
        if (ids.length != labels.length) {
            Log.e(TAG, "timezone_rename_ids len=" + ids.length + " timezone_rename_labels len="
                    + labels.length);
            length = Math.min(ids.length, labels.length);
        }

        for (int i = 0; i < length; i++) {
            TimeZoneInfo tzi = mTimeZonesById.get(ids[i]);
            if (tzi != null) {
                tzi.mDisplayName = labels[i];
            } else {
                Log.e(TAG, "Could not find timezone with label: "+labels[i]);
            }
        }
    }

    public boolean hasTimeZonesInHrOffset(int offsetHr) {
        int index = OFFSET_ARRAY_OFFSET + offsetHr;
        if (index >= mHasTimeZonesInHrOffset.length || index < 0) {
            return false;
        }
        return mHasTimeZonesInHrOffset[index];
    }

    private void indexByOffsets(int idx, TimeZoneInfo tzi) {
        int offsetMillis = tzi.getNowOffsetMillis();
        int index = OFFSET_ARRAY_OFFSET + (int) (offsetMillis / DateUtils.HOUR_IN_MILLIS);
        mHasTimeZonesInHrOffset[index] = true;

        ArrayList<Integer> group = mTimeZonesByOffsets.get(index);
        if (group == null) {
            group = new ArrayList<Integer>();
            mTimeZonesByOffsets.put(index, group);
        }
        group.add(idx);
    }

    public ArrayList<Integer> getTimeZonesByOffset(int offsetHr) {
        int index = OFFSET_ARRAY_OFFSET + offsetHr;
        if (index >= mHasTimeZonesInHrOffset.length || index < 0) {
            return null;
        }
        return mTimeZonesByOffsets.get(index);
    }

    private HashSet<String> loadTzsInZoneTab(Context context) {
        HashSet<String> processedTimeZones = new HashSet<String>();
        AssetManager am = context.getAssets();
        InputStream is = null;

        /*
         * The 'backward' file contain mappings between new and old time zone
         * ids. We will explicitly ignore the old ones.
         */
        try {
            is = am.open("backward");
            BufferedReader reader = new BufferedReader(new InputStreamReader(is));
            String line;

            while ((line = reader.readLine()) != null) {
                // Skip comment lines
                if (!line.startsWith("#") && line.length() > 0) {
                    // 0: "Link"
                    // 1: New tz id
                    // Last: Old tz id
                    String[] fields = line.split("\t+");
                    String newTzId = fields[1];
                    String oldTzId = fields[fields.length - 1];

                    final TimeZone tz = TimeZone.getTimeZone(newTzId);
                    if (tz == null) {
                        Log.e(TAG, "Timezone not found: " + newTzId);
                        continue;
                    }

                    processedTimeZones.add(oldTzId);

                    if (DEBUG) {
                        Log.e(TAG, "# Dropping identical time zone from backward: " + oldTzId);
                    }

                    // Remember the cooler/newer time zone id
                    if (mDefaultTimeZoneId != null && mDefaultTimeZoneId.equals(oldTzId)) {
                        mAlternateDefaultTimeZoneId = newTzId;
                    }
                }
            }
        } catch (IOException ex) {
            Log.e(TAG, "Failed to read 'backward' file.");
        } finally {
            try {
                if (is != null) {
                    is.close();
                }
            } catch (IOException ignored) {
            }
        }

        /*
         * zone.tab contains a list of time zones and country code. They are
         * "sorted first by country, then an order within the country that (1)
         * makes some geographical sense, and (2) puts the most populous zones
         * first, where that does not contradict (1)."
         */
        try {
            String lang = Locale.getDefault().getLanguage();
            is = am.open("zone.tab");
            BufferedReader reader = new BufferedReader(new InputStreamReader(is));
            String line;
            while ((line = reader.readLine()) != null) {
                if (!line.startsWith("#")) { // Skip comment lines
                    // 0: country code
                    // 1: coordinates
                    // 2: time zone id
                    // 3: comments
                    final String[] fields = line.split("\t");
                    final String timeZoneId = fields[2];
                    final String countryCode = fields[0];
                    final TimeZone tz = TimeZone.getTimeZone(timeZoneId);
                    if (tz == null) {
                        Log.e(TAG, "Timezone not found: " + timeZoneId);
                        continue;
                    }

                    /*
                     * Dropping non-GMT tzs without a country code. They are not
                     * really needed and they are dups but missing proper
                     * country codes. e.g. WET CET MST7MDT PST8PDT Asia/Khandyga
                     * Asia/Ust-Nera EST
                     */
                    if (countryCode == null && !timeZoneId.startsWith("Etc/GMT")) {
                        processedTimeZones.add(timeZoneId);
                        continue;
                    }

                    // Remember the mapping between the country code and display
                    // name
                    String country = mCountryCodeToNameMap.get(countryCode);
                    if (country == null) {
                        country = getCountryNames(lang, countryCode);
                        mCountryCodeToNameMap.put(countryCode, country);
                    }

                    // TODO Don't like this here but need to get the country of
                    // the default tz.

                    // Find the country of the default tz
                    if (mDefaultTimeZoneId != null && mDefaultTimeZoneCountry == null
                            && timeZoneId.equals(mAlternateDefaultTimeZoneId)) {
                        mDefaultTimeZoneCountry = country;
                        TimeZone defaultTz = TimeZone.getTimeZone(mDefaultTimeZoneId);
                        if (defaultTz != null) {
                            mDefaultTimeZoneInfo = new TimeZoneInfo(defaultTz, country);

                            int tzToOverride = getIdenticalTimeZoneInTheCountry(mDefaultTimeZoneInfo);
                            if (tzToOverride == -1) {
                                if (DEBUG) {
                                    Log.e(TAG, "Adding default time zone: "
                                            + mDefaultTimeZoneInfo.toString());
                                }
                                mTimeZones.add(mDefaultTimeZoneInfo);
                            } else {
                                mTimeZones.add(tzToOverride, mDefaultTimeZoneInfo);
                                if (DEBUG) {
                                    TimeZoneInfo tzInfoToOverride = mTimeZones.get(tzToOverride);
                                    String tzIdToOverride = tzInfoToOverride.mTzId;
                                    Log.e(TAG, "Replaced by default tz: "
                                            + tzInfoToOverride.toString());
                                    Log.e(TAG, "Adding default time zone: "
                                            + mDefaultTimeZoneInfo.toString());
                                }
                            }
                        }
                    }

                    // Add to the list of time zones if the time zone is unique
                    // in the given country.
                    TimeZoneInfo timeZoneInfo = new TimeZoneInfo(tz, country);
                    int identicalTzIdx = getIdenticalTimeZoneInTheCountry(timeZoneInfo);
                    if (identicalTzIdx == -1) {
                        if (DEBUG) {
                            Log.e(TAG, "# Adding time zone: " + timeZoneId + " ## " +
                                    tz.getDisplayName());
                        }
                        mTimeZones.add(timeZoneInfo);
                    } else {
                        if (DEBUG) {
                            Log.e(TAG, "# Dropping identical time zone: " + timeZoneId + " ## " +
                                    tz.getDisplayName());
                        }
                    }
                    processedTimeZones.add(timeZoneId);
                }
            }

        } catch (IOException ex) {
            Log.e(TAG, "Failed to read 'zone.tab'.");
        } finally {
            try {
                if (is != null) {
                    is.close();
                }
            } catch (IOException ignored) {
            }
        }

        return processedTimeZones;
    }

    private static Locale mBackupCountryLocale;
    private static String[] mBackupCountryCodes;
    private static String[] mBackupCountryNames;

    private String getCountryNames(String lang, String countryCode) {
        final Locale defaultLocale = Locale.getDefault();
        String countryDisplayName;
        if (PALESTINE_COUNTRY_CODE.equalsIgnoreCase(countryCode)) {
            countryDisplayName = mPalestineDisplayName;
        } else {
            countryDisplayName = new Locale(lang, countryCode).getDisplayCountry(defaultLocale);
        }

        if (!countryCode.equals(countryDisplayName)) {
            return countryDisplayName;
        }

        if (mBackupCountryCodes == null || !defaultLocale.equals(mBackupCountryLocale)) {
            mBackupCountryLocale = defaultLocale;
            mBackupCountryCodes = mContext.getResources().getStringArray(
                    R.array.backup_country_codes);
            mBackupCountryNames = mContext.getResources().getStringArray(
                    R.array.backup_country_names);
        }

        int length = Math.min(mBackupCountryCodes.length, mBackupCountryNames.length);

        for (int i = 0; i < length; i++) {
            if (mBackupCountryCodes[i].equals(countryCode)) {
                return mBackupCountryNames[i];
            }
        }

        return countryCode;
    }

    private int getIdenticalTimeZoneInTheCountry(TimeZoneInfo timeZoneInfo) {
        int idx = 0;
        for (TimeZoneInfo tzi : mTimeZones) {
            if (tzi.hasSameRules(timeZoneInfo)) {
                if (tzi.mCountry == null) {
                    if (timeZoneInfo.mCountry == null) {
                        return idx;
                    }
                } else if (tzi.mCountry.equals(timeZoneInfo.mCountry)) {
                    return idx;
                }
            }
            ++idx;
        }
        return -1;
    }
}
