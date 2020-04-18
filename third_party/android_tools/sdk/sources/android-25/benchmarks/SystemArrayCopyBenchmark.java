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

package benchmarks;

import com.google.caliper.Param;

public class SystemArrayCopyBenchmark {
  @Param({"2", "4", "8", "16", "32", "64", "128", "256", "512", "1024",
          "2048", "4096", "8192", "16384", "32768", "65536", "131072", "262144"})
  int arrayLength;

  // Provides benchmarking for different types of arrays using the arraycopy function.

  public void timeSystemCharArrayCopy(int reps) {
    final int len = arrayLength;
    char[] src = new char[len];
    char[] dst = new char[len];
    for (int rep = 0; rep < reps; ++rep) {
      System.arraycopy(src, 0, dst, 0, len);
    }
  }

  public void timeSystemByteArrayCopy(int reps) {
    final int len = arrayLength;
    byte[] src = new byte[len];
    byte[] dst = new byte[len];
    for (int rep = 0; rep < reps; ++rep) {
      System.arraycopy(src, 0, dst, 0, len);
    }
  }

  public void timeSystemShortArrayCopy(int reps) {
    final int len = arrayLength;
    short[] src = new short[len];
    short[] dst = new short[len];
    for (int rep = 0; rep < reps; ++rep) {
      System.arraycopy(src, 0, dst, 0, len);
    }
  }

  public void timeSystemIntArrayCopy(int reps) {
    final int len = arrayLength;
    int[] src = new int[len];
    int[] dst = new int[len];
    for (int rep = 0; rep < reps; ++rep) {
      System.arraycopy(src, 0, dst, 0, len);
    }
  }

  public void timeSystemLongArrayCopy(int reps) {
    final int len = arrayLength;
    long[] src = new long[len];
    long[] dst = new long[len];
    for (int rep = 0; rep < reps; ++rep) {
      System.arraycopy(src, 0, dst, 0, len);
    }
  }

  public void timeSystemFloatArrayCopy(int reps) {
    final int len = arrayLength;
    float[] src = new float[len];
    float[] dst = new float[len];
    for (int rep = 0; rep < reps; ++rep) {
      System.arraycopy(src, 0, dst, 0, len);
    }
  }

  public void timeSystemDoubleArrayCopy(int reps) {
    final int len = arrayLength;
    double[] src = new double[len];
    double[] dst = new double[len];
    for (int rep = 0; rep < reps; ++rep) {
      System.arraycopy(src, 0, dst, 0, len);
    }
  }

  public void timeSystemBooleanArrayCopy(int reps) {
    final int len = arrayLength;
    boolean[] src = new boolean[len];
    boolean[] dst = new boolean[len];
    for (int rep = 0; rep < reps; ++rep) {
      System.arraycopy(src, 0, dst, 0, len);
    }
  }
}
