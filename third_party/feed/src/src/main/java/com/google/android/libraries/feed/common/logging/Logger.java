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

import static android.util.Log.ASSERT;
import static android.util.Log.DEBUG;
import static android.util.Log.ERROR;
import static android.util.Log.INFO;
import static android.util.Log.VERBOSE;
import static android.util.Log.WARN;

import android.os.Build;
import android.util.Base64;
import android.util.Log;
import java.io.UnsupportedEncodingException;
import java.util.Arrays;
import java.util.IllegalFormatException;
import java.util.Locale;

/**
 * Helper class for logging in GSA. This class only logs if the tag for the log request is loggable.
 * For debug and verbose logs, it also looks at the {@link #isEnabled} flag to determine whether to
 * log or not.
 *
 * <p>Usage:
 *
 * <ul>
 *   <li>Use {@code adb shell setprop log.tag.TAG DEBUG} to enable debug logs from <i>TAG</i>.
 *   <li>Use {@code adb shell setprop log.tag.TAG VERBOSE} to enable verbose logs from <i>TAG</i>.
 *   <li>Use {@code adb shell setprop log.tag.TAG SUPPRESS} to disable all logs from <i>TAG</i>.
 * </ul>
 *
 * <p>To log a formatter string, use the syntax specified for {@link java.util.Formatter} or see
 * {@link String#format(String, Object...)}. For example:
 *
 * <pre>{@code
 * L.d(TAG, "%s did something to %s resulting in %s", A, B, C);
 * }</pre>
 *
 * <p>Note: All {@code toString()} operations are evaluated lazily and only when necessary, so it's
 * recommended to pass in objects rather than pre-formatted string representations (e.g. using
 * {@link Object#toString} or {@link String#format}).
 *
 * <p>Note that arrays are automatically represented using {@link Arrays#deepToString} instead of
 * {@link Object#toString} for better readability.
 *
 * <p>TODO: We need to verify the Proguard behaviors
 *
 * <p>TODO: Implementation of Redactable is removed, do we need to support this?
 *
 * <p>TODO: Should we remove the crashing behavior?
 *
 * <p>Note: in release builds Proguard should remove all calls to
 * L#{d,v,dWithStackTrace,vWithStackTrace}. See proguard_release.flags and
 * [INTERNAL LINK]
 *
 * <p>WARNING: Do not add vararg overloaded versions of L#{d,v,dWithStackTrace,vWithStackTrace}.
 * Proguard cannot properly shrink vararg statements.
 */
public final class Logger {
  private static final String STRING_MEANING_NULL = "null";
  private static final String NOT_AN_EXCEPTION = "DEBUG: Not an Exception";

  // private-constructor
  private Logger() {}

  /**
   * Log a verbose message.
   *
   * <p>NOTE: All calls to this method must be wrapped inside if(L.isDebugEnabled(TAG)) {}.
   *
   * @param tag the tag shouldn't be more than 23 characters as {@link Log#isLoggable(String, int)}
   *     has this restriction.
   * @param message the string message to log.
   */
  public static void v(String tag, String message) {
    internalLog(VERBOSE, tag, null, message, false);
  }

  /**
   * Log a verbose message.
   *
   * <p>NOTE: All calls to this method must be wrapped inside if(L.isDebugEnabled(TAG)) {}.
   *
   * @param tag the tag shouldn't be more than 23 characters as {@link Log#isLoggable(String, int)}
   *     has this restriction.
   * @param message the string format recognized by {@link String#format(String, Object...)}.
   * @param arg1 the formatting arg for the previous string.
   */
  public static void v(String tag, String message, /*@Nullable*/ Object arg1) {
    internalLog(VERBOSE, tag, null, message, false, arg1);
  }

  /** @see #v(String, String, Object) */
  public static void v(String tag, String message, /*@Nullable*/ Object arg1, /*@Nullable*/ Object arg2) {
    internalLog(VERBOSE, tag, null, message, false, arg1, arg2);
  }

  /** @see #v(String, String, Object) */
  public static void v(
      String tag,
      String message,
      /*@Nullable*/ Object arg1,
      /*@Nullable*/ Object arg2,
      /*@Nullable*/ Object arg3) {
    internalLog(VERBOSE, tag, null, message, false, arg1, arg2, arg3);
  }

