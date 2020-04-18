/* //device/content/providers/pim/RecurrenceProcessor.java
**
** Copyright 2006, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

package com.android.calendarcommon2;

import android.text.format.Time;
import android.util.Log;

import java.util.TreeSet;

public class RecurrenceProcessor
{
    // these are created once and reused.
    private Time mIterator = new Time(Time.TIMEZONE_UTC);
    private Time mUntil = new Time(Time.TIMEZONE_UTC);
    private StringBuilder mStringBuilder = new StringBuilder();
    private Time mGenerated = new Time(Time.TIMEZONE_UTC);
    private DaySet mDays = new DaySet(false);
    // Give up after this many loops.  This is roughly 1 second of expansion.
    private static final int MAX_ALLOWED_ITERATIONS = 2000;

    public RecurrenceProcessor()
    {
    }

    private static final String TAG = "RecurrenceProcessor";

    private static final boolean SPEW = false;

    /**
     * Returns the time (millis since epoch) of the last occurrence,
     * or -1 if the event repeats forever.  If there are no occurrences
     * (because the exrule or exdates cancel all the occurrences) and the
     * event does not repeat forever, then 0 is returned.
     *
     * This computes a conservative estimate of the last occurrence. That is,
     * the time of the actual last occurrence might be earlier than the time
     * returned by this method.
     *
     * @param dtstart the time of the first occurrence
     * @param recur the recurrence
     * @return an estimate of the time (in UTC milliseconds) of the last
     * occurrence, which may be greater than the actual last occurrence
     * @throws DateException
     */
    public long getLastOccurence(Time dtstart,
                                 RecurrenceSet recur) throws DateException {
        return getLastOccurence(dtstart, null /* no limit */, recur);
    }

    /**
     * Returns the time (millis since epoch) of the last occurrence,
     * or -1 if the event repeats forever.  If there are no occurrences
     * (because the exrule or exdates cancel all the occurrences) and the
     * event does not repeat forever, then 0 is returned.
     *
     * This computes a conservative estimate of the last occurrence. That is,
     * the time of the actual last occurrence might be earlier than the time
     * returned by this method.
     *
     * @param dtstart the time of the first occurrence
     * @param maxtime the max possible time of the last occurrence. null means no limit
     * @param recur the recurrence
     * @return an estimate of the time (in UTC milliseconds) of the last
     * occurrence, which may be greater than the actual last occurrence
     * @throws DateException
     */
    public long getLastOccurence(Time dtstart, Time maxtime,
                                 RecurrenceSet recur) throws DateException {
        long lastTime = -1;
        boolean hasCount = false;

        // first see if there are any "until"s specified.  if so, use the latest
        // until / rdate.
        if (recur.rrules != null) {
            for (EventRecurrence rrule : recur.rrules) {
                if (rrule.count != 0) {
                    hasCount = true;
                } else if (rrule.until != null) {
                    // according to RFC 2445, until must be in UTC.
                    mIterator.parse(rrule.until);
                    long untilTime = mIterator.toMillis(false /* use isDst */);
                    if (untilTime > lastTime) {
                        lastTime = untilTime;
                    }
                }
            }
            if (lastTime != -1 && recur.rdates != null) {
                for (long dt : recur.rdates) {
                    if (dt > lastTime) {
                        lastTime = dt;
                    }
                }
            }

            // If there were only "until"s and no "count"s, then return the
            // last "until" date or "rdate".
            if (lastTime != -1 && !hasCount) {
                return lastTime;
            }
        } else if (recur.rdates != null &&
                   recur.exrules == null && recur.exdates == null) {
            // if there are only rdates, we can just pick the last one.
            for (long dt : recur.rdates) {
                if (dt > lastTime) {
                    lastTime = dt;
                }
            }
            return lastTime;
        }

        // Expand the complete recurrence if there were any counts specified,
        // or if there were rdates specified.
        if (hasCount || recur.rdates != null || maxtime != null) {
            // The expansion might not contain any dates if the exrule or
            // exdates cancel all the generated dates.
            long[] dates = expand(dtstart, recur,
                    dtstart.toMillis(false /* use isDst */) /* range start */,
                    (maxtime != null) ?
                            maxtime.toMillis(false /* use isDst */) : -1 /* range end */);

            // The expansion might not contain any dates if exrule or exdates
            // cancel all the generated dates.
            if (dates.length == 0) {
                return 0;
            }
            return dates[dates.length - 1];
        }
        return -1;
    }

    /**
     * a -- list of values
     * N -- number of values to use in a
     * v -- value to check for
     */
    private static boolean listContains(int[] a, int N, int v)
    {
        for (int i=0; i<N; i++) {
            if (a[i] == v) {
                return true;
            }
        }
        return false;
    }

    /**
     * a -- list of values
     * N -- number of values to use in a
     * v -- value to check for
     * max -- if a value in a is negative, add that negative value
     *        to max and compare that instead; this is how we deal with
     *        negative numbers being offsets from the end value
     */
    private static boolean listContains(int[] a, int N, int v, int max)
    {
        for (int i=0; i<N; i++) {
            int w = a[i];
            if (w > 0) {
                if (w == v) {
                    return true;
                }
            } else {
                max += w; // w is negative
                if (max == v) {
                    return true;
                }
            }
        }
        return false;
    }

    /**
     * Filter out the ones for events whose BYxxx rule is for
     * a period greater than or equal to the period of the FREQ.
     *
     * Returns 0 if the event should not be filtered out
     * Returns something else (a rule number which is useful for debugging)
     * if the event should not be returned
     */
    private static int filter(EventRecurrence r, Time iterator)
    {
        boolean found;
        int freq = r.freq;

        if (EventRecurrence.MONTHLY >= freq) {
            // BYMONTH
            if (r.bymonthCount > 0) {
                found = listContains(r.bymonth, r.bymonthCount,
                        iterator.month + 1);
                if (!found) {
                    return 1;
                }
            }
        }
        if (EventRecurrence.WEEKLY >= freq) {
            // BYWEEK -- this is just a guess.  I wonder how many events
            // acutally use BYWEEKNO.
            if (r.byweeknoCount > 0) {
                found = listContains(r.byweekno, r.byweeknoCount,
                                iterator.getWeekNumber(),
                                iterator.getActualMaximum(Time.WEEK_NUM));
                if (!found) {
                    return 2;
                }
            }
        }
        if (EventRecurrence.DAILY >= freq) {
            // BYYEARDAY
            if (r.byyeardayCount > 0) {
                found = listContains(r.byyearday, r.byyeardayCount,
                                iterator.yearDay, iterator.getActualMaximum(Time.YEAR_DAY));
                if (!found) {
                    return 3;
                }
            }
            // BYMONTHDAY
            if (r.bymonthdayCount > 0 ) {
                found = listContains(r.bymonthday, r.bymonthdayCount,
                                iterator.monthDay,
                                iterator.getActualMaximum(Time.MONTH_DAY));
                if (!found) {
                    return 4;
                }
            }
            // BYDAY -- when filtering, we ignore the number field, because it
            // only is meaningful when creating more events.
byday:
            if (r.bydayCount > 0) {
                int a[] = r.byday;
                int N = r.bydayCount;
                int v = EventRecurrence.timeDay2Day(iterator.weekDay);
                for (int i=0; i<N; i++) {
                    if (a[i] == v) {
                        break byday;
                    }
                }
                return 5;
            }
        }
        if (EventRecurrence.HOURLY >= freq) {
            // BYHOUR
            found = listContains(r.byhour, r.byhourCount,
                            iterator.hour,
                            iterator.getActualMaximum(Time.HOUR));
            if (!found) {
                return 6;
            }
        }
        if (EventRecurrence.MINUTELY >= freq) {
            // BYMINUTE
            found = listContains(r.byminute, r.byminuteCount,
                            iterator.minute,
                            iterator.getActualMaximum(Time.MINUTE));
            if (!found) {
                return 7;
            }
        }
        if (EventRecurrence.SECONDLY >= freq) {
            // BYSECOND
            found = listContains(r.bysecond, r.bysecondCount,
                            iterator.second,
                            iterator.getActualMaximum(Time.SECOND));
            if (!found) {
                return 8;
            }
        }

        if (r.bysetposCount > 0) {
bysetpos:
            // BYSETPOS - we only handle rules like FREQ=MONTHLY;BYDAY=MO,TU,WE,TH,FR;BYSETPOS=-1
            if (freq == EventRecurrence.MONTHLY && r.bydayCount > 0) {
                // Check for stuff like BYDAY=1TU
                for (int i = r.bydayCount - 1; i >= 0; i--) {
                    if (r.bydayNum[i] != 0) {
                        if (Log.isLoggable(TAG, Log.VERBOSE)) {
                            Log.v(TAG, "BYSETPOS not supported with these rules: " + r);
                        }
                        break bysetpos;
                    }
                }
                if (!filterMonthlySetPos(r, iterator)) {
                    // Not allowed, filter it out.
                    return 9;
                }
            } else {
                if (Log.isLoggable(TAG, Log.VERBOSE)) {
                    Log.v(TAG, "BYSETPOS not supported with these rules: " + r);
                }
            }
            // BYSETPOS was defined but we don't know how to handle it.  Do no filtering based
            // on it.
        }

        // if we got to here, we didn't filter it out
        return 0;
    }

    /**
     * Filters out instances that don't match the BYSETPOS clause of a monthly recurrence rule.
     * This is an awkward and inefficient way to go about it.
     *
     * @returns true if this instance should be kept
     */
    private static boolean filterMonthlySetPos(EventRecurrence r, Time instance) {
        /*
         * Compute the day of the week for the first day of the month.  "instance" has a
         * day number and a DotW, so we compute the DotW of the 1st from that.  Note DotW
         * is 0-6, where 0=SUNDAY.
         *
         * The basic calculation is to take the instance's "day of the week" number, subtract
         * (day of the month - 1) mod 7, and then make sure it's positive.  We can simplify
         * that with some algebra.
         */
        int dotw = (instance.weekDay - instance.monthDay + 36) % 7;

        /*
         * The byday[] values are specified as bits, so we can just OR them all
         * together.
         */
        int bydayMask = 0;
        for (int i = 0; i < r.bydayCount; i++) {
            bydayMask |= r.byday[i];
        }

        /*
         * Generate a set according to the BYDAY rules.  For each day of the month, determine
         * if its day of the week is included.  If so, append it to the day set.
         */
        int maxDay = instance.getActualMaximum(Time.MONTH_DAY);
        int daySet[] = new int[maxDay];
        int daySetLength = 0;

        for (int md = 1; md <= maxDay; md++) {
            // For each month day, see if it's part of the set.  (This makes some assumptions
            // about the exact form of the DotW constants.)
            int dayBit = EventRecurrence.SU << dotw;
            if ((bydayMask & dayBit) != 0) {
                daySet[daySetLength++] = md;
            }

            dotw++;
            if (dotw == 7)
                dotw = 0;
        }

        /*
         * Now walk through the BYSETPOS list and see if the instance is equal to any of the
         * specified daySet entries.
         */
        for (int i = r.bysetposCount - 1; i >= 0; i--) {
            int index = r.bysetpos[i];
            if (index > 0) {
                if (index > daySetLength) {
                    continue;  // out of range
                }
                if (daySet[index-1] == instance.monthDay) {
                    return true;
                }
            } else if (index < 0) {
                if (daySetLength + index < 0) {
                    continue;  // out of range
                }
                if (daySet[daySetLength + index] == instance.monthDay) {
                    return true;
                }
            } else {
                // should have been caught by parser
                throw new RuntimeException("invalid bysetpos value");
            }
        }

        return false;
    }


    private static final int USE_ITERATOR = 0;
    private static final int USE_BYLIST = 1;

    /**
     * Return whether we should make this list from the BYxxx list or
     * from the component of the iterator.
     */
    int generateByList(int count, int freq, int byFreq)
    {
        if (byFreq >= freq) {
            return USE_ITERATOR;
        } else {
            if (count == 0) {
                return USE_ITERATOR;
            } else {
                return USE_BYLIST;
            }
        }
    }

    private static boolean useBYX(int freq, int freqConstant, int count)
    {
        return freq > freqConstant && count > 0;
    }

    public static class DaySet
    {
        public DaySet(boolean zulu)
        {
            mTime = new Time(Time.TIMEZONE_UTC);
        }

        void setRecurrence(EventRecurrence r)
        {
            mYear = 0;
            mMonth = -1;
            mR = r;
        }

        boolean get(Time iterator, int day)
        {
            int realYear = iterator.year;
            int realMonth = iterator.month;

            Time t = null;

            if (SPEW) {
                Log.i(TAG, "get called with iterator=" + iterator
                        + " " + iterator.month
                        + "/" + iterator.monthDay
                        + "/" + iterator.year + " day=" + day);
            }
            if (day < 1 || day > 28) {
                // if might be past the end of the month, we need to normalize it
                t = mTime;
                t.set(day, realMonth, realYear);
                unsafeNormalize(t);
                realYear = t.year;
                realMonth = t.month;
                day = t.monthDay;
                if (SPEW) {
                    Log.i(TAG, "normalized t=" + t + " " + t.month
                            + "/" + t.monthDay
                            + "/" + t.year);
                }
            }

            /*
            if (true || SPEW) {
                Log.i(TAG, "set t=" + t + " " + realMonth + "/" + day + "/" + realYear);
            }
            */
            if (realYear != mYear || realMonth != mMonth) {
                if (t == null) {
                    t = mTime;
                    t.set(day, realMonth, realYear);
                    unsafeNormalize(t);
                    if (SPEW) {
                        Log.i(TAG, "set t=" + t + " " + t.month
                                + "/" + t.monthDay
                                + "/" + t.year
                                + " realMonth=" + realMonth + " mMonth=" + mMonth);
                    }
                }
                mYear = realYear;
                mMonth = realMonth;
                mDays = generateDaysList(t, mR);
                if (SPEW) {
                    Log.i(TAG, "generated days list");
                }
            }
            return (mDays & (1<<day)) != 0;
        }

        /**
         * Fill in a bit set containing the days of the month on which this
         * will occur.
         *
         * Only call this if the r.freq > DAILY.  Otherwise, we should be
         * processing the BYDAY, BYMONTHDAY, etc. as filters instead.
         *
         * monthOffset may be -1, 0 or 1
         */
        private static int generateDaysList(Time generated, EventRecurrence r)
        {
            int days = 0;

            int i, count, v;
            int[] byday, bydayNum, bymonthday;
            int j, lastDayThisMonth;
            int first; // Time.SUNDAY, etc
            int k;

            lastDayThisMonth = generated.getActualMaximum(Time.MONTH_DAY);

            // BYDAY
            count = r.bydayCount;
            if (count > 0) {
                // calculate the day of week for the first of this month (first)
                j = generated.monthDay;
                while (j >= 8) {
                    j -= 7;
                }
                first = generated.weekDay;
                if (first >= j) {
                    first = first - j + 1;
                } else {
                    first = first - j + 8;
                }

                // What to do if the event is weekly:
                // This isn't ideal, but we'll generate a month's worth of events
                // and the code that calls this will only use the ones that matter
                // for the current week.
                byday = r.byday;
                bydayNum = r.bydayNum;
                for (i=0; i<count; i++) {
                    v = bydayNum[i];
                    j = EventRecurrence.day2TimeDay(byday[i]) - first + 1;
                    if (j <= 0) {
                        j += 7;
                    }
                    if (v == 0) {
                        // v is 0, each day in the month/week
                        for (; j<=lastDayThisMonth; j+=7) {
                            if (SPEW) Log.i(TAG, "setting " + j + " for rule "
                                    + v + "/" + EventRecurrence.day2TimeDay(byday[i]));
                            days |= 1 << j;
                        }
                    }
                    else if (v > 0) {
                        // v is positive, count from the beginning of the month
                        // -1 b/c the first one should add 0
                        j += 7*(v-1);
                        if (j <= lastDayThisMonth) {
                            if (SPEW) Log.i(TAG, "setting " + j + " for rule "
                                    + v + "/" + EventRecurrence.day2TimeDay(byday[i]));
                            // if it's impossible, we drop it
                            days |= 1 << j;
                        }
                    }
                    else {
                        // v is negative, count from the end of the month
                        // find the last one
                        for (; j<=lastDayThisMonth; j+=7) {
                        }
                        // v is negative
                        // should do +1 b/c the last one should add 0, but we also
                        // skipped the j -= 7 b/c the loop to find the last one
                        // overshot by one week
                        j += 7*v;
                        if (j >= 1) {
                            if (SPEW) Log.i(TAG, "setting " + j + " for rule "
                                    + v + "/" + EventRecurrence.day2TimeDay(byday[i]));
                            days |= 1 << j;
                        }
                    }
                }
            }

            // BYMONTHDAY
            // Q: What happens if we have BYMONTHDAY and BYDAY?
            // A: I didn't see it in the spec, so in lieu of that, we'll
            // intersect the two.  That seems reasonable to me.
            if (r.freq > EventRecurrence.WEEKLY) {
                count = r.bymonthdayCount;
                if (count != 0) {
                    bymonthday = r.bymonthday;
                    if (r.bydayCount == 0) {
                        for (i=0; i<count; i++) {
                            v = bymonthday[i];
                            if (v >= 0) {
                                days |= 1 << v;
                            } else {
                                j = lastDayThisMonth + v + 1; // v is negative
                                if (j >= 1 && j <= lastDayThisMonth) {
                                    days |= 1 << j;
                                }
                            }
                        }
                    } else {
                        // This is O(lastDayThisMonth*count), which is really
                        // O(count) with a decent sized constant.
                        for (j=1; j<=lastDayThisMonth; j++) {
                            next_day : {
                                if ((days&(1<<j)) != 0) {
                                    for (i=0; i<count; i++) {
                                        if (bymonthday[i] == j) {
                                            break next_day;
                                        }
                                    }
                                    days &= ~(1<<j);
                                }
                            }
                        }
                    }
                }
            }
            return days;
        }

        private EventRecurrence mR;
        private int mDays;
        private Time mTime;
        private int mYear;
        private int mMonth;
    }

    /**
     * Expands the recurrence within the given range using the given dtstart
     * value. Returns an array of longs where each element is a date in UTC
     * milliseconds. The return value is never null.  If there are no dates
     * then an array of length zero is returned.
     *
     * @param dtstart a Time object representing the first occurrence
     * @param recur the recurrence rules, including RRULE, RDATES, EXRULE, and
     * EXDATES
     * @param rangeStartMillis the beginning of the range to expand, in UTC
     * milliseconds
     * @param rangeEndMillis the non-inclusive end of the range to expand, in
     * UTC milliseconds; use -1 for the entire range.
     * @return an array of dates, each date is in UTC milliseconds
     * @throws DateException
     * @throws android.util.TimeFormatException if recur cannot be parsed
     */
    public long[] expand(Time dtstart,
            RecurrenceSet recur,
            long rangeStartMillis,
            long rangeEndMillis) throws DateException {
        String timezone = dtstart.timezone;
        mIterator.clear(timezone);
        mGenerated.clear(timezone);

        // We don't need to clear the mUntil (and it wouldn't do any good to
        // do so) because the "until" date string is specified in UTC and that
        // sets the timezone in the mUntil Time object.

        mIterator.set(rangeStartMillis);
        long rangeStartDateValue = normDateTimeComparisonValue(mIterator);

        long rangeEndDateValue;
        if (rangeEndMillis != -1) {
            mIterator.set(rangeEndMillis);
            rangeEndDateValue = normDateTimeComparisonValue(mIterator);
        } else {
            rangeEndDateValue = Long.MAX_VALUE;
        }

        TreeSet<Long> dtSet = new TreeSet<Long>();

        if (recur.rrules != null) {
            for (EventRecurrence rrule : recur.rrules) {
                expand(dtstart, rrule, rangeStartDateValue,
                        rangeEndDateValue, true /* add */, dtSet);
            }
        }
        if (recur.rdates != null) {
            for (long dt : recur.rdates) {
                // The dates are stored as milliseconds. We need to convert
                // them to year/month/day values in the local timezone.
                mIterator.set(dt);
                long dtvalue = normDateTimeComparisonValue(mIterator);
                dtSet.add(dtvalue);
            }
        }
        if (recur.exrules != null) {
            for (EventRecurrence exrule : recur.exrules) {
                expand(dtstart, exrule, rangeStartDateValue,
                        rangeEndDateValue, false /* remove */, dtSet);
            }
        }
        if (recur.exdates != null) {
            for (long dt : recur.exdates) {
                // The dates are stored as milliseconds. We need to convert
                // them to year/month/day values in the local timezone.
                mIterator.set(dt);
                long dtvalue = normDateTimeComparisonValue(mIterator);
                dtSet.remove(dtvalue);
            }
        }
        if (dtSet.isEmpty()) {
            // this can happen if the recurrence does not occur within the
            // expansion window.
            return new long[0];
        }

        // The values in dtSet are represented in a special form that is useful
        // for fast comparisons and that is easy to generate from year/month/day
        // values. We need to convert these to UTC milliseconds and also to
        // ensure that the dates are valid.
        int len = dtSet.size();
        long[] dates = new long[len];
        int i = 0;
        for (Long val: dtSet) {
            setTimeFromLongValue(mIterator, val);
            dates[i++] = mIterator.toMillis(true /* ignore isDst */);
        }
        return dates;
    }

    /**
     * Run the recurrence algorithm.  Processes events defined in the local
     * timezone of the event.  Return a list of iCalendar DATETIME
     * strings containing the start date/times of the occurrences; the output
     * times are defined in the local timezone of the event.
     *
     * If you want all of the events, pass Long.MAX_VALUE for rangeEndDateValue.  If you pass
     * Long.MAX_VALUE for rangeEnd, and the event doesn't have a COUNT or UNTIL field,
     * you'll get a DateException.
     *
     * @param dtstart the dtstart date as defined in RFC2445.  This
     * {@link Time} should be in the timezone of the event.
     * @param r the parsed recurrence, as defiend in RFC2445
     * @param rangeStartDateValue the first date-time you care about, inclusive
     * @param rangeEndDateValue the last date-time you care about, not inclusive (so
     *                  if you care about everything up through and including
     *                  Dec 22 1995, set last to Dec 23, 1995 00:00:00
     * @param add Whether or not we should add to out, or remove from out.
     * @param out the TreeSet you'd like to fill with the events
     * @throws DateException
     * @throws android.util.TimeFormatException if r cannot be parsed.
     */
    public void expand(Time dtstart,
            EventRecurrence r,
            long rangeStartDateValue,
            long rangeEndDateValue,
            boolean add,
            TreeSet<Long> out) throws DateException {
        unsafeNormalize(dtstart);
        long dtstartDateValue = normDateTimeComparisonValue(dtstart);
        int count = 0;

        // add the dtstart instance to the recurrence, if within range.
        // For example, if dtstart is Mar 1, 2010 and the range is Jan 1 - Apr 1,
        // then add it here and increment count.  If the range is earlier or later,
        // then don't add it here.  In that case, count will be incremented later
        // inside  the loop.  It is important that count gets incremented exactly
        // once here or in the loop for dtstart.
        //
        // NOTE: if DTSTART is not synchronized with the recurrence rule, the first instance
        //       we return will not fit the RRULE pattern.
        if (add && dtstartDateValue >= rangeStartDateValue
                && dtstartDateValue < rangeEndDateValue) {
            out.add(dtstartDateValue);
            ++count;
        }

        Time iterator = mIterator;
        Time until = mUntil;
        StringBuilder sb = mStringBuilder;
        Time generated = mGenerated;
        DaySet days = mDays;

        try {

            days.setRecurrence(r);
            if (rangeEndDateValue == Long.MAX_VALUE && r.until == null && r.count == 0) {
                throw new DateException(
                        "No range end provided for a recurrence that has no UNTIL or COUNT.");
            }

            // the top-level frequency
            int freqField;
            int freqAmount = r.interval;
            int freq = r.freq;
            switch (freq)
            {
                case EventRecurrence.SECONDLY:
                    freqField = Time.SECOND;
                    break;
                case EventRecurrence.MINUTELY:
                    freqField = Time.MINUTE;
                    break;
                case EventRecurrence.HOURLY:
                    freqField = Time.HOUR;
                    break;
                case EventRecurrence.DAILY:
                    freqField = Time.MONTH_DAY;
                    break;
                case EventRecurrence.WEEKLY:
                    freqField = Time.MONTH_DAY;
                    freqAmount = 7 * r.interval;
                    if (freqAmount <= 0) {
                        freqAmount = 7;
                    }
                    break;
                case EventRecurrence.MONTHLY:
                    freqField = Time.MONTH;
                    break;
                case EventRecurrence.YEARLY:
                    freqField = Time.YEAR;
                    break;
                default:
                    throw new DateException("bad freq=" + freq);
            }
            if (freqAmount <= 0) {
                freqAmount = 1;
            }

            int bymonthCount = r.bymonthCount;
            boolean usebymonth = useBYX(freq, EventRecurrence.MONTHLY, bymonthCount);
            boolean useDays = freq >= EventRecurrence.WEEKLY &&
                                 (r.bydayCount > 0 || r.bymonthdayCount > 0);
            int byhourCount = r.byhourCount;
            boolean usebyhour = useBYX(freq, EventRecurrence.HOURLY, byhourCount);
            int byminuteCount = r.byminuteCount;
            boolean usebyminute = useBYX(freq, EventRecurrence.MINUTELY, byminuteCount);
            int bysecondCount = r.bysecondCount;
            boolean usebysecond = useBYX(freq, EventRecurrence.SECONDLY, bysecondCount);

            // initialize the iterator
            iterator.set(dtstart);
            if (freqField == Time.MONTH) {
                if (useDays) {
                    // if it's monthly, and we're going to be generating
                    // days, set the iterator day field to 1 because sometimes
                    // we'll skip months if it's greater than 28.
                    // XXX Do we generate days for MONTHLY w/ BYHOUR?  If so,
                    // we need to do this then too.
                    iterator.monthDay = 1;
                }
            }

            long untilDateValue;
            if (r.until != null) {
                // Ensure that the "until" date string is specified in UTC.
                String untilStr = r.until;
                // 15 is length of date-time without trailing Z e.g. "20090204T075959"
                // A string such as 20090204 is a valid UNTIL (see RFC 2445) and the
                // Z should not be added.
                if (untilStr.length() == 15) {
                    untilStr = untilStr + 'Z';
                }
                // The parse() method will set the timezone to UTC
                until.parse(untilStr);

                // We need the "until" year/month/day values to be in the same
                // timezone as all the generated dates so that we can compare them
                // using the values returned by normDateTimeComparisonValue().
                until.switchTimezone(dtstart.timezone);
                untilDateValue = normDateTimeComparisonValue(until);
            } else {
                untilDateValue = Long.MAX_VALUE;
            }

            sb.ensureCapacity(15);
            sb.setLength(15); // TODO: pay attention to whether or not the event
            // is an all-day one.

            if (SPEW) {
                Log.i(TAG, "expand called w/ rangeStart=" + rangeStartDateValue
                        + " rangeEnd=" + rangeEndDateValue);
            }

            // go until the end of the range or we're done with this event
            boolean eventEnded = false;
            int failsafe = 0; // Avoid infinite loops
            events: {
                while (true) {
                    int monthIndex = 0;
                    if (failsafe++ > MAX_ALLOWED_ITERATIONS) { // Give up after about 1 second of processing
                        Log.w(TAG, "Recurrence processing stuck with r=" + r + " rangeStart="
                                  + rangeStartDateValue + " rangeEnd=" + rangeEndDateValue);
                        break;
                    }

                    unsafeNormalize(iterator);

                    int iteratorYear = iterator.year;
                    int iteratorMonth = iterator.month + 1;
                    int iteratorDay = iterator.monthDay;
                    int iteratorHour = iterator.hour;
                    int iteratorMinute = iterator.minute;
                    int iteratorSecond = iterator.second;

                    // year is never expanded -- there is no BYYEAR
                    generated.set(iterator);

                    if (SPEW) Log.i(TAG, "year=" + generated.year);

                    do { // month
                        int month = usebymonth
                                        ? r.bymonth[monthIndex]
                                        : iteratorMonth;
                        month--;
                        if (SPEW) Log.i(TAG, "  month=" + month);

                        int dayIndex = 1;
                        int lastDayToExamine = 0;

                        // Use this to handle weeks that overlap the end of the month.
                        // Keep the year and month that days is for, and generate it
                        // when needed in the loop
                        if (useDays) {
                            // Determine where to start and end, don't worry if this happens
                            // to be before dtstart or after the end, because that will be
                            // filtered in the inner loop
                            if (freq == EventRecurrence.WEEKLY) {
                                /*
                                 * iterator.weekDay indicates the day of the week (0-6, SU-SA).
                                 * Because dayIndex might start in the middle of a week, and we're
                                 * interested in treating a week as a unit, we want to move
                                 * backward to the start of the week.  (This could make the
                                 * dayIndex negative, which will be corrected by normalization
                                 * later on.)
                                 *
                                 * The day that starts the week is determined by WKST, which
                                 * defaults to MO.
                                 *
                                 * Example: dayIndex is Tuesday the 8th, and weeks start on
                                 * Thursdays.  Tuesday is day 2, Thursday is day 4, so we
                                 * want to move back (2 - 4 + 7) % 7 = 5 days to the previous
                                 * Thursday.  If weeks started on Mondays, we would only
                                 * need to move back (2 - 1 + 7) % 7 = 1 day.
                                 */
                                int weekStartAdj = (iterator.weekDay -
                                        EventRecurrence.day2TimeDay(r.wkst) + 7) % 7;
                                dayIndex = iterator.monthDay - weekStartAdj;
                                lastDayToExamine = dayIndex + 6;
                            } else {
                                lastDayToExamine = generated
                                    .getActualMaximum(Time.MONTH_DAY);
                            }
                            if (SPEW) Log.i(TAG, "dayIndex=" + dayIndex
                                    + " lastDayToExamine=" + lastDayToExamine
                                    + " days=" + days);
                        }

                        do { // day
                            int day;
                            if (useDays) {
                                if (!days.get(iterator, dayIndex)) {
                                    dayIndex++;
                                    continue;
                                } else {
                                    day = dayIndex;
                                }
                            } else {
                                day = iteratorDay;
                            }
                            if (SPEW) Log.i(TAG, "    day=" + day);

                            // hour
                            int hourIndex = 0;
                            do {
                                int hour = usebyhour
                                                ? r.byhour[hourIndex]
                                                : iteratorHour;
                                if (SPEW) Log.i(TAG, "      hour=" + hour + " usebyhour=" + usebyhour);

                                // minute
                                int minuteIndex = 0;
                                do {
                                    int minute = usebyminute
                                                    ? r.byminute[minuteIndex]
                                                    : iteratorMinute;
                                    if (SPEW) Log.i(TAG, "        minute=" + minute);

                                    // second
                                    int secondIndex = 0;
                                    do {
                                        int second = usebysecond
                                                        ? r.bysecond[secondIndex]
                                                        : iteratorSecond;
                                        if (SPEW) Log.i(TAG, "          second=" + second);

                                        // we do this here each time, because if we distribute it, we find the
                                        // month advancing extra times, as we set the month to the 32nd, 33rd, etc.
                                        // days.
                                        generated.set(second, minute, hour, day, month, iteratorYear);
                                        unsafeNormalize(generated);

                                        long genDateValue = normDateTimeComparisonValue(generated);
                                        // sometimes events get generated (BYDAY, BYHOUR, etc.) that
                                        // are before dtstart.  Filter these.  I believe this is correct,
                                        // but Google Calendar doesn't seem to always do this.
                                        if (genDateValue >= dtstartDateValue) {
                                            // filter and then add
                                            // TODO: we don't check for stop conditions (like
                                            //       passing the "end" date) unless the filter
                                            //       allows the event.  Could stop sooner.
                                            int filtered = filter(r, generated);
                                            if (0 == filtered) {

                                                // increase the count as long
                                                // as this isn't the same
                                                // as the first instance
                                                // specified by the DTSTART
                                                // (for RRULEs -- additive).
                                                // This condition must be the complement of the
                                                // condition for incrementing count at the
                                                // beginning of the method, so if we don't
                                                // increment count there, we increment it here.
                                                // For example, if add is set and dtstartDateValue
                                                // is inside the start/end range, then it was added
                                                // and count was incremented at the beginning.
                                                // If dtstartDateValue is outside the range or add
                                                // is not set, then we must increment count here.
                                                if (!(dtstartDateValue == genDateValue
                                                        && add
                                                        && dtstartDateValue >= rangeStartDateValue
                                                        && dtstartDateValue < rangeEndDateValue)) {
                                                    ++count;
                                                }
                                                // one reason we can stop is that
                                                // we're past the until date
                                                if (genDateValue > untilDateValue) {
                                                    if (SPEW) {
                                                        Log.i(TAG, "stopping b/c until="
                                                            + untilDateValue
                                                            + " generated="
                                                            + genDateValue);
                                                    }
                                                    break events;
                                                }
                                                // or we're past rangeEnd
                                                if (genDateValue >= rangeEndDateValue) {
                                                    if (SPEW) {
                                                        Log.i(TAG, "stopping b/c rangeEnd="
                                                                + rangeEndDateValue
                                                                + " generated=" + generated);
                                                    }
                                                    break events;
                                                }

                                                if (genDateValue >= rangeStartDateValue) {
                                                    if (SPEW) {
                                                        Log.i(TAG, "adding date=" + generated + " filtered=" + filtered);
                                                    }
                                                    if (add) {
                                                        out.add(genDateValue);
                                                    } else {
                                                        out.remove(genDateValue);
                                                    }
                                                }
                                                // another is that count is high enough
                                                if (r.count > 0 && r.count == count) {
                                                    //Log.i(TAG, "stopping b/c count=" + count);
                                                    break events;
                                                }
                                            }
                                        }
                                        secondIndex++;
                                    } while (usebysecond && secondIndex < bysecondCount);
                                    minuteIndex++;
                                } while (usebyminute && minuteIndex < byminuteCount);
                                hourIndex++;
                            } while (usebyhour && hourIndex < byhourCount);
                            dayIndex++;
                        } while (useDays && dayIndex <= lastDayToExamine);
                        monthIndex++;
                    } while (usebymonth && monthIndex < bymonthCount);

                    // Add freqAmount to freqField until we get another date that we want.
                    // We don't want to "generate" dates with the iterator.
                    // XXX: We do this for days, because there is a varying number of days
                    // per month
                    int oldDay = iterator.monthDay;
                    generated.set(iterator);  // just using generated as a temporary.
                    int n = 1;
                    while (true) {
                        int value = freqAmount * n;
                        switch (freqField) {
                            case Time.SECOND:
                                iterator.second += value;
                                break;
                            case Time.MINUTE:
                                iterator.minute += value;
                                break;
                            case Time.HOUR:
                                iterator.hour += value;
                                break;
                            case Time.MONTH_DAY:
                                iterator.monthDay += value;
                                break;
                            case Time.MONTH:
                                iterator.month += value;
                                break;
                            case Time.YEAR:
                                iterator.year += value;
                                break;
                            case Time.WEEK_DAY:
                                iterator.monthDay += value;
                                break;
                            case Time.YEAR_DAY:
                                iterator.monthDay += value;
                                break;
                            default:
                                throw new RuntimeException("bad field=" + freqField);
                        }

                        unsafeNormalize(iterator);
                        if (freqField != Time.YEAR && freqField != Time.MONTH) {
                            break;
                        }
                        if (iterator.monthDay == oldDay) {
                            break;
                        }
                        n++;
                        iterator.set(generated);
                    }
                }
            }
        }
        catch (DateException e) {
            Log.w(TAG, "DateException with r=" + r + " rangeStart=" + rangeStartDateValue
                    + " rangeEnd=" + rangeEndDateValue);
            throw e;
        }
        catch (RuntimeException t) {
            Log.w(TAG, "RuntimeException with r=" + r + " rangeStart=" + rangeStartDateValue
                    + " rangeEnd=" + rangeEndDateValue);
            throw t;
        }
    }

    /**
     * Normalizes the date fields to give a valid date, but if the time falls
     * in the invalid window during a transition out of Daylight Saving Time
     * when time jumps forward an hour, then the "normalized" value will be
     * invalid.
     * <p>
     * This method also computes the weekDay and yearDay fields.
     *
     * <p>
     * This method does not modify the fields isDst, or gmtOff.
     */
    static void unsafeNormalize(Time date) {
        int second = date.second;
        int minute = date.minute;
        int hour = date.hour;
        int monthDay = date.monthDay;
        int month = date.month;
        int year = date.year;

        int addMinutes = ((second < 0) ? (second - 59) : second) / 60;
        second -= addMinutes * 60;
        minute += addMinutes;
        int addHours = ((minute < 0) ? (minute - 59) : minute) / 60;
        minute -= addHours * 60;
        hour += addHours;
        int addDays = ((hour < 0) ? (hour - 23) : hour) / 24;
        hour -= addDays * 24;
        monthDay += addDays;

        // We want to make "monthDay" positive. We do this by subtracting one
        // from the year and adding a year's worth of days to "monthDay" in
        // the following loop while "monthDay" <= 0.
        while (monthDay <= 0) {
            // If month is after Feb, then add this year's length so that we
            // include this year's leap day, if any.
            // Otherwise (the month is Feb or earlier), add last year's length.
            // Subtract one from the year in either case. This gives the same
            // effective date but makes monthDay (the day of the month) much
            // larger. Eventually (usually in one iteration) monthDay will
            // be positive.
            int days = month > 1 ? yearLength(year) : yearLength(year - 1);
            monthDay += days;
            year -= 1;
        }
        // At this point, monthDay >= 1. Normalize the month to the range [0,11].
        if (month < 0) {
            int years = (month + 1) / 12 - 1;
            year += years;
            month -= 12 * years;
        } else if (month >= 12) {
            int years = month / 12;
            year += years;
            month -= 12 * years;
        }
        // At this point, month is in the range [0,11] and monthDay >= 1.
        // Now loop until the monthDay is in the correct range for the month.
        while (true) {
            // On January, check if we can jump forward a whole year.
            if (month == 0) {
                int yearLength = yearLength(year);
                if (monthDay > yearLength) {
                    year++;
                    monthDay -= yearLength;
                }
            }
            int monthLength = monthLength(year, month);
            if (monthDay > monthLength) {
                monthDay -= monthLength;
                month++;
                if (month >= 12) {
                    month -= 12;
                    year++;
                }
            } else break;
        }
        // At this point, monthDay <= the length of the current month and is
        // in the range [1,31].

        date.second = second;
        date.minute = minute;
        date.hour = hour;
        date.monthDay = monthDay;
        date.month = month;
        date.year = year;
        date.weekDay = weekDay(year, month, monthDay);
        date.yearDay = yearDay(year, month, monthDay);
    }

    /**
     * Returns true if the given year is a leap year.
     *
     * @param year the given year to test
     * @return true if the given year is a leap year.
     */
    static boolean isLeapYear(int year) {
        return (year % 4 == 0) && ((year % 100 != 0) || (year % 400 == 0));
    }

    /**
     * Returns the number of days in the given year.
     *
     * @param year the given year
     * @return the number of days in the given year.
     */
    static int yearLength(int year) {
        return isLeapYear(year) ? 366 : 365;
    }

    private static final int[] DAYS_PER_MONTH = { 31, 28, 31, 30, 31, 30, 31,
            31, 30, 31, 30, 31 };
    private static final int[] DAYS_IN_YEAR_PRECEDING_MONTH = { 0, 31, 59, 90,
        120, 151, 180, 212, 243, 273, 304, 334 };

    /**
     * Returns the number of days in the given month of the given year.
     *
     * @param year the given year.
     * @param month the given month in the range [0,11]
     * @return the number of days in the given month of the given year.
     */
    static int monthLength(int year, int month) {
        int n = DAYS_PER_MONTH[month];
        if (n != 28) {
            return n;
        }
        return isLeapYear(year) ? 29 : 28;
    }

    /**
     * Computes the weekday, a number in the range [0,6] where Sunday=0, from
     * the given year, month, and day.
     *
     * @param year the year
     * @param month the 0-based month in the range [0,11]
     * @param day the 1-based day of the month in the range [1,31]
     * @return the weekday, a number in the range [0,6] where Sunday=0
     */
    static int weekDay(int year, int month, int day) {
        if (month <= 1) {
            month += 12;
            year -= 1;
        }
        return (day + (13 * month - 14) / 5 + year + year/4 - year/100 + year/400) % 7;
    }

    /**
     * Computes the 0-based "year day", given the year, month, and day.
     *
     * @param year the year
     * @param month the 0-based month in the range [0,11]
     * @param day the 1-based day in the range [1,31]
     * @return the 0-based "year day", the number of days into the year
     */
    static int yearDay(int year, int month, int day) {
        int yearDay = DAYS_IN_YEAR_PRECEDING_MONTH[month] + day - 1;
        if (month >= 2 && isLeapYear(year)) {
            yearDay += 1;
        }
        return yearDay;
    }

    /**
     * Converts a normalized Time value to a 64-bit long. The mapping of Time
     * values to longs provides a total ordering on the Time values so that
     * two Time values can be compared efficiently by comparing their 64-bit
     * long values.  This is faster than converting the Time values to UTC
     * millliseconds.
     *
     * @param normalized a Time object whose date and time fields have been
     * normalized
     * @return a 64-bit long value that can be used for comparing and ordering
     * dates and times represented by Time objects
     */
    private static final long normDateTimeComparisonValue(Time normalized) {
        // 37 bits for the year, 4 bits for the month, 5 bits for the monthDay,
        // 5 bits for the hour, 6 bits for the minute, 6 bits for the second.
        return ((long)normalized.year << 26) + (normalized.month << 22)
                + (normalized.monthDay << 17) + (normalized.hour << 12)
                + (normalized.minute << 6) + normalized.second;
    }

    private static final void setTimeFromLongValue(Time date, long val) {
        date.year = (int) (val >> 26);
        date.month = (int) (val >> 22) & 0xf;
        date.monthDay = (int) (val >> 17) & 0x1f;
        date.hour = (int) (val >> 12) & 0x1f;
        date.minute = (int) (val >> 6) & 0x3f;
        date.second = (int) (val & 0x3f);
    }
}
