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

package com.google.i18n.addressinput.common;

/**
 * Enumerates problems that default address verification can report.
 */
// This is an external class and part of the widget's public API.
// TODO: Review public API for external classes.
public enum AddressProblemType {
  /**
   * The field is not null and not whitespace, and the field should not be used by addresses in this
   * country.
   * <p>
   * For example, in the U.S. the SORTING_CODE field is unused, so its presence is an
   * error. This cannot happen when using the Address Widget to enter an address.
   */
  UNEXPECTED_FIELD,

  /**
   * The field is null or whitespace, and the field is required for addresses in this country.
   * <p>
   * For example, in the U.S. ADMIN_AREA is a required field.
   */
  MISSING_REQUIRED_FIELD,

  /**
   * A list of values for the field is defined and the value does not occur in the list. Applies
   * to hierarchical elements like REGION, ADMIN_AREA, LOCALITY, and DEPENDENT_LOCALITY.
   *
   * <p>For example, in the U.S. the only valid values for ADMIN_AREA are the two-letter state
   * codes.
   */
  UNKNOWN_VALUE,

  /**
   * A format for the field is defined and the value does not match. This is used to match
   * POSTAL_CODE against the general format pattern. Formats indicate how many digits/letters should
   * be present, and what punctuation is allowed.
   * <p>
   * For example, in the U.S. postal codes are five digits with an optional hyphen followed by
   * four digits.
   */
  INVALID_FORMAT,

  /**
   * A specific pattern for the field is defined and the value does not match. This is used to match
   * example) and the value does not match. This is used to match POSTAL_CODE against a regular
   * expression.
   * <p>
   * For example, in the US postal codes in the state of California start with a '9'.
   */
  MISMATCHING_VALUE;

  /**
   * Returns a unique string identifying this problem (for use in a message catalog).
   */
  public String keyname() {
    return Util.toLowerCaseLocaleIndependent(name());
  }
}