  /** @see #v(String, String, Object) */
  public static void v(
      String tag,
      String message,
      /*@Nullable*/ Object arg1,
      /*@Nullable*/ Object arg2,
      /*@Nullable*/ Object arg3,
      /*@Nullable*/ Object arg4) {
    internalLog(VERBOSE, tag, null, message, false, arg1, arg2, arg3, arg4);
  }

  /** @see #v(String, String, Object) */
  public static void v(
      String tag,
      String message,
      /*@Nullable*/ Object arg1,
      /*@Nullable*/ Object arg2,
      /*@Nullable*/ Object arg3,
      /*@Nullable*/ Object arg4,
      /*@Nullable*/ Object arg5) {
    internalLog(VERBOSE, tag, null, message, false, arg1, arg2, arg3, arg4, arg5);
  }

  /**
   * Log a verbose message and include the current stack trace in the log.
   *
   * <p>NOTE: All calls to this method must be wrapped inside if(L.isDebugEnabled(TAG)) {}.
   *
   * @param tag the tag shouldn't be more than 23 characters as {@link Log#isLoggable(String, int)}
   *     has this restriction.
   * @param message the string message to log.
   */
  public static void vWithStackTrace(String tag, String message) {
    internalLog(VERBOSE, tag, null, message, true);
  }

  /**
   * Log a verbose message and include the current stack trace in the log.
   *
   * <p>NOTE: All calls to this method must be wrapped inside if(L.isDebugEnabled(TAG)) {}.
   *
   * @param tag the tag shouldn't be more than 23 characters as {@link Log#isLoggable(String, int)}
   *     has this restriction.
   * @param message the string format recognized by {@link String#format(String, Object...)}.
   * @param arg1 the formatting arg for the previous string.
   */
  public static void vWithStackTrace(String tag, String message, /*@Nullable*/ Object arg1) {
    internalLog(VERBOSE, tag, null, message, true, arg1);
  }

  /** @see #vWithStackTrace(String, String, Object) */
  public static void vWithStackTrace(
      String tag, String message, /*@Nullable*/ Object arg1, /*@Nullable*/ Object arg2) {
    internalLog(VERBOSE, tag, null, message, true, arg1, arg2);
  }

  /** @see #vWithStackTrace(String, String, Object) */
  public static void vWithStackTrace(
      String tag,
      String message,
      /*@Nullable*/ Object arg1,
      /*@Nullable*/ Object arg2,
      /*@Nullable*/ Object arg3) {
    internalLog(VERBOSE, tag, null, message, true, arg1, arg2, arg3);
  }

  /** @see #vWithStackTrace(String, String, Object) */
  public static void vWithStackTrace(
      String tag,
      String message,
      /*@Nullable*/ Object arg1,
      /*@Nullable*/ Object arg2,
      /*@Nullable*/ Object arg3,
      /*@Nullable*/ Object arg4) {
    internalLog(VERBOSE, tag, null, message, true, arg1, arg2, arg3, arg4);
  }

  /** @see #vWithStackTrace(String, String, Object) */
  public static void vWithStackTrace(
      String tag,
      String message,
      /*@Nullable*/ Object arg1,
      /*@Nullable*/ Object arg2,
      /*@Nullable*/ Object arg3,
      /*@Nullable*/ Object arg4,
      /*@Nullable*/ Object arg5) {
    internalLog(VERBOSE, tag, null, message, true, arg1, arg2, arg3, arg4, arg5);
  }

  /**
   * Log a debug message.
   *
   * <p>NOTE: All calls to this method must be wrapped inside if(L.isDebugEnabled(TAG)) {}.
   *
   * @param tag the tag shouldn't be more than 23 characters as {@link Log#isLoggable(String, int)}
   *     has this restriction.
   * @param message the string message to log.
   */
  public static void d(String tag, String message) {
    internalLog(DEBUG, tag, null, message, false);
  }

  /**
   * Log a debug message.
   *
   * <p>NOTE: All calls to this method must be wrapped inside if(L.isDebugEnabled(TAG)) {}.
   *
   * @param tag the tag shouldn't be more than 23 characters as {@link Log#isLoggable(String, int)}
   *     has this restriction.
   * @param message the string format recognized by {@link String#format(String, Object...)}.
   * @param arg1 the formatting arg for the previous string.
   */
  public static void d(String tag, String message, /*@Nullable*/ Object arg1) {
    internalLog(DEBUG, tag, null, message, false, arg1);
  }

