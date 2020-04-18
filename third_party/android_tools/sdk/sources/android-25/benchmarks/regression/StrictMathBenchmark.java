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

/**
 * Many of these tests are bogus in that the cost will vary wildly depending on inputs.
 * For _my_ current purposes, that's okay. But beware!
 */
public class StrictMathBenchmark {
    private final double d = 1.2;
    private final float f = 1.2f;
    private final int i = 1;
    private final long l = 1L;

    /* Values for full line coverage of ceiling function */
    private static final double[] CEIL_DOUBLES = new double[] {
            3245817.2018463886,
            1418139.083668501,
            3.572936802189103E15,
            -4.7828929737254625E249,
            213596.58636369856,
            6.891928421440976E-96,
            -7.9318566885477E-36,
            -1.9610339084804148E15,
            -4.696725715628246E10,
            3742491.296880909,
            7.140274745333553E11
    };

    /* Values for full line coverage of floor function */
    private static final double[] FLOOR_DOUBLES = new double[] {
            7.140274745333553E11,
            3742491.296880909,
            -4.696725715628246E10,
            -1.9610339084804148E15,
            7.049948629370372E-56,
            -7.702933170334643E-16,
            -1.99657681810579,
            -1.1659287182288336E236,
            4.085518816513057E15,
            -1500948.440658056,
            -2.2316479921415575E7
    };

    public void timeAbsD(int reps) {
        for (int rep = 0; rep < reps; ++rep) {
            StrictMath.abs(d);
        }
    }

    public void timeAbsF(int reps) {
        for (int rep = 0; rep < reps; ++rep) {
            StrictMath.abs(f);
        }
    }

    public void timeAbsI(int reps) {
        for (int rep = 0; rep < reps; ++rep) {
            StrictMath.abs(i);
        }
    }

    public void timeAbsL(int reps) {
        for (int rep = 0; rep < reps; ++rep) {
            StrictMath.abs(l);
        }
    }

    public void timeAcos(int reps) {
        for (int rep = 0; rep < reps; ++rep) {
            StrictMath.acos(d);
        }
    }

    public void timeAsin(int reps) {
        for (int rep = 0; rep < reps; ++rep) {
            StrictMath.asin(d);
        }
    }

    public void timeAtan(int reps) {
        for (int rep = 0; rep < reps; ++rep) {
            StrictMath.atan(d);
        }
    }

    public void timeAtan2(int reps) {
        for (int rep = 0; rep < reps; ++rep) {
            StrictMath.atan2(3, 4);
        }
    }

    public void timeCbrt(int reps) {
        for (int rep = 0; rep < reps; ++rep) {
            StrictMath.cbrt(d);
        }
    }

    public void timeCeilOverInterestingValues(int reps) {
        for (int rep = 0; rep < reps; ++rep) {
            for (int i = 0; i < CEIL_DOUBLES.length; ++i) {
                StrictMath.ceil(CEIL_DOUBLES[i]);
            }
        }
    }

    public void timeCopySignD(int reps) {
        for (int rep = 0; rep < reps; ++rep) {
            StrictMath.copySign(d, d);
        }
    }

    public void timeCopySignF(int reps) {
        for (int rep = 0; rep < reps; ++rep) {
            StrictMath.copySign(f, f);
        }
    }

    public void timeCos(int reps) {
        for (int rep = 0; rep < reps; ++rep) {
            StrictMath.cos(d);
        }
    }

    public void timeCosh(int reps) {
        for (int rep = 0; rep < reps; ++rep) {
            StrictMath.cosh(d);
        }
    }

    public void timeExp(int reps) {
        for (int rep = 0; rep < reps; ++rep) {
            StrictMath.exp(d);
        }
    }

    public void timeExpm1(int reps) {
        for (int rep = 0; rep < reps; ++rep) {
            StrictMath.expm1(d);
        }
    }

    public void timeFloorOverInterestingValues(int reps) {
        for (int rep = 0; rep < reps; ++rep) {
            for (int i = 0; i < FLOOR_DOUBLES.length; ++i) {
                StrictMath.floor(FLOOR_DOUBLES[i]);
            }
        }
    }

    public void timeGetExponentD(int reps) {
        for (int rep = 0; rep < reps; ++rep) {
            StrictMath.getExponent(d);
        }
    }

    public void timeGetExponentF(int reps) {
        for (int rep = 0; rep < reps; ++rep) {
            StrictMath.getExponent(f);
        }
    }

    public void timeHypot(int reps) {
        for (int rep = 0; rep < reps; ++rep) {
            StrictMath.hypot(d, d);
        }
    }

    public void timeIEEEremainder(int reps) {
        for (int rep = 0; rep < reps; ++rep) {
            StrictMath.IEEEremainder(d, d);
        }
    }

