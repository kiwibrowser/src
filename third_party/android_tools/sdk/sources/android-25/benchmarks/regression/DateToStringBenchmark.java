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

package benchmarks.regression;

import android.text.format.DateFormat;
import com.google.caliper.BeforeExperiment;
import java.text.SimpleDateFormat;
import java.util.Calendar;
import java.util.Date;
import java.util.GregorianCalendar;

public final class DateToStringBenchmark {
    Date date;
    Calendar calendar;
    SimpleDateFormat format;

    @BeforeExperiment
    protected void setUp() throws Exception {
        date = new Date(0);
        calendar = new GregorianCalendar();
        calendar.setTime(date);
        format = new SimpleDateFormat("EEE MMM dd HH:mm:ss zzz yyyy");
    }

    public void timeDateToString(int reps) throws Exception {
        for (int i = 0; i < reps; ++i) {
            date.toString();
        }
    }

    public void timeDateToString_Formatter(int reps) throws Exception {
        for (int i = 0; i < reps; ++i) {
            new SimpleDateFormat("EEE MMM dd HH:mm:ss zzz yyyy").format(date);
        }
    }

    public void timeDateToString_ClonedFormatter(int reps) throws Exception {
        for (int i = 0; i < reps; ++i) {
            ((SimpleDateFormat) format.clone()).format(date);
        }
    }

    public void timeDateToString_AndroidDateFormat(int reps) {
        for (int i = 0; i < reps; i++) {
            DateFormat.format("EEE MMM dd HH:mm:ss zzz yyyy", calendar);
        }
    }
}
