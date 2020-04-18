/*
 * Copyright 2011 Google Inc.
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

package com.google.ipc.invalidation.external.client.types;


/**
 * A utility class to format bytes to string for ease of reading and debugging.
 *
 */
class BytesFormatter {

  /**
   * Three arrays that store the representation of each character from 0 to 255.
   * The ith number's octal representation is: CHAR_OCTAL_STRINGS1[i],
   * CHAR_OCTAL_STRINGS2[i], CHAR_OCTAL_STRINGS3[i]
   * <p>
   * E.g., if the number 128, these arrays contain 2, 0, 0 at index 128. We use
   * 3 char arrays instead of an array of strings since the code path for a
   * character append operation is quite a bit shorter than the append operation
   * for strings.
   */
  private static final char[] CHAR_OCTAL_STRINGS1 = new char[256];
  private static final char[] CHAR_OCTAL_STRINGS2 = new char[256];
  private static final char[] CHAR_OCTAL_STRINGS3 = new char[256];

  static {
    // Initialize the array with the Octal string values so that we do not have
    // to do String.format for every byte during runtime.
    for (int i = 0; i < CHAR_OCTAL_STRINGS1.length; i++) {
      // Unsophisticated way to get an octal string padded to 3 characters.
      String intAsStr = Integer.toOctalString(i);
      switch (intAsStr.length()) {
        case 3:
          break;
        case 2:
          intAsStr = "0" + intAsStr;
          break;
        case 1:
          intAsStr = "00" + intAsStr;
          break;
        default:
          throw new RuntimeException("Bad integer value: " + intAsStr);
      }
      if (intAsStr.length() != 3) {
        throw new RuntimeException("Bad padding: " + intAsStr);
      }
      String value = '\\' + intAsStr;
      CHAR_OCTAL_STRINGS1[i] = value.charAt(1);
      CHAR_OCTAL_STRINGS2[i] = value.charAt(2);
      CHAR_OCTAL_STRINGS3[i] = value.charAt(3);
    }
  }

  /** Returns a human-readable string for the contents of {@code bytes}. */
  public static String toString(byte[] bytes) {
    if (bytes == null) {
      return null;
    }
    StringBuilder builder = new StringBuilder(3 * bytes.length);
    for (byte c : bytes) {
      switch(c) {
        case '\n': builder.append('\\'); builder.append('n'); break;
        case '\r': builder.append('\\'); builder.append('r'); break;
        case '\t': builder.append('\\'); builder.append('t'); break;
        case '\"': builder.append('\\'); builder.append('"'); break;
        case '\\': builder.append('\\'); builder.append('\\'); break;
        default:
          if ((c >= 32) && (c < 127) && c != '\'') {
            builder.append((char) c);
          } else {
            int byteValue = c;
            if (c < 0) {
              byteValue = c + 256;
            }
            builder.append('\\');
            builder.append(CHAR_OCTAL_STRINGS1[byteValue]);
            builder.append(CHAR_OCTAL_STRINGS2[byteValue]);
            builder.append(CHAR_OCTAL_STRINGS3[byteValue]);
          }
      }
    }
    return builder.toString();
  }

  private BytesFormatter() {  // To prevent instantiation.
  }
}
