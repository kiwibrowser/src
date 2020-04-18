// Copyright 2018 The Feed Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

package com.google.android.libraries.feed.common.logging;

import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Locale;

/** Date-formatting static methods. */
public class StringFormattingUtils {

  // Do not instantiate
  private StringFormattingUtils() {}

  private static SimpleDateFormat logDateFormat;

  /** Formats {@code date} in the same format as used by logcat. */
  public static synchronized String formatLogDate(Date date) {
    if (logDateFormat == null) {
      // Getting the date format is mildly expensive, so don't do it unless we need it.
      logDateFormat = new SimpleDateFormat("MM-dd HH:mm:ss.SSS", Locale.US);
    }
    return logDateFormat.format(date);
  }
}