  /** @see #d(String, String, Object) */
  public static void d(String tag, String message, /*@Nullable*/ Object arg1, /*@Nullable*/ Object arg2) {
    internalLog(DEBUG, tag, null, message, false, arg1, arg2);
  }

  /** @see #d(String, String, Object) */
  public static void d(
      String tag,
      String message,
      /*@Nullable*/ Object arg1,
      /*@Nullable*/ Object arg2,
      /*@Nullable*/ Object arg3) {
    internalLog(DEBUG, tag, null, message, false, arg1, arg2, arg3);
  }

  /** @see #d(String, String, Object) */
  public static void d(
      String tag,
      String message,
      /*@Nullable*/ Object arg1,
      /*@Nullable*/ Object arg2,
      /*@Nullable*/ Object arg3,
      /*@Nullable*/ Object arg4) {
    internalLog(DEBUG, tag, null, message, false, arg1, arg2, arg3, arg4);
  }

  /** @see #d(String, String, Object) */
  public static void d(
      String tag,
      String message,
      /*@Nullable*/ Object arg1,
      /*@Nullable*/ Object arg2,
      /*@Nullable*/ Object arg3,
      /*@Nullable*/ Object arg4,
      /*@Nullable*/ Object arg5) {
    internalLog(DEBUG, tag, null, message, false, arg1, arg2, arg3, arg4, arg5);
  }

  /** @see #d(String, String, Object) */
  public static void d(
      String tag,
      String message,
      /*@Nullable*/ Object arg1,
      /*@Nullable*/ Object arg2,
      /*@Nullable*/ Object arg3,
      /*@Nullable*/ Object arg4,
      /*@Nullable*/ Object arg5,
      /*@Nullable*/ Object arg6) {
    internalLog(DEBUG, tag, null, message, false, arg1, arg2, arg3, arg4, arg5, arg6);
  }

  /** @see #d(String, String, Object) */
  public static void d(
      String tag,
      String message,
      /*@Nullable*/ Object arg1,
      /*@Nullable*/ Object arg2,
      /*@Nullable*/ Object arg3,
      /*@Nullable*/ Object arg4,
      /*@Nullable*/ Object arg5,
      /*@Nullable*/ Object arg6,
      /*@Nullable*/ Object arg7) {
    internalLog(DEBUG, tag, null, message, false, arg1, arg2, arg3, arg4, arg5, arg6, arg7);
  }

  /** @see #d(String, String, Object) */
  public static void d(
      String tag,
      String message,
      /*@Nullable*/ Object arg1,
      /*@Nullable*/ Object arg2,
      /*@Nullable*/ Object arg3,
      /*@Nullable*/ Object arg4,
      /*@Nullable*/ Object arg5,
      /*@Nullable*/ Object arg6,
      /*@Nullable*/ Object arg7,
      /*@Nullable*/ Object arg8) {
    internalLog(DEBUG, tag, null, message, false, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8);
  }

  /** @see #d(String, String, Object) */
  public static void d(
      String tag,
      String message,
      /*@Nullable*/ Object arg1,
      /*@Nullable*/ Object arg2,
      /*@Nullable*/ Object arg3,
      /*@Nullable*/ Object arg4,
      /*@Nullable*/ Object arg5,
      /*@Nullable*/ Object arg6,
      /*@Nullable*/ Object arg7,
      /*@Nullable*/ Object arg8,
      /*@Nullable*/ Object arg9) {
    internalLog(
        DEBUG, tag, null, message, false, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9);
  }

  /** @see #d(String, String, Object) */
  public static void d(
      String tag,
      String message,
      /*@Nullable*/ Object arg1,
      /*@Nullable*/ Object arg2,
      /*@Nullable*/ Object arg3,
      /*@Nullable*/ Object arg4,
      /*@Nullable*/ Object arg5,
      /*@Nullable*/ Object arg6,
      /*@Nullable*/ Object arg7,
      /*@Nullable*/ Object arg8,
      /*@Nullable*/ Object arg9,
      /*@Nullable*/ Object arg10) {
    internalLog(
        DEBUG, tag, null, message, false, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9,
        arg10);
  }

  /**
   * Log a debug message that includes the current stack trace in the log.
   *
   * <p>NOTE: All calls to this method must be wrapped inside if(L.isDebugEnabled(TAG)) {}.
   *
   * @param tag the tag shouldn't be more than 23 characters as {@link Log#isLoggable(String, int)}
   *     has this restriction.
   * @param message the string message to log.
   */
  public static void dWithStackTrace(String tag, String message) {
    internalLog(DEBUG, tag, null, message, true);
  }

