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

package com.google.ipc.invalidation.util;

import java.util.logging.Level;

/**
 * A basic formatting logger interface.
 *
 */
public interface BaseLogger {
  /**
   * Logs a message.
   *
   * @param level the level at which the message should be logged (e.g., {@code INFO})
   * @param template the string to log, optionally containing %s sequences
   * @param args variables to substitute for %s sequences in {@code template}
   */
  void log(Level level, String template, Object... args);

  /**
   * Returns true iff statements at {@code level} are not being suppressed.
   */
  boolean isLoggable(Level level);

  /**
   * Logs a message at the SEVERE level.
   * See specs of {@code #log} for the parameters.
   */
  void severe(String template, Object...args);

  /**
   * Logs a message at the WARNING level.
   * See specs of {@code #log} for the parameters.
   */
  void warning(String template, Object...args);

  /**
   * Logs a message at the INFO level.
   * See specs of {@code #log} for the parameters.
   */
  void info(String template, Object...args);

  /**
   * Logs a message at the FINE level.
   * See specs of {@code #log} for the parameters.
   */
  void fine(String template, Object...args);
}
