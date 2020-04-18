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

import java.util.Arrays;
import java.util.Collections;
import java.util.EnumSet;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

/**
 * Configuration options for the address input widget used to control the visibility and interaction
 * of specific fields to suit specific use cases (eg, collecting business addresses, collecting
 * addresses for credit card verification etc...).
 * <p>
 * When form options are passed to the address widget a snapshot is taken and any further changes to
 * the options are ignored.
 * <p>
 * This design is somewhat like using a builder but has the advantage that the caller only sees the
 * outer (mutable) type and never needs to know about the "built" snapshot. This reduces the public
 * API footprint and simplifies usage of this class.
 */
// This is an external class and part of the widget's public API.
public final class FormOptions {

  // These fields must never be null).
  private Set<AddressField> hiddenFields = EnumSet.noneOf(AddressField.class);
  private Set<AddressField> readonlyFields = EnumSet.noneOf(AddressField.class);
  private Set<String> blacklistedRegions = new HashSet<String>();
  // Key is ISO 2-letter region code.
  private Map<String, List<AddressField>> customFieldOrder =
      new HashMap<String, List<AddressField>>();

  /** Creates an empty, mutable, form options instance. */
  public FormOptions() {
  }

  /**
   * Hides the given address field. Calls to this method <strong>are cumulative</strong>. Fields
   * which are specified here but not part of a country's specified fields will be ignored.
   * <p>
   * Note also that hiding fields occurs after custom ordering has been applied, although combining
   * these two features is not generally recommended due to the confusion it is likely to cause.
   */
  public FormOptions setHidden(AddressField field) {
    hiddenFields.add(field);
    return this;
  }

  /**
   * Sets the given address field as read-only. Calls to this method <strong>are cumulative
   * </strong>. Fields which are specified here but not part of a country's specified fields will be
   * ignored.
   * <p>
   * This method is j2objc- & iOS API friendly as the signature does not expose varargs / Java
   * arrays or collections.
   */
  public FormOptions setReadonly(AddressField field) {
    readonlyFields.add(field);
    return this;
  }

  /**
   * Sets the order of address input fields for the given ISO 3166-1 two letter country code.
   * <p>
   * Input fields affected by custom ordering will be shown in the widget in the order they are
   * given to this method (for the associated region code). Fields which are visible for a region,
   * but which are not specified here, will appear in their original position in the form. For
   * example, if a region defines the following fields:
   * <pre>
   * [ RECIPIENT -> ORGANIZATION -> STREET_ADDRESS -> LOCALITY -> ADMIN_AREA -> COUNTRY ]
   * </pre>
   * and the custom ordering for that region is (somewhat contrived):
   * <pre>
   * [ ORGANIZATION -> COUNTRY -> RECIPIENT ]
   * </pre>
   * Then the visible order of the input fields will be:
   * <pre>
   * [ ORGANIZATION -> COUNTRY -> STREET_ADDRESS -> LOCALITY -> ADMIN_AREA -> RECIPIENT ]
   * </pre>
   * <ul>
   * <li>Fields not specified in the custom ordering (STREET_ADDRESS, LOCALITY, ADMIN_AREA)
   * remain in their original, absolute, positions.
   * <li>Custom ordered fields are re-positioned such that their relative order is now as
   * specified (but other, non custom-ordered, fields can appear between them).
   * </ul>
   * <p>
   * If the custom order contains a field which is not present for the specified region, it is
   * silently ignored. Setting a custom ordering can never be used as a way to add fields for a
   * region.
   * <p>
   * Typically this feature is used to reverse things like RECIPIENT and ORGANIZATION for certain
   * business related use cases. It should not be used to "correct" perceived bad field ordering
   * or make different countries "more consistent with each other".
   */
  public FormOptions setCustomFieldOrder(String regionCode, AddressField... fields) {
    // TODO: Consider checking the given region code for validity against RegionDataConstants.
    List<AddressField> fieldList = Collections.unmodifiableList(Arrays.asList(fields));
    if (fieldList.size() > 0) {
      if (EnumSet.copyOf(fieldList).size() != fieldList.size()) {
        throw new IllegalArgumentException("duplicate address field: " + fieldList);
      }
      customFieldOrder.put(regionCode, fieldList);
    } else {
      customFieldOrder.remove(regionCode);
    }
    return this;
  }

  /**
   * Blacklist the given CLDR (Common Locale Data Repository) region (country) code
   * indicating countries that for legal or other reasons should not be available.
   * <p>
   * Calls are cumulative, call this method once for each region that needs to be blacklisted.
   * <p>
   * We reserve the right to change this API from taking individual regions to taking a set.
   */
  public FormOptions blacklistRegion(String regionCode) {
    if (regionCode == null) {
      throw new NullPointerException();
    }
    // TODO(addresswidget-team): Add region code validation against RegionDataConstants.
    blacklistedRegions.add(Util.toUpperCaseLocaleIndependent(regionCode));
    return this;
  }

  /** Returns an immutable snapshot of the current state of the form options. */
  public Snapshot createSnapshot() {
    return new Snapshot(this);
  }

  /** An immutable snapshot of a {@code FormOptions} instance. */
  public static class Snapshot {
    private final Set<AddressField> hiddenFields;
    private final Set<AddressField> readonlyFields;
    private final Set<String> blacklistedRegions;
    private final Map<String, List<AddressField>> customFieldOrder;

    Snapshot(FormOptions options) {
      this.hiddenFields = Collections.unmodifiableSet(EnumSet.copyOf(options.hiddenFields));
      this.readonlyFields = Collections.unmodifiableSet(EnumSet.copyOf(options.readonlyFields));
      // Shallow copy as lists are already immutable.
      this.customFieldOrder = Collections.unmodifiableMap(
          new HashMap<String, List<AddressField>>(options.customFieldOrder));
      this.blacklistedRegions =
          Collections.unmodifiableSet(new HashSet<String>(options.blacklistedRegions));
    }

    public boolean isHidden(AddressField field) {
      return hiddenFields.contains(field);
    }

    public boolean isReadonly(AddressField field) {
      return readonlyFields.contains(field);
    }

    /**
     * Gets the overridden field orders with their corresponding region code. Returns null if field
     * orders for {@code regionCode} is not specified.
     */
    List<AddressField> getCustomFieldOrder(String regionCode) {
      return customFieldOrder.get(regionCode);
    }

    public boolean isBlacklistedRegion(String regionCode) {
      return blacklistedRegions.contains(Util.toUpperCaseLocaleIndependent(regionCode));
    }
  }
}