    public void timeLog(int reps) {
        for (int rep = 0; rep < reps; ++rep) {
            StrictMath.log(d);
        }
    }

    public void timeLog10(int reps) {
        for (int rep = 0; rep < reps; ++rep) {
            StrictMath.log10(d);
        }
    }

    public void timeLog1p(int reps) {
        for (int rep = 0; rep < reps; ++rep) {
            StrictMath.log1p(d);
        }
    }

    public void timeMaxD(int reps) {
        for (int rep = 0; rep < reps; ++rep) {
            StrictMath.max(d, d);
        }
    }

    public void timeMaxF(int reps) {
        for (int rep = 0; rep < reps; ++rep) {
            StrictMath.max(f, f);
        }
    }

    public void timeMaxI(int reps) {
        for (int rep = 0; rep < reps; ++rep) {
            StrictMath.max(i, i);
        }
    }

    public void timeMaxL(int reps) {
        for (int rep = 0; rep < reps; ++rep) {
            StrictMath.max(l, l);
        }
    }

    public void timeMinD(int reps) {
        for (int rep = 0; rep < reps; ++rep) {
            StrictMath.min(d, d);
        }
    }

    public void timeMinF(int reps) {
        for (int rep = 0; rep < reps; ++rep) {
            StrictMath.min(f, f);
        }
    }

    public void timeMinI(int reps) {
        for (int rep = 0; rep < reps; ++rep) {
            StrictMath.min(i, i);
        }
    }

    public void timeMinL(int reps) {
        for (int rep = 0; rep < reps; ++rep) {
            StrictMath.min(l, l);
        }
    }

    public void timeNextAfterD(int reps) {
        for (int rep = 0; rep < reps; ++rep) {
            StrictMath.nextAfter(d, d);
        }
    }

    public void timeNextAfterF(int reps) {
        for (int rep = 0; rep < reps; ++rep) {
            StrictMath.nextAfter(f, f);
        }
    }

    public void timeNextUpD(int reps) {
        for (int rep = 0; rep < reps; ++rep) {
            StrictMath.nextUp(d);
        }
    }

    public void timeNextUpF(int reps) {
        for (int rep = 0; rep < reps; ++rep) {
            StrictMath.nextUp(f);
        }
    }

    public void timePow(int reps) {
        for (int rep = 0; rep < reps; ++rep) {
            StrictMath.pow(d, d);
        }
    }

    public void timeRandom(int reps) {
        for (int rep = 0; rep < reps; ++rep) {
            StrictMath.random();
        }
    }

    public void timeRint(int reps) {
        for (int rep = 0; rep < reps; ++rep) {
            StrictMath.rint(d);
        }
    }

    public void timeRoundD(int reps) {
        for (int rep = 0; rep < reps; ++rep) {
            StrictMath.round(d);
        }
    }

    public void timeRoundF(int reps) {
        for (int rep = 0; rep < reps; ++rep) {
            StrictMath.round(f);
        }
    }

    public void timeScalbD(int reps) {
        for (int rep = 0; rep < reps; ++rep) {
            StrictMath.scalb(d, 5);
        }
    }

    public void timeScalbF(int reps) {
        for (int rep = 0; rep < reps; ++rep) {
            StrictMath.scalb(f, 5);
        }
    }

    public void timeSignumD(int reps) {
        for (int rep = 0; rep < reps; ++rep) {
            StrictMath.signum(d);
        }
    }

    public void timeSignumF(int reps) {
        for (int rep = 0; rep < reps; ++rep) {
            StrictMath.signum(f);
        }
    }

    public void timeSin(int reps) {
        for (int rep = 0; rep < reps; ++rep) {
            StrictMath.sin(d);
        }
    }

    public void timeSinh(int reps) {
        for (int rep = 0; rep < reps; ++rep) {
            StrictMath.sinh(d);
        }
    }

    public void timeSqrt(int reps) {
        for (int rep = 0; rep < reps; ++rep) {
            StrictMath.sqrt(d);
        }
    }

    public void timeTan(int reps) {
        for (int rep = 0; rep < reps; ++rep) {
            StrictMath.tan(d);
        }
    }

    public void timeTanh(int reps) {
        for (int rep = 0; rep < reps; ++rep) {
            StrictMath.tanh(d);
        }
    }

    public void timeToDegrees(int reps) {
        for (int rep = 0; rep < reps; ++rep) {
            StrictMath.toDegrees(d);
        }
    }

    public void timeToRadians(int reps) {
        for (int rep = 0; rep < reps; ++rep) {
            StrictMath.toRadians(d);
        }
    }

    public void timeUlpD(int reps) {
        for (int rep = 0; rep < reps; ++rep) {
            StrictMath.ulp(d);
        }
    }

    public void timeUlpF(int reps) {
        for (int rep = 0; rep < reps; ++rep) {
            StrictMath.ulp(f);
        }
    }
}
