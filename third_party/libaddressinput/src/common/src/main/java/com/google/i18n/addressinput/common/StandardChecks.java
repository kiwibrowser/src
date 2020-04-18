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

import static com.google.i18n.addressinput.common.AddressField.ADMIN_AREA;
import static com.google.i18n.addressinput.common.AddressField.COUNTRY;
import static com.google.i18n.addressinput.common.AddressField.DEPENDENT_LOCALITY;
import static com.google.i18n.addressinput.common.AddressField.LOCALITY;
import static com.google.i18n.addressinput.common.AddressField.ORGANIZATION;
import static com.google.i18n.addressinput.common.AddressField.POSTAL_CODE;
import static com.google.i18n.addressinput.common.AddressField.RECIPIENT;
import static com.google.i18n.addressinput.common.AddressField.SORTING_CODE;
import static com.google.i18n.addressinput.common.AddressField.STREET_ADDRESS;
import static com.google.i18n.addressinput.common.AddressProblemType.INVALID_FORMAT;
import static com.google.i18n.addressinput.common.AddressProblemType.MISMATCHING_VALUE;
import static com.google.i18n.addressinput.common.AddressProblemType.MISSING_REQUIRED_FIELD;
import static com.google.i18n.addressinput.common.AddressProblemType.UNEXPECTED_FIELD;
import static com.google.i18n.addressinput.common.AddressProblemType.UNKNOWN_VALUE;

import java.util.Arrays;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * Loader for a map defining the standard checks to perform on AddressFields.
 */
public final class StandardChecks {
  private StandardChecks() {
  }

  public static final Map<AddressField, List<AddressProblemType>> PROBLEM_MAP;

  static {
    Map<AddressField, List<AddressProblemType>> map =
        new HashMap<AddressField, List<AddressProblemType>>();

    addToMap(map, COUNTRY, UNEXPECTED_FIELD, MISSING_REQUIRED_FIELD, UNKNOWN_VALUE);
    addToMap(map, ADMIN_AREA, UNEXPECTED_FIELD, MISSING_REQUIRED_FIELD, UNKNOWN_VALUE);
    addToMap(map, LOCALITY, UNEXPECTED_FIELD, MISSING_REQUIRED_FIELD, UNKNOWN_VALUE);
    addToMap(map, DEPENDENT_LOCALITY, UNEXPECTED_FIELD, MISSING_REQUIRED_FIELD, UNKNOWN_VALUE);
    addToMap(map, POSTAL_CODE, UNEXPECTED_FIELD, MISSING_REQUIRED_FIELD, INVALID_FORMAT,
        MISMATCHING_VALUE);
    addToMap(map, STREET_ADDRESS, UNEXPECTED_FIELD, MISSING_REQUIRED_FIELD);
    addToMap(map, SORTING_CODE, UNEXPECTED_FIELD, MISSING_REQUIRED_FIELD);
    addToMap(map, ORGANIZATION, UNEXPECTED_FIELD, MISSING_REQUIRED_FIELD);
    addToMap(map, RECIPIENT, UNEXPECTED_FIELD, MISSING_REQUIRED_FIELD);

    PROBLEM_MAP = Collections.unmodifiableMap(map);
  }

  private static void addToMap(Map<AddressField, List<AddressProblemType>> map, AddressField field,
      AddressProblemType... problems) {
    map.put(field, Collections.unmodifiableList(Arrays.asList(problems)));
  }
}
