/*
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

import java.util.Calendar;

/**
 * According to RFC2445, durations are like this:
 *       WEEKS
 *     | DAYS [ HOURS [ MINUTES [ SECONDS ] ] ]
 *     | HOURS [ MINUTES [ SECONDS ] ]
 * it doesn't specifically, say, but this sort of implies that you can't have
 * 70 seconds.
 */
public class Duration
{
    public int sign; // 1 or -1
    public int weeks;
    public int days;
    public int hours;
    public int minutes;
    public int seconds;

    public Duration()
    {
        sign = 1;
    }

    /**
     * Parse according to RFC2445 ss4.3.6.  (It's actually a little loose with
     * its parsing, for better or for worse)
     */
    public void parse(String str) throws DateException
    {
        sign = 1;
        weeks = 0;
        days = 0;
        hours = 0;
        minutes = 0;
        seconds = 0;

        int len = str.length();
        int index = 0;
        char c;

        if (len < 1) {
            return ;
        }

        c = str.charAt(0);
        if (c == '-') {
            sign = -1;
            index++;
        }
        else if (c == '+') {
            index++;
        }

        if (len < index) {
            return ;
        }

        c = str.charAt(index);
        if (c != 'P') {
            throw new DateException (
                    "Duration.parse(str='" + str + "') expected 'P' at index="
                    + index);
        }
        index++;
        c = str.charAt(index);
        if (c == 'T') {
            index++;
        }

        int n = 0;
        for (; index < len; index++) {
            c = str.charAt(index);
            if (c >= '0' && c <= '9') {
                n *= 10;
                n += ((int)(c-'0'));
            }
            else if (c == 'W') {
                weeks = n;
                n = 0;
            }
            else if (c == 'H') {
                hours = n;
                n = 0;
            }
            else if (c == 'M') {
                minutes = n;
                n = 0;
            }
            else if (c == 'S') {
                seconds = n;
                n = 0;
            }
            else if (c == 'D') {
                days = n;
                n = 0;
            }
            else if (c == 'T') {
            }
            else {
                throw new DateException (
                        "Duration.parse(str='" + str + "') unexpected char '"
                        + c + "' at index=" + index);
            }
        }
    }

    /**
     * Add this to the calendar provided, in place, in the calendar.
     */
    public void addTo(Calendar cal)
    {
        cal.add(Calendar.DAY_OF_MONTH, sign*weeks*7);
        cal.add(Calendar.DAY_OF_MONTH, sign*days);
        cal.add(Calendar.HOUR, sign*hours);
        cal.add(Calendar.MINUTE, sign*minutes);
        cal.add(Calendar.SECOND, sign*seconds);
    }

    public long addTo(long dt) {
        return dt + getMillis();
    }

    public long getMillis() {
        long factor = 1000 * sign;
        return factor * ((7*24*60*60*weeks)
                + (24*60*60*days)
                + (60*60*hours)
                + (60*minutes)
                + seconds);
    }
}