  /**
   * Log a debug message that includes the current stack trace in the log.
   *
   * <p>NOTE: All calls to this method must be wrapped inside if(L.isDebugEnabled(TAG)) {}.
   *
   * @param tag the tag shouldn't be more than 23 characters as {@link Log#isLoggable(String, int)}
   *     has this restriction.
   * @param message the string format recognized by {@link String#format(String, Object...)}.
   * @param arg1 the formatting arg for the previous string.
   */
  public static void dWithStackTrace(String tag, String message, /*@Nullable*/ Object arg1) {
    internalLog(DEBUG, tag, null, message, true, arg1);
  }

  /** @see #dWithStackTrace(String, String, Object) */
  public static void dWithStackTrace(
      String tag, String message, /*@Nullable*/ Object arg1, /*@Nullable*/ Object arg2) {
    internalLog(DEBUG, tag, null, message, true, arg1, arg2);
  }

  /** @see #dWithStackTrace(String, String, Object) */
  public static void dWithStackTrace(
      String tag,
      String message,
      /*@Nullable*/ Object arg1,
      /*@Nullable*/ Object arg2,
      /*@Nullable*/ Object arg3) {
    internalLog(DEBUG, tag, null, message, true, arg1, arg2, arg3);
  }

  /** @see #dWithStackTrace(String, String, Object) */
  public static void dWithStackTrace(
      String tag,
      String message,
      /*@Nullable*/ Object arg1,
      /*@Nullable*/ Object arg2,
      /*@Nullable*/ Object arg3,
      /*@Nullable*/ Object arg4) {
    internalLog(DEBUG, tag, null, message, true, arg1, arg2, arg3, arg4);
  }

  /** @see #dWithStackTrace(String, String, Object) */
  public static void dWithStackTrace(
      String tag,
      String message,
      /*@Nullable*/ Object arg1,
      /*@Nullable*/ Object arg2,
      /*@Nullable*/ Object arg3,
      /*@Nullable*/ Object arg4,
      /*@Nullable*/ Object arg5) {
    internalLog(DEBUG, tag, null, message, true, arg1, arg2, arg3, arg4, arg5);
  }

  /**
   * Log an info message.
   *
   * @param tag the tag shouldn't be more than 23 characters as {@link Log#isLoggable(String, int)}
   *     has this restriction.
   * @param message the string message to log. This can also be a string format that's recognized by
   *     {@link String#format(String, Object...)}. e.g. "%s did something to %s, and %d happened as
   *     a result".
   * @param args the formatting args for the previous string.
   */
  public static void i(String tag, /*@Nullable*/ String message, /*@Nullable*/ Object... args) {
    internalLog(INFO, tag, null, message, false, args);
  }

  /**
   * Log an info message that includes the current stack trace in the log.
   *
   * @param tag the tag shouldn't be more than 23 characters as {@link Log#isLoggable(String, int)}
   *     has this restriction.
   * @param message the string message to log. This can also be a string format that's recognized by
   *     {@link String#format(String, Object...)}. e.g. "%s did something to %s, and %d happened as
   *     a result".
   * @param args the formatting args for the previous string.
   */
  public static void iWithStackTrace(String tag, String message, /*@Nullable*/ Object... args) {
    internalLog(INFO, tag, null, message, true, args);
  }

  /**
   * Log a warning message.
   *
   * @param tag the tag shouldn't be more than 23 characters as {@link Log#isLoggable(String, int)}
   *     has this restriction.
   * @param message the string message to log. This can also be a string format that's recognized by
   *     {@link String#format(String, Object...)}. e.g. "%s did something to %s, and %d happened as
   *     a result".
   * @param args the formatting args for the previous string.
   */
  public static void w(String tag, /*@Nullable*/ String message, /*@Nullable*/ Object... args) {
    internalLog(WARN, tag, null, message, false, args);
  }

  /**
   * Log a warning message.
   *
   * @param tag the tag shouldn't be more than 23 characters as {@link Log#isLoggable(String, int)}
   *     has this restriction.
   * @param th a throwable to log.
   * @param message the string message to log. This can also be a string format that's recognized by
   *     {@link String#format(String, Object...)}. e.g. "%s did something to %s, and %d happened as
   *     a result".
   * @param args the formatting args for the previous string.
   */
  public static void w(
      String tag, Throwable th, /*@Nullable*/ String message, /*@Nullable*/ Object... args) {
    internalLog(WARN, tag, th, message, false, args);
  }

