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
public class MathBenchmark {
    private final double d = 1.2;
    private final float f = 1.2f;
    private final int i = 1;
    private final long l = 1L;

    // NOTE: To avoid the benchmarked function from being optimized away, we store the result
    // and use it as the benchmark's return value. This is good enough for now but may not be in
    // the future, a smart compiler could determine that the result value will depend on whether
    // we get into the loop or not and turn the whole loop into an if statement.

    public double timeAbsD(int reps) {
        double result = d;
        for (int rep = 0; rep < reps; ++rep) {
            result = Math.abs(d);
        }
        return result;
    }

    public float timeAbsF(int reps) {
        float result = f;
        for (int rep = 0; rep < reps; ++rep) {
            result = Math.abs(f);
        }
        return result;
    }

    public int timeAbsI(int reps) {
        int result = i;
        for (int rep = 0; rep < reps; ++rep) {
            result = Math.abs(i);
        }
        return result;
    }

    public long timeAbsL(int reps) {
        long result = l;
        for (int rep = 0; rep < reps; ++rep) {
            result = Math.abs(l);
        }
        return result;
    }

    public double timeAcos(int reps) {
        double result = d;
        for (int rep = 0; rep < reps; ++rep) {
            result = Math.acos(d);
        }
        return result;
    }

    public double timeAsin(int reps) {
        double result = d;
        for (int rep = 0; rep < reps; ++rep) {
            result = Math.asin(d);
        }
        return result;
    }

    public double timeAtan(int reps) {
        double result = d;
        for (int rep = 0; rep < reps; ++rep) {
            result = Math.atan(d);
        }
        return result;
    }

    public double timeAtan2(int reps) {
        double result = d;
        for (int rep = 0; rep < reps; ++rep) {
            result = Math.atan2(3, 4);
        }
        return result;
    }

    public double timeCbrt(int reps) {
        double result = d;
        for (int rep = 0; rep < reps; ++rep) {
            result = Math.cbrt(d);
        }
        return result;
    }

    public double timeCeil(int reps) {
        double result = d;
        for (int rep = 0; rep < reps; ++rep) {
            result = Math.ceil(d);
        }
        return result;
    }

    public double timeCopySignD(int reps) {
        double result = d;
        for (int rep = 0; rep < reps; ++rep) {
            result = Math.copySign(d, d);
        }
        return result;
    }

    public float timeCopySignF(int reps) {
        float result = f;
        for (int rep = 0; rep < reps; ++rep) {
            result = Math.copySign(f, f);
        }
        return result;
    }

    public double timeCopySignD_strict(int reps) {
        double result = d;
        for (int rep = 0; rep < reps; ++rep) {
            result = StrictMath.copySign(d, d);
        }
        return result;
    }

    public float timeCopySignF_strict(int reps) {
        float result = f;
        for (int rep = 0; rep < reps; ++rep) {
            result = StrictMath.copySign(f, f);
        }
        return result;
    }

    public double timeCos(int reps) {
        double result = d;
        for (int rep = 0; rep < reps; ++rep) {
            result = Math.cos(d);
        }
        return result;
    }

    public double timeCosh(int reps) {
        double result = d;
        for (int rep = 0; rep < reps; ++rep) {
            result = Math.cosh(d);
        }
        return result;
    }

    public double timeExp(int reps) {
        double result = d;
        for (int rep = 0; rep < reps; ++rep) {
            result = Math.exp(d);
        }
        return result;
    }

    public double timeExpm1(int reps) {
        double result = d;
        for (int rep = 0; rep < reps; ++rep) {
            result = Math.expm1(d);
        }
        return result;
    }

    public double timeFloor(int reps) {
        double result = d;
        for (int rep = 0; rep < reps; ++rep) {
            result = Math.floor(d);
        }
        return result;
    }

    public int timeGetExponentD(int reps) {
        int result = i;
        for (int rep = 0; rep < reps; ++rep) {
            result = Math.getExponent(d);
        }
        return result;
    }

    public int timeGetExponentF(int reps) {
        int result = i;
        for (int rep = 0; rep < reps; ++rep) {
            result = Math.getExponent(f);
        }
        return result;
    }

    public double timeHypot(int reps) {
        double result = d;
        for (int rep = 0; rep < reps; ++rep) {
            result = Math.hypot(d, d);
        }
        return result;
    }

    public double timeIEEEremainder(int reps) {
        double result = d;
        for (int rep = 0; rep < reps; ++rep) {
            result = Math.IEEEremainder(d, d);
        }
        return result;
    }

    public double timeLog(int reps) {
        double result = d;
        for (int rep = 0; rep < reps; ++rep) {
            result = Math.log(d);
        }
        return result;
    }

    public double timeLog10(int reps) {
        double result = d;
        for (int rep = 0; rep < reps; ++rep) {
            result = Math.log10(d);
        }
        return result;
    }

    public double timeLog1p(int reps) {
        double result = d;
        for (int rep = 0; rep < reps; ++rep) {
            result = Math.log1p(d);
        }
        return result;
    }

    public double timeMaxD(int reps) {
        double result = d;
        for (int rep = 0; rep < reps; ++rep) {
            result = Math.max(d, d);
        }
        return result;
    }

