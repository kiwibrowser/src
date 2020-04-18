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

import static com.google.i18n.addressinput.common.Util.checkNotNull;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.EnumSet;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * An immutable data structure for international postal addresses, built using the nested
 * {@link Builder} class.
 * <p>
 * Addresses may seem simple, but even within the US there are many quirks (hyphenated street
 * addresses, etc.), and internationally addresses vary a great deal. The most sane and complete in
 * many ways is the OASIS "extensible Address Language", xAL, which is a published and documented
 * XML schema:<br>
 * <a href="http://www.oasis-open.org/committees/ciq/download.shtml">
 * http://www.oasis-open.org/committees/ciq/download.shtml</a>
 * <p>
 * An example address:
 * <pre>
 * postalCountry: US
 * streetAddress: 1098 Alta Ave
 * adminstrativeArea: CA
 * locality: Mountain View
 * dependentLocality:
 * postalCode: 94043
 * sortingCode:
 * organization: Google
 * recipient: Chen-Kang Yang
 * language code: en
 * </pre>
 * <p>
 * When using this class it's advised to do as little pre- or post-processing of the fields as
 * possible. Typically we expect instances of this class to be used by the address widget and then
 * transmitted or converted to other representations using the standard conversion libraries or
 * formatted using one of the supported formatters. Attempting to infer semantic information from
 * the values of the fields in this class is generally a bad idea.
 * <p>
 * Specifically the {@link #getFieldValue(AddressField)} is a problematic API as it encourages the
 * belief that it is semantically correct to iterate over the fields in order. In general it should
 * not be necessary to iterate over the fields in this class; instead use just the specific getter
 * methods for the fields you need.
 * <p>
 * There are valid use cases for examining individual fields, but these are almost always region
 * dependent:
 * <ul>
 * <li>Finding the region of the address. This is really the only completely safe field you can
 * examine which gives an unambiguous and well defined result under all circumstances.
 * <li>Testing if two addresses have the same administrative area. This is only reliable if the
 * data was entered via a drop-down menu, and the size of administrative areas varies greatly
 * between and within countries so it doesn't infer much about locality.
 * <li>Extracting postal codes or sorting codes for address validation or external lookup. This
 * only works for certain countries, such as the United Kingdom, where postal codes have a high
 * resolution.
 * </ul>
 * <p>
 * All values stored in this class are trimmed of ASCII whitespace. Setting an empty, or whitespace
 * only field in the builder will clear it and result in a {@code null} being returned from the
 * corresponding {@code AddressData} instance..
 */
// This is an external class and part of the widget's public API.
// TODO: Review public API for external classes and tidy JavaDoc.
public final class AddressData {
  // The list of deprecated address fields which are superseded by STREET_ADDRESS.
  @SuppressWarnings("deprecation")  // For legacy address fields.
  private static final List<AddressField> ADDRESS_LINE_FIELDS = Collections.unmodifiableList(
      Arrays.asList(AddressField.ADDRESS_LINE_1, AddressField.ADDRESS_LINE_2));

  private static final int ADDRESS_LINE_COUNT = ADDRESS_LINE_FIELDS.size();

  // The set of address fields for which a single string value can be mapped.
  private static final EnumSet<AddressField> SINGLE_VALUE_FIELDS;
  static {
    SINGLE_VALUE_FIELDS = EnumSet.allOf(AddressField.class);
    SINGLE_VALUE_FIELDS.removeAll(ADDRESS_LINE_FIELDS);
    SINGLE_VALUE_FIELDS.remove(AddressField.STREET_ADDRESS);
  }

  // When this is merged for use by GWT, remember to add @NonFinalForGwt in place of final fields.

  // Detailed information on these fields is available in the javadoc for their respective getters.

  // CLDR (Common Locale Data Repository) country code.
  private final String postalCountry;

  // The most specific part of any address. They may be left empty if more detailed fields are used
  // instead, or they may be used in addition to these if the more detailed fields do not fulfill
  // requirements, or they may be used instead of more detailed fields to represent the street-level
  // part.
  private final List<String> addressLines;

  // Top-level administrative subdivision of this country.
  private final String administrativeArea;

  // Generally refers to the city/town portion of an address.
  private final String locality;

  // Dependent locality or sublocality. Used for neighborhoods or suburbs.
  private final String dependentLocality;

  // Values are frequently alphanumeric.
  private final String postalCode;

  // This corresponds to the SortingCode sub-element of the xAL PostalServiceElements element.
  // Use is very country-specific.
  private final String sortingCode;

  // The firm or organization. This goes at a finer granularity than address lines in the address.
  private final String organization;

  // The recipient. This goes at a finer granularity than address lines in the address. Not present
  // in xAL.
  private final String recipient;

  // BCP-47 language code for the address. Can be set to null.
  private final String languageCode;

  // NOTE: If you add a new field which is semantically significant, you must also add a check for
  // that field in {@link equals} and {@link hashCode}.

  private AddressData(Builder builder) {
    this.postalCountry = builder.fields.get(AddressField.COUNTRY);
    this.administrativeArea = builder.fields.get(AddressField.ADMIN_AREA);
    this.locality = builder.fields.get(AddressField.LOCALITY);
    this.dependentLocality = builder.fields.get(AddressField.DEPENDENT_LOCALITY);
    this.postalCode = builder.fields.get(AddressField.POSTAL_CODE);
    this.sortingCode = builder.fields.get(AddressField.SORTING_CODE);
    this.organization = builder.fields.get(AddressField.ORGANIZATION);
    this.recipient = builder.fields.get(AddressField.RECIPIENT);
    this.addressLines = Collections.unmodifiableList(
        normalizeAddressLines(new ArrayList<String>(builder.addressLines)));
    this.languageCode = builder.language;
  }

  // Helper to normalize a list of address lines. The input may contain null entries or strings
  // which must be split into multiple lines. The resulting list entries will be trimmed
  // consistently with String.trim() and any empty results are ignored.
  // TODO: Trim entries properly with respect to Unicode whitespace.
  private static List<String> normalizeAddressLines(List<String> lines) {
    // Guava equivalent code for each line would look something like:
    // Splitter.on("\n").trimResults(CharMatcher.inRange('\0', ' ')).omitEmptyStrings();
    for (int index = 0; index < lines.size(); ) {
      String line = lines.remove(index);
      if (line == null) {
        continue;
      }
      if (line.contains("\n")) {
        for (String splitLine : line.split("\n")) {
          index = maybeAddTrimmedLine(splitLine, lines, index);
        }
      } else {
        index = maybeAddTrimmedLine(line, lines, index);
      }
    }
    return lines;
  }

  // Helper to trim a string and (if not empty) add it to the given list at the specified index.
  // Returns the new index at which any following elements should be added.
  private static int maybeAddTrimmedLine(String line, List<String> lines, int index) {
    line = Util.trimToNull(line);
    if (line != null) {
      lines.add(index++, line);
    }
    return index;
  }

  /**
   * Returns a string representation of the address, used for debugging.
   */
  @Override
  public String toString() {
    StringBuilder output = new StringBuilder("(AddressData: "
        + "POSTAL_COUNTRY=" + postalCountry + "; "
        + "LANGUAGE=" + languageCode + "; ");
    for (String line : addressLines) {
      output.append(line + "; ");
    }
    output.append("ADMIN_AREA=" + administrativeArea + "; "
        + "LOCALITY=" + locality + "; "
        + "DEPENDENT_LOCALITY=" + dependentLocality + "; "
        + "POSTAL_CODE=" + postalCode + "; "
        + "SORTING_CODE=" + sortingCode + "; "
        + "ORGANIZATION=" + organization + "; "
        + "RECIPIENT=" + recipient
        + ")");
    return output.toString();
  }

  @Override
  public boolean equals(Object o) {
    if (o == this) {
      return true;
    }
    if (!(o instanceof AddressData)) {
      return false;
    }
    AddressData addressData = (AddressData) o;

    return (postalCountry == null
            ? addressData.getPostalCountry() == null
            : postalCountry.equals(addressData.getPostalCountry()))
        && (addressLines == null
            ? addressData.getAddressLines() == null
            : addressLines.equals(addressData.getAddressLines()))
        && (administrativeArea == null
            ? addressData.getAdministrativeArea() == null
            : this.getAdministrativeArea().equals(addressData.getAdministrativeArea()))
        && (locality == null
            ? addressData.getLocality() == null
            : locality.equals(addressData.getLocality()))
        && (dependentLocality == null
            ? addressData.getDependentLocality() == null
            : dependentLocality.equals(addressData.getDependentLocality()))
        && (postalCode == null
            ? addressData.getPostalCode() == null
            : postalCode.equals(addressData.getPostalCode()))
        && (sortingCode == null
            ? addressData.getSortingCode() == null
            : sortingCode.equals(addressData.getSortingCode()))
        && (organization == null
            ? addressData.getOrganization() == null
            : organization.equals(addressData.getOrganization()))
        && (recipient == null
            ? addressData.getRecipient() == null
            : recipient.equals(addressData.getRecipient()))
        && (languageCode == null
            ? this.getLanguageCode() == null
            : languageCode.equals(addressData.getLanguageCode()));
  }

  @Override
  public int hashCode() {
    // 17 and 31 are arbitrary seed values.
    int result = 17;

    String[] fields =
        new String[] {
          postalCountry,
          administrativeArea,
          locality,
          dependentLocality,
          postalCode,
          sortingCode,
          organization,
          recipient,
          languageCode
        };

    for (String field : fields) {
      result = 31 * result + (field == null ? 0 : field.hashCode());
    }

    // The only significant field which is not a String.
    result = 31 * result + (addressLines == null ? 0 : addressLines.hashCode());

    return result;
  }

  /**
   * Returns the CLDR region code for this address; note that this is <em>not</em> the same as the
   * ISO 3166-1 2-letter country code. While technically optional, this field will always be set
   * by the address widget when an address is entered or edited, and will be assumed to be set by
   * many other tools.
   * <p>
   * While they have most of their values in common with the CLDR region codes, the ISO 2-letter
   * country codes have one significant disadvantage; they are not stable and values can change over
   * time. For example {@code "CS"} was originally used to represent Czechoslovakia, but later
   * represented Serbia and Montenegro. In contrast, CLDR region codes are never reused and can
   * represent more regions, such as Ascension Island (AC).
   * <p>
   * See the page on
   * <a href="http://unicode.org/cldr/charts/26/supplemental/territory_containment_un_m_49.html">
   * Territory Containment</a> for a list of CLDR region codes.
   * <p>
   * Note that the region codes not user-presentable; "GB" is Great Britain but this should always
   * be displayed to a user as "UK" or "United Kingdom".
   */
  public String getPostalCountry() {
    return postalCountry;
  }

  /**
   * Returns multiple free-form address lines representing low level parts of an address,
   * possibly empty. The first line represents the lowest level part of the address, other than
   * recipient or organization.
   * <p>
   * Note that the number of lines returned by this method may be greater than the number of lines
   * set on the original builder if some of the lines contained embedded newlines. The values
   * returned by this method will never contain embedded newlines.
   * <p>
   * For example:
   * <pre>{@code
   *   data = AddressData.builder()
   *       .setAddressLine1("First line\nSecond line")
   *       .setAddressLine2("Last line")
   *       .build();
   *   // We end up with 3 lines in the final AddressData instance:
   *   // data.getAddressLines() == [ "First line", "Second line", "Last line" ]
   * }</pre>
   */
  public List<String> getAddressLines() {
    return addressLines;
  }

  /** @deprecated Use {@link #getAddressLines} in preference. */
  @Deprecated
  public String getAddressLine1() {
    return getAddressLine(1);
  }

  /** @deprecated Use {@link #getAddressLines} in preference. */
  @Deprecated
  public String getAddressLine2() {
    return getAddressLine(2);
  }

  // Helper for returning the Nth address line. This is split out here so that it's easily to
  // change the maximum number of address lines we support.
  private String getAddressLine(int lineNumber) {
    // If not the last available line, OR if we're the last line but there are no extra lines...
    if (lineNumber < ADDRESS_LINE_COUNT || lineNumber >= addressLines.size()) {
      return (lineNumber <= addressLines.size()) ? addressLines.get(lineNumber - 1) : null;
    }
    // We're asking for the last available line and there are additional lines in the data.
    // Here it should be true that: lineNumber == ADDRESS_LINE_COUNT
    // Guava equivalent:
    // return Joiner.on(", ")
    //     .join(addressLines.subList(ADDRESS_LINE_COUNT - 1, addressLines.size()));
    StringBuilder joinedLastLine = new StringBuilder(addressLines.get(lineNumber - 1));
    for (String line : addressLines.subList(lineNumber, addressLines.size())) {
      joinedLastLine.append(", ").append(line);
    }
    return joinedLastLine.toString();
  }

  /**
   * Returns the top-level administrative subdivision of this country. Different postal countries
   * use different names to refer to their administrative areas. For example: "state" (US), "region"
   * (Italy) or "prefecture" (Japan).
   * <p>
   * Where data is available, the user will be able to select the administrative area name from a
   * drop-down list, ensuring that it has only expected values. However this is not always possible
   * and no strong assumptions about validity should be made by the user for this value.
   */
  public String getAdministrativeArea() {
    return administrativeArea;
  }

  /**
   * Returns the language specific locality, if present. The usage of this field varies by region,
   * but it generally refers to the "city" or "town" of the address. Some regions do not use this
   * field; their address lines combined with things like postal code or administrative area are
   * sufficient to locate an address.
   * <p>
   * Different countries use different names to refer to their localities. For example: "city" (US),
   * "comune" (Italy) or "post town" (Great Britain). For Japan this would return the shikuchouson
   * and sub-shikuchouson.
   */
  public String getLocality() {
    return locality;
  }

  /**
   * Returns the dependent locality, if present.
   * <p>
   * This is used for neighborhoods and suburbs. Typically a dependent locality will represent a
   * smaller geographical area than a locality, but need not be contained within it.
   */
  public String getDependentLocality() {
    return dependentLocality;
  }

  /**
   * Returns the postal code of the address, if present. This value is not language specific but
   * may contain arbitrary formatting characters such as spaces or hyphens and might require
   * normalization before any meaningful comparison of values.
   * <p>
   * For example: "94043", "94043-1351", "SW1W", "SW1W 9TQ".
   */
  public String getPostalCode() {
    return postalCode;
  }

  /**
   * Returns the sorting code if present. Sorting codes are distinct from postal codes and only
   * used in a handful of regions (eg, France).
   * <p>
   * For example in France this field would contain a
   * <a href="http://en.wikipedia.org/wiki/List_of_postal_codes_in_France">CEDEX</a> value.
   */
  public String getSortingCode() {
    return sortingCode;
  }

  /**
   * Returns the free form organization string, if present. No assumptions should be made about
   * the contents of this field. This field exists because in some situations the organization
   * and recipient fields must be treated specially during formatting. It is not a good idea to
   * allow users to enter the organization or recipient in the street address lines as this will
   * result in badly formatted and non-geocodeable addresses.
   */
  public String getOrganization() {
    return organization;
  }

  /**
   * Returns the free form recipient string, if present. No assumptions should be made about the
   * contents of this field. This field exists because in some situations the organization
   * and recipient fields must be treated specially during formatting. It is not a good idea to
   * allow users to enter the organization or recipient in the street address lines as this will
   * result in badly formatted and non-geocodeable addresses.
   */
  public String getRecipient() {
    return recipient;
  }

  /**
   * Returns a value for those address fields which map to a single string value.
   * <p>
   * Note that while it is possible to pass {@link AddressField#ADDRESS_LINE_1} and
   * {@link AddressField#ADDRESS_LINE_2} into this method, these fields are deprecated and will be
   * removed. In general you should be using named methods to obtain specific values for the address
   * (eg, {@link #getAddressLines()}) and avoid iterating in a general way over the fields.
   * This method has very little value outside of the widget itself and is scheduled for removal.
   *
   * @deprecated Do not use; scheduled for removal from the public API.
   */
  @Deprecated
  @SuppressWarnings("deprecation")
  // TODO: Move this to a utility method rather than exposing it in the public API.
  public String getFieldValue(AddressField field) {
    switch (field) {
      case COUNTRY:
        return postalCountry;
      case ADMIN_AREA:
        return administrativeArea;
      case LOCALITY:
        return locality;
      case DEPENDENT_LOCALITY:
        return dependentLocality;
      case POSTAL_CODE:
        return postalCode;
      case SORTING_CODE:
        return sortingCode;
      case ADDRESS_LINE_1:
        return getAddressLine1();
      case ADDRESS_LINE_2:
        return getAddressLine2();
      case ORGANIZATION:
        return organization;
      case RECIPIENT:
        return recipient;
      default:
        throw new IllegalArgumentException("multi-value fields not supported: " + field);
    }
  }

  /**
   * Returns the BCP-47 language code for this address which defines the language we expect to be
   * used for any language specific fields. If this method returns {@code null} then the language
   * is assumed to be in the default (most used) language for the region code of the address;
   * although the precise determination of a default language is often approximate and may change
   * over time. Wherever possible it is recommended to construct {@code AddressData} instances
   * with a specific language code.
   * <p>
   * Languages are used to guide how the address is <a
   * href="http://en.wikipedia.org/wiki/Mailing_address_format_by_country"> formatted for
   * display</a>. The same address may have different {@link AddressData} representations in
   * different languages. For example, the French name of "New Mexico" is "Nouveau-Mexique".
   */
  public String getLanguageCode() {
    return languageCode;
  }

  /** Returns a new builder to construct an {@code AddressData} instance. */
  public static Builder builder() {
    return new Builder();
  }

  /** Returns a new builder to construct an {@code AddressData} instance. */
  public static Builder builder(AddressData address) {
    return builder().set(address);
  }

  /** Builder for AddressData. */
  public static final class Builder {
    // A map of single value address fields to their values.
    private final Map<AddressField, String> fields = new HashMap<AddressField, String>();
    // The address lines, not normalized.
    private final List<String> addressLines = new ArrayList<String>();
    // The BCP-47 language of the address.
    private String language = null;

    /**
     * Constructs an empty builder for AddressData instances. Prefer to use one of the
     * {@link AddressData#builder} methods in preference to this.
     */
    // TODO: Migrate users and make this private.
    public Builder() {}

    /**
     * Constructs a builder for AddressData instances using data from the given address.
     * Prefer to use one of the {@link AddressData#builder} methods in preference to this.
     *
     * @deprecated Use the builder() methods on AddressData in preference to this.
     */
    @Deprecated
    // TODO: Migrate users and delete this method.
    public Builder(AddressData address) {
      set(address);
    }

    /**
     * Sets the 2-letter CLDR region code of the address; see
     * {@link AddressData#getPostalCountry()}. Unlike other values passed to the builder, the
     * region code can never be null.
     *
     * @param regionCode the CLDR region code of the address.
     */
    // TODO: Rename to setRegionCode.
    public Builder setCountry(String regionCode) {
      return set(AddressField.COUNTRY, checkNotNull(regionCode));
    }

    /**
     * Sets or clears the administrative area of the address; see
     * {@link AddressData#getAdministrativeArea()}.
     *
     * @param adminAreaName the administrative area name, or null to clear an existing value.
     */
    // TODO: Rename to setAdministrativeArea.
    public Builder setAdminArea(String adminAreaName) {
      return set(AddressField.ADMIN_AREA, adminAreaName);
    }

    /**
     * Sets or clears the locality of the address; see {@link AddressData#getLocality()}.
     *
     * @param locality the locality name, or null to clear an existing value.
     */
    public Builder setLocality(String locality) {
      return set(AddressField.LOCALITY, locality);
    }

    /**
     * Sets or clears the dependent locality of the address; see
     * {@link AddressData#getDependentLocality()}.
     *
     * @param dependentLocality the dependent locality name, or null to clear an existing value.
     */
    public Builder setDependentLocality(String dependentLocality) {
      return set(AddressField.DEPENDENT_LOCALITY, dependentLocality);
    }

    /**
     * Sets or clears the postal code of the address; see {@link AddressData#getPostalCode()}.
     *
     * @param postalCode the postal code, or null to clear an existing value.
     */
    public Builder setPostalCode(String postalCode) {
      return set(AddressField.POSTAL_CODE, postalCode);
    }

    /**
     * Sets or clears the sorting code of the address; see {@link AddressData#getSortingCode()}.
     *
     * @param sortingCode the sorting code, or null to clear an existing value.
     */
    public Builder setSortingCode(String sortingCode) {
      return set(AddressField.SORTING_CODE, sortingCode);
    }

    /**
     * Sets or clears the BCP-47 language code for this address (eg, "en" or "zh-Hant"). If the
     * language is not set, then the address will be assumed to be in the default language of the
     * country of the address; however it is highly discouraged to rely on this as the default
     * language may change over time. See {@link AddressData#getLanguageCode()}.
     *
     * @param languageCode the BCP-47 language code, or null to clear an existing value.
     */
    public Builder setLanguageCode(String languageCode) {
      this.language = languageCode;
      return this;
    }

    /**
     * Sets multiple unstructured street level lines in the address. Calling this method will
     * always discard any existing address lines before adding new ones.
     * <p>
     * Note that the number of lines set by this method is preserved in the builder's state but a
     * single line set here may result in multiple lines in the resulting {@code AddressData}
     * instance if it contains embedded newline characters.
     * <p>
     * For example:
     * <pre>{@code
     *   data = AddressData.builder()
     *       .setAddressLines(Arrays.asList("First line\nSecond line"))
     *       .setAddressLine2("Last line");
     *       .build();
     *   // data.getAddressLines() == [ "First line", "Second line", "Last line" ]
     * }</pre>
     */
    public Builder setAddressLines(Iterable<String> lines) {
      addressLines.clear();
      for (String line : lines) {
        addressLines.add(line);
      }
      return this;
    }

    /**
     * Adds another address line. Embedded newlines will be normalized when "build()" is called.
     */
    // TODO: Consider removing this method if nobody is using it to simplify the API.
    public Builder addAddressLine(String value) {
      addressLines.add(value);
      return this;
    }

    /**
     * Sets multiple street lines from a single street string, clearing any existing address lines
     * first. The input string may contain new lines which will result in multiple separate lines
     * in the resulting {@code AddressData} instance. After splitting, each line is trimmed and
     * empty lines are ignored.
     * <p>
     * Example: {@code "  \n   \n1600 Amphitheatre Ave\n\nRoom 122"} will set the lines:
     * <ol>
     * <li>"1600 Amphitheatre Ave"
     * <li>"Room 122"
     * </ol>
     *
     * @param value a string containing one or more address lines, separated by {@code "\n"}.
     */
    public Builder setAddress(String value) {
      addressLines.clear();
      addressLines.add(value);
      normalizeAddressLines(addressLines);
      return this;
    }

    /**
     * Copies all the data of the given address into the builder. Any existing data in the builder
     * is discarded.
     */
    public Builder set(AddressData data) {
      fields.clear();
      for (AddressField addressField : SINGLE_VALUE_FIELDS) {
        set(addressField, data.getFieldValue(addressField));
      }
      addressLines.clear();
      addressLines.addAll(data.addressLines);
      setLanguageCode(data.getLanguageCode());
      return this;
    }

    /**
     * TODO: Remove this method in favor of setAddressLines(Iterable<String>).
     *
     * @deprecated Use {@link #setAddressLines} instead.
     */
    @Deprecated
    public Builder setAddressLine1(String value) {
      return setAddressLine(1, value);
    }

    /**
     * TODO: Remove this method in favor of setAddressLines(Iterable<String>).
     *
     * @deprecated Use {@link #setAddressLines} instead.
     */
    @Deprecated
    public Builder setAddressLine2(String value) {
      return setAddressLine(2, value);
    }

    /**
     * Sets or clears the organization of the address; see {@link AddressData#getOrganization()}.
     *
     * @param organization the organization, or null to clear an existing value.
     */
    public Builder setOrganization(String organization) {
      return set(AddressField.ORGANIZATION, organization);
    }

    /**
     * Sets or clears the recipient of the address; see {@link AddressData#getRecipient()}.
     *
     * @param recipient the recipient, or null to clear an existing value.
     */
    public Builder setRecipient(String recipient) {
      return set(AddressField.RECIPIENT, recipient);
    }

    /**
     * Sets an address field with the specified value. If the value is empty (null or whitespace),
     * the original value associated with the field will be removed.
     *
     * @deprecated Do not use; scheduled for removal from the public API.
     */
    @Deprecated
    @SuppressWarnings("deprecation")
    // TODO: Reimplement using public API as a utility function in AddressWidget (the only caller).
    public Builder set(AddressField field, String value) {
      if (SINGLE_VALUE_FIELDS.contains(field)) {
        value = Util.trimToNull(value);
        if (value == null) {
          fields.remove(field);
        } else {
          fields.put(field, value);
        }
      } else if (field == AddressField.STREET_ADDRESS) {
        if (value == null) {
          addressLines.clear();
        } else {
          setAddress(value);
        }
      } else {
        int lineNum = ADDRESS_LINE_FIELDS.indexOf(field) + 1;
        if (lineNum > 0) {
          setAddressLine(lineNum, value);
        }
      }
      return this;
    }

    // This may preserve whitespace at the ends of lines, but this gets normalized when we build
    // the data instance.
    private Builder setAddressLine(int lineNum, String value) {
      if (Util.trimToNull(value) == null) {
        if (lineNum < addressLines.size()) {
          // Clearing an element that isn't the last in the list.
          addressLines.set(lineNum - 1, null);
        } else if (lineNum == addressLines.size()) {
          // Clearing the last element (remove it and clear up trailing nulls).
          addressLines.remove(lineNum - 1);
          for (int i = addressLines.size() - 1; i >= 0 && addressLines.get(i) == null; i--) {
            addressLines.remove(i);
          }
        }
      } else {
        // Padding the list with nulls if necessary.
        for (int i = addressLines.size(); i < lineNum; i++) {
          addressLines.add(null);
        }
        // Set the non-null value.
        addressLines.set(lineNum - 1, value);
      }
      return this;
    }

    /**
     * Builds an AddressData instance from the current state of the builder. A builder instance may
     * be used to build multiple data instances.
     * <p>
     * During building the street address line information is normalized and the following will be
     * true for any build instance.
     * <ol>
     * <li>The order of address lines is retained relative to the builder.
     * <li>Empty address lines (empty strings, whitespace only or null) are removed.
     * <li>Remaining address lines are trimmed of whitespace.
     * </ol>
     */
    public AddressData build() {
      return new AddressData(this);
    }
  }
}
