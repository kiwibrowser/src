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
 * Utility math for fonts.
 * 
 * @author Stuart Gill
 */
public final class FontMath {
  private FontMath() {
    // Prevent construction.
  }

  public static int log2(int a) {
    return 31 - Integer.numberOfLeadingZeros(a);
  }

  /**
   * Calculates the amount of padding needed. The values provided need to be in
   * the same units. So, if the size is given as the number of bytes then the
   * alignment size must also be specified as byte size to align to.
   *
   * @param size the size of the data that may need padding
   * @param alignmentSize the number of units to align to
   * @return the number of units needing to be added for alignment
   */
  public static int paddingRequired(int size, int alignmentSize) {
    int padding = alignmentSize - (size % alignmentSize);
    return padding == alignmentSize ? 0 : padding;
  }
}
