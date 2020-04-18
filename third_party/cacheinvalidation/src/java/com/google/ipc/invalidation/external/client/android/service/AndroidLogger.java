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

package com.google.ipc.invalidation.external.client.android.service;

import com.google.ipc.invalidation.external.client.SystemResources;
import com.google.ipc.invalidation.external.client.SystemResources.Logger;
import com.google.ipc.invalidation.util.Formatter;

import android.util.Log;

import java.util.HashMap;
import java.util.Map;
import java.util.logging.Level;


/**
 * Provides the implementation of {@link Logger} for Android. The logging tag will be based upon the
 * top-level class name containing the code invoking the logger (the outer class, not an inner or
 * anonymous class name).   For severe and warning level messages, the Android logger will also
 * dump the stack trace of the first argument if it is a throwable.
 */
public class AndroidLogger implements Logger {

  /** Creates a new AndroidLogger that uses the provided value as the Android logging tag */
  public static AndroidLogger forTag(String tag) {
    return new AndroidLogger(tag, null);
  }

  /** Creates a new AndroidLogger that will compute a tag value dynamically based upon the class
   * that calls into the logger and will prepend the provided prefix (if any) on all
   * logged messages.
   */
  public static AndroidLogger forPrefix(String prefix) {
    return new AndroidLogger(null, prefix);
  }

  /**
   * If {@code false}, then Log.isLoggable() is called to filter log messages
   */
  private static boolean filteringDisabled = false;

  /**
   * Maps from a Java {@link Level} to the android {@link Log} priority value used to log
   * messages at that level.
   */
  private static Map<Level, Integer> levelToPriority = new HashMap<Level, Integer>();

  static {
    // Define the mappings for Java log levels to the associated Android log priorities
    levelToPriority.put(Level.INFO, Log.INFO);
    levelToPriority.put(Level.WARNING, Log.WARN);
    levelToPriority.put(Level.SEVERE, Log.ERROR);
    levelToPriority.put(Level.FINE, Log.DEBUG);
    levelToPriority.put(Level.FINER, Log.VERBOSE);
    levelToPriority.put(Level.FINEST, Log.VERBOSE);
    levelToPriority.put(Level.CONFIG, Log.INFO);
  }

  /**
   * Disables log filtering so all logged messages will be captured.
   */
  public static void disableFilteringForTest() {
    filteringDisabled = true;
  }

  /**
   * The default minimum Android log level. We default to 0 to ensure everything is logged.
   * This should be a value from the {@link Log} constants.
   */
  private static int minimumLogLevel = 0;

  /**
   * The maximum length of an Android logging tag. There's no formal constants but the constraint is
   * mentioned in the Log javadoc
   */
  private static final int MAX_TAG_LENGTH = 23;

  /** Constant tag to use for logged messages (or {@code null} to use topmost class on stack */
  private final String tag;

  /** Prefix added to Android logging messages */
  private final String logPrefix;

  /** Creates a logger that prefixes every logging stmt with {@code logPrefix}. */
  private AndroidLogger(String tag, String logPrefix) {
    this.tag = tag;
    this.logPrefix = logPrefix;
  }

  @Override
  public boolean isLoggable(Level level) {
    return isLoggable(getTag(), levelToPriority(level));
  }

  @Override
  public void log(Level level, String template, Object... args) {
    int androidLevel = levelToPriority(level);
    String tag = getTag();
    if (isLoggable(tag, androidLevel)) {
      Log.println(androidLevel, tag, format(template, args));
    }
  }

  @Override
  public void severe(String template, Object...args) {
    String tag = getTag();
    if (isLoggable(tag, Log.ERROR)) {
      // If the first argument is an exception, use the form of Log that will dump a stack trace
      if ((args.length > 0) && (args[0] instanceof Throwable)) {
        Log.e(tag, format(template, args), (Throwable) args[0]);
      } else {
        Log.e(tag, format(template, args));
      }
    }
  }

  @Override
  public void warning(String template, Object...args) {
    String tag = getTag();
    if (isLoggable(tag, Log.WARN)){
      // If the first argument is an exception, use the form of Log that will dump a stack trace
      if ((args.length > 0) && (args[0] instanceof Throwable)) {
        Log.w(tag, format(template, args), (Throwable) args[0]);
      } else {
        Log.w(tag, format(template, args));
      }
    }
  }

  @Override
  public void info(String template, Object...args) {
    String tag = getTag();
    if (isLoggable(tag, Log.INFO)) {
      Log.i(tag, format(template, args));
    }
  }

  @Override
  public void fine(String template, Object...args) {
    String tag = getTag();
    if (isLoggable(tag, Log.DEBUG)) {
      Log.d(tag, format(template, args));
    }
  }

  @Override
  public void setSystemResources(SystemResources resources) {
    // No-op.
  }

  /** Given a Java logging level, returns the corresponding Android log priority. */
  private static int levelToPriority(Level level) {
    Integer priority = levelToPriority.get(level);
    if (priority != null) {
      return priority;
    }
    throw new IllegalArgumentException("Unsupported level: " + level);
  }

  /** Formats the content of a logged messages for output, prepending the log prefix if any. */
  private String format(String template, Object...args) {
    return (logPrefix != null) ?
        ("[" + logPrefix + "] " + Formatter.format(template, args)) :
        Formatter.format(template, args);
  }

  /** Returns the Android logging tag that should be placed on logged messages */
  private String getTag() {
    if (tag != null) {
      return tag;
    }

    StackTraceElement[] stackTrace = new Throwable().getStackTrace();
    String className = null;
    for (int i = 0; i < stackTrace.length; i++) {
      className = stackTrace[i].getClassName();

      // Skip over this class's methods
      if (!className.equals(AndroidLogger.class.getName())) {
        break;
      }
    }

    // Compute the unqualified class name w/out any inner class, then truncate to the
    // maximum tag length.
    int unqualBegin = className.lastIndexOf('.') + 1;
    if (unqualBegin < 0) { // should never happen, but be safe
      unqualBegin = 0;
    }
    int unqualEnd = className.indexOf('$', unqualBegin);
    if (unqualEnd < 0) {
      unqualEnd = className.length();
    }
    if ((unqualEnd - unqualBegin) > MAX_TAG_LENGTH) {
      unqualEnd = unqualBegin + MAX_TAG_LENGTH;
    }
    return className.substring(unqualBegin, unqualEnd);
  }

  /**
   * Add additional constraint on logging. In addition to the normal check of
   * {@link Log#isLoggable(String, int)} for logging, this also requires a minimum
   * log level of the given value. This should be a value from the {@link Log} constants.
   */
  public static void setMinimumAndroidLogLevel(int logLevel) {
    minimumLogLevel = logLevel;
  }

  /**
   * Returns {@code true} is the provided tag/level will produce logged output.
   */
  
  boolean isLoggable(String tag, int priority) {
    return filteringDisabled || (priority >= minimumLogLevel && Log.isLoggable(tag, priority));
  }
}
