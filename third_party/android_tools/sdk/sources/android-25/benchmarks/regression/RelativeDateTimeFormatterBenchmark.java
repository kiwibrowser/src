/*
 * Copyright (C) 2015 The Android Open Source Project
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

package benchmarks.regression;

import java.util.Locale;
import java.util.TimeZone;

import static libcore.icu.DateUtilsBridge.FORMAT_ABBREV_RELATIVE;
import static libcore.icu.RelativeDateTimeFormatter.getRelativeDateTimeString;
import static libcore.icu.RelativeDateTimeFormatter.getRelativeTimeSpanString;

public class RelativeDateTimeFormatterBenchmark {
  public void timeRelativeDateTimeFormatter_getRelativeTimeSpanString(int reps) throws Exception {
    Locale l = Locale.US;
    TimeZone utc = TimeZone.getTimeZone("Europe/London");
    int flags = 0;

    for (int rep = 0; rep < reps; ++rep) {
      getRelativeTimeSpanString(l, utc, 0L, 0L, 0L, flags);
    }
  }

  public void timeRelativeDateTimeFormatter_getRelativeTimeSpanString_ABBREV(int reps) throws Exception {
    Locale l = Locale.US;
    TimeZone utc = TimeZone.getTimeZone("UTC");
    int flags = FORMAT_ABBREV_RELATIVE;

    for (int rep = 0; rep < reps; ++rep) {
      getRelativeTimeSpanString(l, utc, 0L, 0L, 0L, flags);
    }
  }

  public void timeRelativeDateTimeFormatter_getRelativeDateTimeString(int reps) throws Exception {
    Locale l = Locale.US;
    TimeZone utc = TimeZone.getTimeZone("UTC");
    int flags = 0;

    for (int rep = 0; rep < reps; ++rep) {
      getRelativeDateTimeString(l, utc, 0L, 0L, 0L, 0L, flags);
    }
  }

  public void timeRelativeDateTimeFormatter_getRelativeDateTimeString_ABBREV(int reps) throws Exception {
    Locale l = Locale.US;
    TimeZone utc = TimeZone.getTimeZone("America/Los_Angeles");
    int flags = FORMAT_ABBREV_RELATIVE;

    for (int rep = 0; rep < reps; ++rep) {
      getRelativeDateTimeString(l, utc, 0L, 0L, 0L, 0L, flags);
    }
  }
}
