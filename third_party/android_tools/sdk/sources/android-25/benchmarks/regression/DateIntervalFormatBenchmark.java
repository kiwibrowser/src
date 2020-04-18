/*
 * Copyright (C) 2013 Google Inc.
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

package benchmarks.regression;

import android.icu.util.TimeZone;
import android.icu.util.ULocale;
import libcore.icu.DateIntervalFormat;

import static libcore.icu.DateUtilsBridge.*;

public class DateIntervalFormatBenchmark {
  public void timeDateIntervalFormat_formatDateRange_DATE(int reps) throws Exception {
    ULocale l = ULocale.US;
    TimeZone utc = TimeZone.getTimeZone("UTC");
    int flags = FORMAT_SHOW_DATE | FORMAT_SHOW_WEEKDAY;

    for (int rep = 0; rep < reps; ++rep) {
      DateIntervalFormat.formatDateRange(l, utc, 0L, 0L, flags);
    }
  }

  public void timeDateIntervalFormat_formatDateRange_TIME(int reps) throws Exception {
    ULocale l = ULocale.US;
    TimeZone utc = TimeZone.getTimeZone("UTC");
    int flags = FORMAT_SHOW_TIME | FORMAT_24HOUR;

    for (int rep = 0; rep < reps; ++rep) {
      DateIntervalFormat.formatDateRange(l, utc, 0L, 0L, flags);
    }
  }

  public void timeDateIntervalFormat_formatDateRange_DATE_TIME(int reps) throws Exception {
    ULocale l = ULocale.US;
    TimeZone utc = TimeZone.getTimeZone("UTC");
    int flags = FORMAT_SHOW_DATE | FORMAT_SHOW_WEEKDAY | FORMAT_SHOW_TIME | FORMAT_24HOUR;

    for (int rep = 0; rep < reps; ++rep) {
      DateIntervalFormat.formatDateRange(l, utc, 0L, 0L, flags);
    }
  }
}
