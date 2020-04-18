/*
 * Copyright 2010 Google Inc. All Rights Reserved.
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

package com.google.typography.font.sfntly.math;

/**
 * Fixed 16.16 integer utilities.
 * 
 * @author Stuart Gill
 */
public final class Fixed1616 {
  public static int integral(int fixed) {
    return (fixed >> 16) & 0xffff;
  }

  public static int fractional(int fixed) {
    return fixed & 0xffff;
  }

  public static int fixed(int integral, int fractional) {
    return ((integral & 0xffff) << 16) | (fractional & 0xffff);
  }
  
  /**
   * @param fixed the number to convert
   * @return a double representing the 16-16 floating point number
   */
  public static double doubleValue(int fixed) {
    return fixed / 65536.0; // shift the decimal 16 bits
  }

  public static String toString(int fixed) {
    StringBuilder sb = new StringBuilder();
    sb.append(Fixed1616.integral(fixed));
    sb.append(".");
    sb.append(Fixed1616.fractional(fixed));
    return sb.toString();
  }
}
