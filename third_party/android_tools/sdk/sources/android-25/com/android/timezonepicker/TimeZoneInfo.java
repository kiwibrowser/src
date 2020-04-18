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
import android.text.Spannable;
import android.text.Spannable.Factory;
import android.text.format.DateUtils;
import android.text.format.Time;
import android.text.style.ForegroundColorSpan;
import android.util.Log;
import android.util.SparseArray;

import java.lang.reflect.Field;
import java.text.DateFormat;
import java.util.Arrays;
import java.util.Date;
import java.util.Formatter;
import java.util.Locale;
import java.util.TimeZone;

public class TimeZoneInfo implements Comparable<TimeZoneInfo> {
    private static final int GMT_TEXT_COLOR = TimeZonePickerUtils.GMT_TEXT_COLOR;
    private static final int DST_SYMBOL_COLOR = TimeZonePickerUtils.DST_SYMBOL_COLOR;
    private static final char SEPARATOR = ',';
    private static final String TAG = null;
    public static int NUM_OF_TRANSITIONS = 6;
    public static long time = System.currentTimeMillis() / 1000;
    public static boolean is24HourFormat;
    private static final Factory mSpannableFactory = Spannable.Factory.getInstance();

    TimeZone mTz;
    public String mTzId;
    int mRawoffset;
    public String mCountry;
    public int groupId;
    public String mDisplayName;
    private Time recycledTime = new Time();
    private static StringBuilder mSB = new StringBuilder(50);
    private static Formatter mFormatter = new Formatter(mSB, Locale.getDefault());

    public TimeZoneInfo(TimeZone tz, String country) {
        mTz = tz;
        mTzId = tz.getID();
        mCountry = country;
        mRawoffset = tz.getRawOffset();
    }

    SparseArray<String> mLocalTimeCache = new SparseArray<String>();
    long mLocalTimeCacheReferenceTime = 0;
    static private long mGmtDisplayNameUpdateTime;
    static private SparseArray<CharSequence> mGmtDisplayNameCache =
            new SparseArray<CharSequence>();

    public String getLocalTime(long referenceTime) {
        recycledTime.timezone = TimeZone.getDefault().getID();
        recycledTime.set(referenceTime);

        int currYearDay = recycledTime.year * 366 + recycledTime.yearDay;

        recycledTime.timezone = mTzId;
        recycledTime.set(referenceTime);

        String localTimeStr = null;

        int hourMinute = recycledTime.hour * 60 +
                recycledTime.minute;

        if (mLocalTimeCacheReferenceTime != referenceTime) {
            mLocalTimeCacheReferenceTime = referenceTime;
            mLocalTimeCache.clear();
        } else {
            localTimeStr = mLocalTimeCache.get(hourMinute);
        }

        if (localTimeStr == null) {
            String format = "%I:%M %p";
            if (currYearDay != (recycledTime.year * 366 + recycledTime.yearDay)) {
                if (is24HourFormat) {
                    format = "%b %d %H:%M";
                } else {
                    format = "%b %d %I:%M %p";
                }
            } else if (is24HourFormat) {
                format = "%H:%M";
            }

            // format = "%Y-%m-%d %H:%M";
            localTimeStr = recycledTime.format(format);
            mLocalTimeCache.put(hourMinute, localTimeStr);
        }

        return localTimeStr;
    }

    public int getLocalHr(long referenceTime) {
        recycledTime.timezone = mTzId;
        recycledTime.set(referenceTime);
        return recycledTime.hour;
    }

    public int getNowOffsetMillis() {
        return mTz.getOffset(System.currentTimeMillis());
    }