  /**
   * Log a warning message that includes the current stack trace in the log.
   *
   * @param tag the tag shouldn't be more than 23 characters as {@link Log#isLoggable(String, int)}
   *     has this restriction.
   * @param message the string message to log. This can also be a string format that's recognized by
   *     {@link String#format(String, Object...)}. e.g. "%s did something to %s, and %d happened as
   *     a result".
   * @param args the formatting args for the previous string.
   */
  public static void wWithStackTrace(
      String tag, /*@Nullable*/ String message, /*@Nullable*/ Object... args) {
    internalLog(WARN, tag, null, message, true, args);
  }

  /**
   * Log an error message.
   *
   * @param tag the tag shouldn't be more than 23 characters as {@link Log#isLoggable(String, int)}
   *     has this restriction.
   * @param message the string message to log. This can also be a string format that's recognized by
   *     {@link String#format(String, Object...)}. e.g. "%s did something to %s, and %d happened as
   *     a result".
   * @param args the formatting args for the previous string.
   */
  public static void e(String tag, /*@Nullable*/ String message, /*@Nullable*/ Object... args) {
    internalLog(ERROR, tag, null, message, false, args);
  }

  /**
   * Log an error message.
   *
   * @param tag the tag shouldn't be more than 23 characters as {@link Log#isLoggable(String, int)}
   *     has this restriction.
   * @param th a throwable to log.
   * @param message the string message to log. This can also be a string format that's recognized by
   *     {@link String#format(String, Object...)}. e.g. "%s did something to %s, and %d happened as
   *     a result".
   * @param args the formatting args for the previous string.
   */
  public static void e(
      String tag, Throwable th, /*@Nullable*/ String message, /*@Nullable*/ Object... args) {
    internalLog(ERROR, tag, th, message, false, args);
  }

  /**
   * Log an error message that includes the current stack trace in the log.
   *
   * @param tag the tag shouldn't be more than 23 characters as {@link Log#isLoggable(String, int)}
   *     has this restriction.
   * @param message the string message to log. This can also be a string format that's recognized by
   *     {@link String#format(String, Object...)}. e.g. "%s did something to %s, and %d happened as
   *     a result".
   * @param args the formatting args for the previous string.
   */
  public static void eWithStackTrace(
      String tag, /*@Nullable*/ String message, /*@Nullable*/ Object... args) {
    internalLog(ERROR, tag, null, message, true, args);
  }

  /**
   * What a terrible failure! ASSERT level log. On non-prod(dogfood/dev) builds, this crashes the
   * app. On prod builds, this logs at WARN level.
   *
   * <p>This is for failures that need to be caught during development/dogfood, but which shouldn't
   * crash the app in production.
   *
   * @param tag the tag shouldn't be more than 23 characters as {@link Log#isLoggable(String, int)}
   *     has this restriction.
   * @param message the string message to log. This can also be a string format that's recognized by
   *     {@link String#format(String, Object...)}. e.g. "%s did something to %s, and %d happened as
   *     a result".
   * @param args the formatting args for the previous string.
   */
  public static void wtf(String tag, /*@Nullable*/ String message, /*@Nullable*/ Object... args) {
    internalLog(WARN, tag, null, message, false, args);
    if (shouldWtfCrash()) {
      throw new RuntimeException(buildMessage(message, args));
    }
  }

  /**
   * What a terrible failure! ASSERT level log. On non-prod(dogfood/dev) builds, this crashes the
   * app. On prod builds, this logs at WARN level.
   *
   * <p>This is for failures that need to be caught during development/dogfood, but which shouldn't
   * crash the app in production.
   *
   * @param tag the tag shouldn't be more than 23 characters as {@link Log#isLoggable(String, int)}
   *     has this restriction.
   * @param th a throwable to log.
   * @param message the string message to log. This can also be a string format that's recognized by
   *     {@link String#format(String, Object...)}. e.g. "%s did something to %s, and %d happened as
   *     a result".
   * @param args the formatting args for the previous string.
   */
  public static void wtf(
      String tag, Throwable th, /*@Nullable*/ String message, /*@Nullable*/ Object... args) {
    internalLog(WARN, tag, th, message, false, args);
    if (shouldWtfCrash()) {
      throw new RuntimeException(buildMessage(message, args), th);
    }
  }

