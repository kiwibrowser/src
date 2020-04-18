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

import java.util.HashMap;
import java.util.Map;

/**
 * Enumerates all the data fields found in the JSON-format address property data that are used by
 * the Android Address Input Widget.
 */
public enum AddressDataKey {
  /**
   * Identifies the countries for which data is provided.
   */
  COUNTRIES,
  /**
   * The standard format string. This identifies which fields can be used in the address, along with
   * their order. This also carries additional information for use in formatting the fields into
   * multiple lines. This is also used to indicate which fields should _not_ be used for an address.
   */
  FMT,
  /**
   * The unique ID of the region, in the form of a path from parent IDs to the key.
   */
  ID,
  /**
   * The key of the region, unique to its parent. If there is an accepted abbreviation for this
   * region, then the key will be set to this and name will be set to the local name for this
   * region. If there is no accepted abbreviation, then this key will be the local name and there
   * will be no local name specified. This value must be present.
   */
  KEY,
  /**
   * The default language of any data for this region, if known.
   */
  LANG,
  /**
   * The languages used by any data for this region, if known.
   */
  LANGUAGES,
  /**
   * The latin format string {@link #FMT} used when a country defines an alternative format for
   * use with the latin script, such as in China.
   */
  LFMT,
  /**
   * Indicates the type of the name used for the locality (city) field.
   */
  LOCALITY_NAME_TYPE,
  /**
   * Indicates which fields must be present in a valid address.
   */
  REQUIRE,
  /**
   * Indicates the type of the name used for the state (administrative area) field.
   */
  STATE_NAME_TYPE,
  /**
   * Indicates the type of the name used for the sublocality field.
   */
  SUBLOCALITY_NAME_TYPE,
  /**
   * Encodes the {@link #KEY} value of all the children of this region.
   */
  SUB_KEYS,
  /**
   * Encodes the transliterated latin name value of all the children of this region, if the local
   * names are not in latin script already.
   */
  SUB_LNAMES,
  /**
   * Indicates, for each child of this region, whether that child has additional children.
   */
  SUB_MORES,
  /**
   * Encodes the local name value of all the children of this region.
   */
  SUB_NAMES,
  /**
   * Encodes width overrides for specific fields.
   */
  WIDTH_OVERRIDES,
  /**
   * Encodes the {@link #ZIP} value for the subtree beneath this region.
   */
  XZIP,
  /**
   * Encodes the postal code pattern if at the country level, and the postal code prefix if at a
   * level below country.
   */
  ZIP,
  /**
   * Indicates the type of the name used for the ZIP (postal code) field.
   */
  ZIP_NAME_TYPE;

  /**
   * Returns a field based on its keyname (value in the JSON-format file), or null if no field
   * matches.
   */
  static AddressDataKey get(String keyname) {
    return ADDRESS_KEY_NAME_MAP.get(Util.toLowerCaseLocaleIndependent(keyname));
  }

  private static final Map<String, AddressDataKey> ADDRESS_KEY_NAME_MAP =
      new HashMap<String, AddressDataKey>();

  static {
    // Populates the map of enums against their lower-cased string values for easy look-up.
    for (AddressDataKey field : values()) {
      ADDRESS_KEY_NAME_MAP.put(Util.toLowerCaseLocaleIndependent(field.toString()), field);
    }
  }
}
