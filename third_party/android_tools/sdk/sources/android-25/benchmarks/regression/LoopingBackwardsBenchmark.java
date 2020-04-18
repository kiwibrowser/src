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

import com.google.caliper.Param;

/**
 * Testing the old canard that looping backwards is faster.
 *
 * @author Kevin Bourrillion
 */
public class LoopingBackwardsBenchmark {
  @Param({"2", "20", "2000", "20000000"}) int max;

  public int timeForwards(int reps) {
    int dummy = 0;
    for (int i = 0; i < reps; i++) {
      for (int j = 0; j < max; j++) {
        dummy += j;
      }
    }
    return dummy;
  }

  public int timeBackwards(int reps) {
    int dummy = 0;
    for (int i = 0; i < reps; i++) {
      for (int j = max - 1; j >= 0; j--) {
        dummy += j;
      }
    }
    return dummy;
  }
}