  // TODO: this should be consistent across build types
  private static boolean shouldWtfCrash() {
    // L.wtf should crash for DEV and ALPHA builds.
    // Since Proguard is definitely being run for RELEASE builds, the method returns true by
    // default, but Proguard will assume it returns false for RELEASE build.
    return true;
  }

  /**
   * Tests if verbose logging is enabled for a given tag. It's used to guard some potentially
   * expensive logging code.
   *
   * @param tag The tag that we want to test
   * @return true if this tag is enabled for verbose logging, false otherwise
   */
  public static boolean isVerboseEnabled(String tag) {
    return isEnabled(tag, VERBOSE);
  }

  /**
   * Tests if debug logging is enabled for a given tag. It's used to guard some potentially
   * expensive logging code.
   *
   * @param tag The tag that we want to test
   * @return true if this tag is enabled for debug logging, false otherwise
   */
  public static boolean isDebugEnabled(String tag) {
    return isEnabled(tag, DEBUG);
  }

  private static String capTag(String tag) {
    String cappedTag;
    if (tag.length() > 23) {
      cappedTag = tag.substring(0, 23);
      internalLog(
          WARN,
          cappedTag,
          null,
          "Tag [%s] is too long; truncated to [%s]",
          false,
          tag,
          cappedTag);
      return cappedTag;
    }
    return tag;
  }

  public static boolean isEnabled(String tag, int level) {
    String cappedTag = capTag(tag);

    if (Build.VERSION.SDK_INT == Build.VERSION_CODES.M) {
      // Android M has a bug which makes isLoggable() inconsistent. Only suppress debug and verbose
      // level logging, to ensure that log messages of other levels are never suppressed. Note that
      // this effectively disables suppression of the log tag.
      // See: http://g/android-chatty-eng/25uo6B2wraQ/discussion
      return ((level != DEBUG && level != VERBOSE) || Log.isLoggable(cappedTag, level));
    } else {
      return Log.isLoggable(cappedTag, level);
    }
  }

  public static void log(
      int level, String tag, /*@Nullable*/ String message, /*@Nullable*/ Object... args) {
    internalLog(level, tag, null, message, false, args);
  }

  public static void log(
      int level,
      String tag,
      /*@Nullable*/ Throwable th,
      /*@Nullable*/ String message,
      /*@Nullable*/ Object... args) {
    internalLog(level, tag, th, message, false, args);
  }

  // if (sensitive), all args must be instances of Redactable
  // uses array rather than varargs to avoid ambiguous conversion of /*@Nullable*/ Redactable...
  private static void internalLog(
      int level,
      String tag,
      /*@Nullable*/ Throwable th,
      /*@Nullable*/ String message,
      boolean withStackTrace,
      /*@Nullable*/ Object... args) {
    tag = capTag(tag);

    // Return early if we should not be logging this tag at the given level.
    if (!isEnabled(tag, level)) {
      return;
    }

    String formattedMessage = buildMessage(message, args);
    if (th == null && withStackTrace) {
      String throwableMessage;
      if (message == null) {
        throwableMessage = NOT_AN_EXCEPTION;
      } else {
        throwableMessage = formattedMessage;
      }
      th = new DebugStackTraceLogger(tag + ": " + throwableMessage);
    }
    // TODO: Remove self from the stack trace.
    switch (level) {
      case VERBOSE:
        if (th != null) {
          Log.v(tag, formattedMessage, th);
        } else {
          Log.v(tag, formattedMessage);
        }
        break;
      case DEBUG:
        if (th != null) {
          Log.d(tag, formattedMessage, th);
        } else {
          Log.d(tag, formattedMessage);
        }
        break;
      case INFO:
        if (th != null) {
          Log.i(tag, formattedMessage, th);
        } else {
          Log.i(tag, formattedMessage);
        }
        break;
      case WARN:
        if (th != null) {
          Log.w(tag, formattedMessage, th);
        } else {
          Log.w(tag, formattedMessage);
        }
        break;
      case ERROR:
        if (th != null) {
          Log.e(tag, formattedMessage, th);
        } else {
          Log.e(tag, formattedMessage);
        }
        break;
      case ASSERT:
        if (th != null) {
          Log.wtf(tag, formattedMessage, th);
        } else {
          Log.wtf(tag, formattedMessage);
        }
        break;
      default:
        // TODO: What should we do by default.  If we hit this it's a code error in this
        // class.  Handling as verbose...
        if (th != null) {
          Log.v(tag, formattedMessage, th);
        } else {
          Log.v(tag, formattedMessage);
        }
    }
  }