    /*
     * The method is synchronized because there's one mSB, which is used by
     * mFormatter, per instance. If there are multiple callers for
     * getGmtDisplayName, the output may be mangled.
     */
    public synchronized CharSequence getGmtDisplayName(Context context) {
        // TODO Note: The local time is shown in current time (current GMT
        // offset) which may be different from the time specified by
        // mTimeMillis

        final long nowMinute = System.currentTimeMillis() / DateUtils.MINUTE_IN_MILLIS;
        final long now = nowMinute * DateUtils.MINUTE_IN_MILLIS;
        final int gmtOffset = mTz.getOffset(now);
        int cacheKey;

        boolean hasFutureDST = mTz.useDaylightTime();
        if (hasFutureDST) {
            cacheKey = (int) (gmtOffset + 36 * DateUtils.HOUR_IN_MILLIS);
        } else {
            cacheKey = (int) (gmtOffset - 36 * DateUtils.HOUR_IN_MILLIS);
        }

        CharSequence displayName = null;
        if (mGmtDisplayNameUpdateTime != nowMinute) {
            mGmtDisplayNameUpdateTime = nowMinute;
            mGmtDisplayNameCache.clear();
        } else {
            displayName = mGmtDisplayNameCache.get(cacheKey);
        }

        if (displayName == null) {
            mSB.setLength(0);
            int flags = DateUtils.FORMAT_ABBREV_ALL;
            flags |= DateUtils.FORMAT_SHOW_TIME;
            if (TimeZoneInfo.is24HourFormat) {
                flags |= DateUtils.FORMAT_24HOUR;
            }

            // mFormatter writes to mSB
            DateUtils.formatDateRange(context, mFormatter, now, now, flags, mTzId);
            mSB.append("  ");
            int gmtStart = mSB.length();
            TimeZonePickerUtils.appendGmtOffset(mSB, gmtOffset);
            int gmtEnd = mSB.length();

            int symbolStart = 0;
            int symbolEnd = 0;
            if (hasFutureDST) {
                mSB.append(' ');
                symbolStart = mSB.length();
                mSB.append(TimeZonePickerUtils.getDstSymbol()); // Sun symbol
                symbolEnd = mSB.length();
            }

            // Set the gray colors.
            Spannable spannableText = mSpannableFactory.newSpannable(mSB);
            spannableText.setSpan(new ForegroundColorSpan(GMT_TEXT_COLOR),
                    gmtStart, gmtEnd, Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);

            if (hasFutureDST) {
                spannableText.setSpan(new ForegroundColorSpan(DST_SYMBOL_COLOR),
                        symbolStart, symbolEnd, Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
            }
            displayName = spannableText;
            mGmtDisplayNameCache.put(cacheKey, displayName);
        }
        return displayName;
    }

    public boolean hasSameRules(TimeZoneInfo tzi) {
        return this.mTz.hasSameRules(tzi.mTz);
    }

    @Override
    public String toString() {
        StringBuilder sb = new StringBuilder();

        final String country = this.mCountry;
        final TimeZone tz = this.mTz;

        sb.append(mTzId);
        sb.append(SEPARATOR);
        sb.append(tz.getDisplayName(false /* daylightTime */, TimeZone.LONG));
        sb.append(SEPARATOR);
        sb.append(tz.getDisplayName(false /* daylightTime */, TimeZone.SHORT));
        sb.append(SEPARATOR);
        if (tz.useDaylightTime()) {
            sb.append(tz.getDisplayName(true, TimeZone.LONG));
            sb.append(SEPARATOR);
            sb.append(tz.getDisplayName(true, TimeZone.SHORT));
        } else {
            sb.append(SEPARATOR);
        }
        sb.append(SEPARATOR);
        sb.append(tz.getRawOffset() / 3600000f);
        sb.append(SEPARATOR);
        sb.append(tz.getDSTSavings() / 3600000f);
        sb.append(SEPARATOR);
        sb.append(country);
        sb.append(SEPARATOR);

        // 1-1-2013 noon GMT
        sb.append(getLocalTime(1357041600000L));
        sb.append(SEPARATOR);

        // 3-15-2013 noon GMT
        sb.append(getLocalTime(1363348800000L));
        sb.append(SEPARATOR);

        // 7-1-2013 noon GMT
        sb.append(getLocalTime(1372680000000L));
        sb.append(SEPARATOR);

        // 11-01-2013 noon GMT
        sb.append(getLocalTime(1383307200000L));
        sb.append(SEPARATOR);

        sb.append('\n');
        return sb.toString();
    }

    private static String formatTime(DateFormat df, int s) {
        long ms = s * 1000L;
        return df.format(new Date(ms));
    }

    /*
     * Returns a negative integer if this instance is less than the other; a
     * positive integer if this instance is greater than the other; 0 if this
     * instance has the same order as the other.
     */
    @Override
    public int compareTo(TimeZoneInfo other) {
        if (this.getNowOffsetMillis() != other.getNowOffsetMillis()) {
            return (other.getNowOffsetMillis() < this.getNowOffsetMillis()) ? -1 : 1;
        }

        // By country
        if (this.mCountry == null) {
            if (other.mCountry != null) {
                return 1;
            }
        }

        if (other.mCountry == null) {
            return -1;
        } else {
            int diff = this.mCountry.compareTo(other.mCountry);

            if (diff != 0) {
                return diff;
            }
        }

        // Finally diff by display name
        if (mDisplayName != null && other.mDisplayName != null)
            return this.mDisplayName.compareTo(other.mDisplayName);

        return this.mTz.getDisplayName(Locale.getDefault()).compareTo(
                other.mTz.getDisplayName(Locale.getDefault()));

    }
}
