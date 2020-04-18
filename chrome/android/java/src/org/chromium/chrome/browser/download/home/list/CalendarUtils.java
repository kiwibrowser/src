// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.download.home.list;

import java.util.Calendar;

/** A set of utility methods meant to make interacting with a {@link Calendar} instance easier. */
public final class CalendarUtils {
    private static Calendar sCal1;
    private static Calendar sCal2;

    private CalendarUtils() {}

    /**
     * Helper method to return the start of the day according to the localized {@link Calendar}.
     * This instance will have no hour, minute, second, etc. associated with it.  Only year, month,
     * and day of month.
     * @param t The timestamp (in milliseconds) since epoch.
     * @return  A {@link Calendar} instance representing the beginning of the day denoted by
     *          {@code t}.
     */
    public static Calendar getStartOfDay(long t) {
        ensureCalendarsAreAvailable();
        sCal1.setTimeInMillis(t);

        int year = sCal1.get(Calendar.YEAR);
        int month = sCal1.get(Calendar.MONTH);
        int day = sCal1.get(Calendar.DATE);
        sCal1.clear();
        sCal1.set(year, month, day, 0, 0, 0);

        return sCal1;
    }

    /**
     * Helper method determining whether or not two timestamps occur on the same localized calendar
     * day.
     * @param t1 A timestamp (in milliseconds) since epoch.
     * @param t2 A timestamp (in milliseconds) since epoch.
     * @return   Whether or not {@code t1} and {@code t2} represent times on the same localized
     *           calendar day.
     */
    public static boolean isSameDay(long t1, long t2) {
        ensureCalendarsAreAvailable();

        sCal1.setTimeInMillis(t1);
        sCal2.setTimeInMillis(t2);
        return compareCalendarDay(sCal1, sCal2) == 0;
    }

    private static void ensureCalendarsAreAvailable() {
        if (sCal1 == null) sCal1 = CalendarFactory.get();
        if (sCal2 == null) sCal2 = CalendarFactory.get();
    }

    private static int compareCalendarDay(Calendar cal1, Calendar cal2) {
        boolean sameDay = cal1.get(Calendar.DAY_OF_YEAR) == cal2.get(Calendar.DAY_OF_YEAR)
                && cal1.get(Calendar.YEAR) == cal2.get(Calendar.YEAR);

        if (sameDay) {
            return 0;
        } else if (cal1.before(cal2)) {
            return 1;
        } else {
            return -1;
        }
    }
}