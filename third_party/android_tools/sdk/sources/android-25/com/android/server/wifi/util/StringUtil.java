/*
 * Copyright (C) 2016 The Android Open Source Project
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

package com.android.server.wifi.util;

/** Basic string utilities */
public class StringUtil {
    static final byte ASCII_PRINTABLE_MIN = ' ';
    static final byte ASCII_PRINTABLE_MAX = '~';

    /** Returns true if-and-only-if |byteArray| can be safely printed as ASCII. */
    public static boolean isAsciiPrintable(byte[] byteArray) {
        if (byteArray == null) {
            return true;
        }

        for (byte b : byteArray) {
            switch (b) {
                // Control characters which actually are printable. Fall-throughs are deliberate.
                case 0x07:      // bell ('\a' not recognized in Java)
                case '\f':      // form feed
                case '\n':      // new line
                case '\t':      // horizontal tab
                case 0x0b:      // vertical tab ('\v' not recognized in Java)
                    continue;
            }

            if (b < ASCII_PRINTABLE_MIN || b > ASCII_PRINTABLE_MAX) {
                return false;
            }
        }

        return true;
    }
}
