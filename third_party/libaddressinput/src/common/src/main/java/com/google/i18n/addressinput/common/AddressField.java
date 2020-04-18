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

import java.util.Collections;
import java.util.HashMap;
import java.util.Map;

/**
 * Identifiers for the input fields of the address widget, used to control options related to
 * visibility and ordering of UI elements. Note that one {@code AddressField} may represent more
 * than one input field in the UI (eg, {@link #STREET_ADDRESS}), but each input field can be
 * identified by exactly one {@code AddressField}.
 * <p>
 * In certain use cases not all fields are necessary, and you can hide fields using
 * {@link FormOptions#setHidden}. An example of this is when you are collecting postal addresses not
 * intended for delivery and wish to suppress the collection of a recipient's name or organization.
 * <p>
 * An alternative to hiding fields is to make them read-only, using {@link FormOptions#setReadonly}.
 * An example of this would be in the case that the country of an address was already determined but
 * we wish to make it clear to the user that we have already taken it into account and do not want
 * it entered again.
 *
 * @see FormOptions
 */
public enum AddressField {
  /** The drop-down menu used to select a region for {@link AddressData#getPostalCountry()}. */
  COUNTRY('R'),
  /**
   * The input field used to enter a value for {@link AddressData#getAddressLine1()}.
   * @deprecated Use {@link #STREET_ADDRESS} instead.
   */
  @Deprecated
  ADDRESS_LINE_1('1'),
  /**
   * The input field used to enter a value for {@link AddressData#getAddressLine2()}.
   * @deprecated Use {@link #STREET_ADDRESS} instead.
   */
  @Deprecated
  ADDRESS_LINE_2('2'),
  /** The input field(s) used to enter values for {@link AddressData#getAddressLines()}. */
  STREET_ADDRESS('A'),
  /** The input field used to enter a value for {@link AddressData#getAdministrativeArea()}. */
  ADMIN_AREA('S'),
  /** The input field used to enter a value for {@link AddressData#getLocality()}. */
  LOCALITY('C'),
  /** The input field used to enter a value for {@link AddressData#getDependentLocality()}. */
  DEPENDENT_LOCALITY('D'),
  /** The input field used to enter a value for {@link AddressData#getPostalCode()}. */
  POSTAL_CODE('Z'),
  /** The input field used to enter a value for {@link AddressData#getSortingCode()}. */
  SORTING_CODE('X'),

  /** The input field used to enter a value for {@link AddressData#getRecipient()}. */
  RECIPIENT('N'),
  /** The input field used to enter a value for {@link AddressData#getOrganization()}. */
  ORGANIZATION('O');

  /** Classification of the visual width of address input fields. */
  public enum WidthType {
    /**
     * Identifies an input field as accepting full-width input, such as address lines or recipient.
     */
    LONG,
    /**
     * Identifies an input field as accepting short (often bounded) input, such as postal code.
     */
    SHORT;

    static WidthType of(char c) {
      switch (c) {
        // In case we need a 'narrow'. Map it to 'S' for now to facilitate the rollout.
        case 'N':
        case 'S':
          return SHORT;
        case 'L':
          return LONG;
        default:
          throw new IllegalArgumentException("invalid width character: " + c);
      }
    }
  }

  private static final Map<Character, AddressField> FIELD_MAPPING;

  static {
    Map<Character, AddressField> map = new HashMap<Character, AddressField>();
    for (AddressField value : values()) {
      map.put(value.getChar(), value);
    }
    FIELD_MAPPING = Collections.unmodifiableMap(map);
  }

  // Defines the character codes used in the metadata to specify the types of fields used in
  // address formatting. Note that the metadata also has a character for newlines, which is
  // not defined here.
  private final char idChar;

  private AddressField(char c) {
    this.idChar = c;
  }

  /**
   * Returns the AddressField corresponding to the given identification character.
   *
   * @throws IllegalArgumentException if the identifier does not correspond to a valid field.
   */
  static AddressField of(char c) {
    AddressField field = FIELD_MAPPING.get(c);
    if (field == null) {
      throw new IllegalArgumentException("invalid field character: " + c);
    }
    return field;
  }

  /**
   * Returns the field's identification character, as used in the metadata.
   *
   * @return identification char.
   */
  char getChar() {
    return idChar;
  }

  /**
   * Returns default width of this address field. This may be overridden for a specific country when
   * we have data for the possible inputs in that field and use a drop-down, rather than a text
   * field, in the UI.
   */
  // TODO: We'd probably be better off just having a widthType field in the enum.
  private WidthType getDefaultWidthType() {
    return this.equals(POSTAL_CODE) || this.equals(SORTING_CODE) ? WidthType.SHORT : WidthType.LONG;
  }

  /**
   * Returns default width of this address field. Takes per-country heuristics into account for
   * text input fields. This may be overridden for a specific country when we have data for the
   * possible inputs in that field and use a drop-down, rather than a text field, in the UI.
   */
  public WidthType getWidthTypeForRegion(String regionCode) {
    Util.checkNotNull(regionCode);
    WidthType width = FormatInterpreter.getWidthOverride(this, regionCode);
    return width != null ? width : getDefaultWidthType();
  }
}
