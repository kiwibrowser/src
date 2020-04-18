/*
 * Copyright (C) 2009 Google Inc.
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

import java.text.Collator;
import java.text.DateFormat;
import java.text.DateFormatSymbols;
import java.text.DecimalFormatSymbols;
import java.text.NumberFormat;
import java.text.SimpleDateFormat;
import java.util.GregorianCalendar;
import java.util.Locale;

/**
 * Benchmarks creation and cloning various expensive objects.
 */
public class ExpensiveObjectsBenchmark {
    public void timeNewDateFormatTimeInstance(int reps) {
        for (int i = 0; i < reps; ++i) {
            DateFormat df = DateFormat.getTimeInstance(DateFormat.SHORT);
            df.format(System.currentTimeMillis());
        }
    }

    public void timeClonedDateFormatTimeInstance(int reps) {
        DateFormat df = DateFormat.getTimeInstance(DateFormat.SHORT);
        for (int i = 0; i < reps; ++i) {
            ((DateFormat) df.clone()).format(System.currentTimeMillis());
        }
    }

    public void timeReusedDateFormatTimeInstance(int reps) {
        DateFormat df = DateFormat.getTimeInstance(DateFormat.SHORT);
        for (int i = 0; i < reps; ++i) {
            synchronized (df) {
                df.format(System.currentTimeMillis());
            }
        }
    }

    public void timeNewCollator(int reps) {
        for (int i = 0; i < reps; ++i) {
            Collator.getInstance(Locale.US);
        }
    }

    public void timeClonedCollator(int reps) {
        Collator c = Collator.getInstance(Locale.US);
        for (int i = 0; i < reps; ++i) {
            c.clone();
        }
    }

    public void timeNewDateFormatSymbols(int reps) {
        for (int i = 0; i < reps; ++i) {
            new DateFormatSymbols(Locale.US);
        }
    }

    public void timeClonedDateFormatSymbols(int reps) {
        DateFormatSymbols dfs = new DateFormatSymbols(Locale.US);
        for (int i = 0; i < reps; ++i) {
            dfs.clone();
        }
    }

    public void timeNewDecimalFormatSymbols(int reps) {
        for (int i = 0; i < reps; ++i) {
            new DecimalFormatSymbols(Locale.US);
        }
    }

    public void timeClonedDecimalFormatSymbols(int reps) {
        DecimalFormatSymbols dfs = new DecimalFormatSymbols(Locale.US);
        for (int i = 0; i < reps; ++i) {
            dfs.clone();
        }
    }

    public void timeNewNumberFormat(int reps) {
        for (int i = 0; i < reps; ++i) {
            NumberFormat.getInstance(Locale.US);
        }
    }

    public void timeClonedNumberFormat(int reps) {
        NumberFormat nf = NumberFormat.getInstance(Locale.US);
        for (int i = 0; i < reps; ++i) {
            nf.clone();
        }
    }

    public void timeNumberFormatTrivialFormatLong(int reps) {
        NumberFormat nf = NumberFormat.getInstance(Locale.US);
        for (int i = 0; i < reps; ++i) {
            nf.format(1024L);
        }
    }

    public void timeLongToString(int reps) {
        for (int i = 0; i < reps; ++i) {
            Long.toString(1024L);
        }
    }

    public void timeNumberFormatTrivialFormatDouble(int reps) {
        NumberFormat nf = NumberFormat.getInstance(Locale.US);
        for (int i = 0; i < reps; ++i) {
            nf.format(1024.0);
        }
    }

    public void timeNewSimpleDateFormat(int reps) {
        for (int i = 0; i < reps; ++i) {
            new SimpleDateFormat();
        }
    }

    public void timeClonedSimpleDateFormat(int reps) {
        SimpleDateFormat sdf = new SimpleDateFormat();
        for (int i = 0; i < reps; ++i) {
            sdf.clone();
        }
    }

    public void timeNewGregorianCalendar(int reps) {
        for (int i = 0; i < reps; ++i) {
            new GregorianCalendar();
        }
    }

    public void timeClonedGregorianCalendar(int reps) {
        GregorianCalendar gc = new GregorianCalendar();
        for (int i = 0; i < reps; ++i) {
            gc.clone();
        }
    }
}
