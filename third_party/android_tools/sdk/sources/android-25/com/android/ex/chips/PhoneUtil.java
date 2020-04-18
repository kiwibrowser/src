/*

 * Copyright (C) 2011 The Android Open Source Project
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

package com.android.ex.chips;

import android.text.TextUtils;

import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * A utility class for chips using phone numbers.
 */
public class PhoneUtil {

    // This pattern comes from android.util.Patterns. It has been tweaked to handle a "1" before
    // parens, so numbers such as "1 (425) 222-2342" match.
    private static final Pattern PHONE_PATTERN
            = Pattern.compile(                                  // sdd = space, dot, or dash
                    "(\\+[0-9]+[\\- \\.]*)?"                    // +<digits><sdd>*
                    + "(1?[ ]*\\([0-9]+\\)[\\- \\.]*)?"         // 1(<digits>)<sdd>*
                    + "([0-9][0-9\\- \\.][0-9\\- \\.]+[0-9])"); // <digit><digit|sdd>+<digit>

    /**
     * Returns true if the string is a phone number.
     */
    public static boolean isPhoneNumber(String number) {
        // TODO: replace this function with libphonenumber's isPossibleNumber (see
        // PhoneNumberUtil). One complication is that it requires the sender's region which
        // comes from the CurrentCountryIso. For now, let's just do this simple match.
        if (TextUtils.isEmpty(number)) {
            return false;
        }

        Matcher match = PHONE_PATTERN.matcher(number);
        return match.matches();
    }
}