    public float timeMaxF(int reps) {
        float result = f;
        for (int rep = 0; rep < reps; ++rep) {
            result = Math.max(f, f);
        }
        return result;
    }

    public int timeMaxI(int reps) {
        int result = i;
        for (int rep = 0; rep < reps; ++rep) {
            result = Math.max(i, i);
        }
        return result;
    }

    public long timeMaxL(int reps) {
        long result = l;
        for (int rep = 0; rep < reps; ++rep) {
            result = Math.max(l, l);
        }
        return result;
    }

    public double timeMinD(int reps) {
        double result = d;
        for (int rep = 0; rep < reps; ++rep) {
            result = Math.min(d, d);
        }
        return result;
    }

    public float timeMinF(int reps) {
        float result = f;
        for (int rep = 0; rep < reps; ++rep) {
            result = Math.min(f, f);
        }
        return result;
    }

    public int timeMinI(int reps) {
        int result = i;
        for (int rep = 0; rep < reps; ++rep) {
            result = Math.min(i, i);
        }
        return result;
    }

    public long timeMinL(int reps) {
        long result = l;
        for (int rep = 0; rep < reps; ++rep) {
            result = Math.min(l, l);
        }
        return result;
    }

    public double timeNextAfterD(int reps) {
        double result = d;
        for (int rep = 0; rep < reps; ++rep) {
            result = Math.nextAfter(d, d);
        }
        return result;
    }

    public float timeNextAfterF(int reps) {
        float result = f;
        for (int rep = 0; rep < reps; ++rep) {
            result = Math.nextAfter(f, f);
        }
        return result;
    }

    public double timeNextUpD(int reps) {
        double result = d;
        for (int rep = 0; rep < reps; ++rep) {
            result = Math.nextUp(d);
        }
        return result;
    }

    public float timeNextUpF(int reps) {
        float result = f;
        for (int rep = 0; rep < reps; ++rep) {
            result = Math.nextUp(f);
        }
        return result;
    }

    public double timePow(int reps) {
        double result = d;
        for (int rep = 0; rep < reps; ++rep) {
            result = Math.pow(d, d);
        }
        return result;
    }

    public double timeRandom(int reps) {
        double result = d;
        for (int rep = 0; rep < reps; ++rep) {
            result = Math.random();
        }
        return result;
    }

    public double timeRint(int reps) {
        double result = d;
        for (int rep = 0; rep < reps; ++rep) {
            result = Math.rint(d);
        }
        return result;
    }

    public long timeRoundD(int reps) {
        long result = l;
        for (int rep = 0; rep < reps; ++rep) {
            result = Math.round(d);
        }
        return result;
    }

    public int timeRoundF(int reps) {
        int result = i;
        for (int rep = 0; rep < reps; ++rep) {
            result = Math.round(f);
        }
        return result;
    }

    public double timeScalbD(int reps) {
        double result = d;
        for (int rep = 0; rep < reps; ++rep) {
            result = Math.scalb(d, 5);
        }
        return result;
    }

    public float timeScalbF(int reps) {
        float result = f;
        for (int rep = 0; rep < reps; ++rep) {
            result = Math.scalb(f, 5);
        }
        return result;
    }

    public double timeSignumD(int reps) {
        double result = d;
        for (int rep = 0; rep < reps; ++rep) {
            result = Math.signum(d);
        }
        return result;
    }

    public float timeSignumF(int reps) {
        float result = f;
        for (int rep = 0; rep < reps; ++rep) {
            result = Math.signum(f);
        }
        return result;
    }

    public double timeSin(int reps) {
        double result = d;
        for (int rep = 0; rep < reps; ++rep) {
            result = Math.sin(d);
        }
        return result;
    }

    public double timeSinh(int reps) {
        double result = d;
        for (int rep = 0; rep < reps; ++rep) {
            result = Math.sinh(d);
        }
        return result;
    }

    public double timeSqrt(int reps) {
        double result = d;
        for (int rep = 0; rep < reps; ++rep) {
            result = Math.sqrt(d);
        }
        return result;
    }

    public double timeTan(int reps) {
        double result = d;
        for (int rep = 0; rep < reps; ++rep) {
            result = Math.tan(d);
        }
        return result;
    }

    public double timeTanh(int reps) {
        double result = d;
        for (int rep = 0; rep < reps; ++rep) {
            result = Math.tanh(d);
        }
        return result;
    }

    public double timeToDegrees(int reps) {
        double result = d;
        for (int rep = 0; rep < reps; ++rep) {
            result = Math.toDegrees(d);
        }
        return result;
    }

    public double timeToRadians(int reps) {
        double result = d;
        for (int rep = 0; rep < reps; ++rep) {
            result = Math.toRadians(d);
        }
        return result;
    }

    public double timeUlpD(int reps) {
        double result = d;
        for (int rep = 0; rep < reps; ++rep) {
            result = Math.ulp(d);
        }
        return result;
    }

    public float timeUlpF(int reps) {
        float result = f;
        for (int rep = 0; rep < reps; ++rep) {
            result = Math.ulp(f);
        }
        return result;
    }
}