  protected static String buildMessage(/*@Nullable*/ String message, /*@Nullable*/ Object[] args) {
    // If the message is null, ignore the args and return "null";
    if (message == null) {
      return STRING_MEANING_NULL;
    }

    // else if the args are null or 0-length, return message
    if (args == null || args.length == 0) {
      return message;
    }

    // Use deepToString to get a more useful representation of any arrays in args
    for (int i = 0; i < args.length; i++) {
      if (args[i] != null && args[i].getClass().isArray()) {
        // Wrap in an array, deepToString, then remove the extra [] from the wrapper. This
        // allows handling all array types rather than having separate branches for all
        // primitive array types plus Object[].
        String string = Arrays.deepToString(new Object[] {args[i]});
        // Strip the outer [] from the wrapper array.
        args[i] = string.substring(1, string.length() - 1);
      }
    }

    // else try formatting the string.
    try {
      return String.format(Locale.US, message, args);
    } catch (IllegalFormatException ex) {
      return message + Arrays.toString(args);
    }
  }

  /**
   * Print the raw protobuffer with the indicated data using the TAG and the fake html tags marked
   * by &lt;name&gt; and &lt;end-name&gt;
   *
   * <p>A utility called parse-request-response.sh can then be used to pick up the log lines,
   * extract lines called request and response and write them out to a developer's workstation.
   *
   * <p>This should only be done during development or during a limited Alpha. Protocol buffers are
   * large. The ring-buffer in logcat is small, and a shared resource with other apps. Your misuse
   * of logging Protocol buffers limits the utility of logcat to other teams like Maps, Youtube,
   * Play Store. Please do not enable logging of large protocol buffers in production, and please do
   * not remove the IS_DEV_BUILD check.
   *
   * <p>If you find yourself requiring protocol buffers, a much more sensible solution is to write
   * them to disk in /sdcard/, and allow developers to pull this out of dogfood devices. This avoids
   * spamming the log ringbuffer, and also persists beyond a reboot.
   */
  public static void debugLogRawProto(String tag, byte[] rawProto, String name) {
    // Create a string representation of the raw data.
    String request;
    try {
      // A lot is happening in this line.  We are encoding the raw data into Base64: we
      // skip wrapping because the default wrapping is at 76 chars which leads to too many
      // calls into Log.i.  Instead, we chunk up later in logLongString(...) where we
      // break at 2000 chars.  Also, the encoded bytes are then encoded into UTF-8 by the
      // String class which maintains the encoding since the entire charset of Base64
      // encoding fits without modification in utf-8.
      request = new String(Base64.encode(rawProto, Base64.NO_WRAP), "UTF-8");
    } catch (UnsupportedEncodingException e) {
      // Ceaseless wonder!  We failed to do UTF-8 encoding.  Give up with a message
      // that is easy to trace in the source code.
      request = "<Exception: UTF-8 encoding failed in VelvetNetworkClient>";
    }
    // Indicate how to view this data.
    Logger.d(tag, "Use tools/mnc_assist/parse-request-response.sh\n<%s>", name);
    // Log.d truncates long lines.  This is a workaround to print long lines by chunking
    // into 2000 chars per line.  The line limit here is arbitrary but cannot be pushed past
    // 4000 chars.  Keeping it at 3500 or lower should be safe.
    int LENGTH_MAX = 2000;
    int total = request.length();
    for (int start = 0; start < total; start += LENGTH_MAX) {
      // String.substring(...) expects the end to be the string length, never more.
      int end = Math.min(start + LENGTH_MAX, total);
      Logger.d(tag, "%s", request.substring(start, end));
    }
    // End-tag is not a standard HTML </end> because the utility
    // used to parse these treats the backlash characters / in a
    // special way.  Instead we use a marker called end-name instead.
    Logger.d(tag, "\n<end-%s>", name);
  }

  /**
   * Exception subclass that makes it clear that in a log, this is not an exception but a log
   * statement for debugging.
   */
  public static final class DebugStackTraceLogger extends Throwable {

    public DebugStackTraceLogger(String formattedMessage) {
      super(formattedMessage);
    }

    public DebugStackTraceLogger() {
      this(NOT_AN_EXCEPTION);
    }
  }
}
