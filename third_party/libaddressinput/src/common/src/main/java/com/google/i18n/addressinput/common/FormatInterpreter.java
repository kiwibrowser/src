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

import com.google.i18n.addressinput.common.AddressField.WidthType;
import com.google.i18n.addressinput.common.LookupKey.ScriptType;

import org.json.JSONException;
import org.json.JSONObject;
import org.json.JSONTokener;

import java.util.ArrayList;
import java.util.Collections;
import java.util.EnumSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

/**
 * Address format interpreter. A utility to find address format related info.
 */
public final class FormatInterpreter {
  private static final String NEW_LINE = "%n";

  private final FormOptions.Snapshot formOptions;

  /**
   * Creates a new instance of {@link FormatInterpreter}.
   */
  public FormatInterpreter(FormOptions.Snapshot options) {
    Util.checkNotNull(
        RegionDataConstants.getCountryFormatMap(), "null country name map not allowed");
    Util.checkNotNull(options);
    this.formOptions = options;
    Util.checkNotNull(getJsonValue("ZZ", AddressDataKey.FMT),
        "Could not obtain a default address field order.");
  }

  /**
   * Returns a list of address fields based on the format of {@code regionCode}. Script type is
   * needed because some countries uses different address formats for local/Latin scripts.
   *
   * @param scriptType if {@link ScriptType#LOCAL}, use local format; else use Latin format.
   */
  // TODO: Consider not re-doing this work every time the widget is re-rendered.
  @SuppressWarnings("deprecation")  // For legacy address fields.
  public List<AddressField> getAddressFieldOrder(ScriptType scriptType, String regionCode) {
    Util.checkNotNull(scriptType);
    Util.checkNotNull(regionCode);
    String formatString = getFormatString(scriptType, regionCode);
    return getAddressFieldOrder(formatString, regionCode);
  }

  List<AddressField> getAddressFieldOrder(String formatString, String regionCode) {
    EnumSet<AddressField> visibleFields = EnumSet.noneOf(AddressField.class);
    List<AddressField> fieldOrder = new ArrayList<AddressField>();
    // TODO: Change this to just enumerate the address fields directly.
    for (String substring : getFormatSubstrings(formatString)) {
      // Skips un-escaped characters and new lines.
      if (!substring.matches("%.") || substring.equals(NEW_LINE)) {
        continue;
      }
      AddressField field = getFieldForFormatSubstring(substring);
      // Accept only the first instance for any duplicate fields (which can occur because the
      // string we start with defines format order, which can contain duplicate fields).
      if (!visibleFields.contains(field)) {
        visibleFields.add(field);
        fieldOrder.add(field);
      }
    }
    applyFieldOrderOverrides(regionCode, fieldOrder);

    // Uses two address lines instead of street address.
    for (int n = 0; n < fieldOrder.size(); n++) {
      if (fieldOrder.get(n) == AddressField.STREET_ADDRESS) {
        fieldOrder.set(n, AddressField.ADDRESS_LINE_1);
        fieldOrder.add(n + 1, AddressField.ADDRESS_LINE_2);
        break;
      }
    }
    return Collections.unmodifiableList(fieldOrder);
  }

  /**
   * Returns true if this format substring (e.g. %C) represents an address field. Returns false if
   * it is a literal or newline.
   */
  private static boolean formatSubstringRepresentsField(String formatSubstring) {
    return !formatSubstring.equals(NEW_LINE) && formatSubstring.startsWith("%");
  }

  /**
   * Gets data from the address represented by a format substring such as %C. Will throw an
   * exception if no field can be found.
   */
  private static AddressField getFieldForFormatSubstring(String formatSubstring) {
    return AddressField.of(formatSubstring.charAt(1));
  }

  /**
   * Returns true if the address has any data for this address field.
   */
  private static boolean addressHasValueForField(AddressData address, AddressField field) {
    if (field == AddressField.STREET_ADDRESS) {
      return address.getAddressLines().size() > 0;
    } else {
      String value = address.getFieldValue(field);
      return (value != null && !value.isEmpty());
    }
  }

  private void applyFieldOrderOverrides(String regionCode, List<AddressField> fieldOrder) {
    List<AddressField> customFieldOrder = formOptions.getCustomFieldOrder(regionCode);
    if (customFieldOrder == null) {
      return;
    }

    // We can assert that fieldOrder and customFieldOrder contain no duplicates.
    // We know this by the construction above and in FormOptions but we still have to think
    // about fields in the custom ordering which aren't visible (the loop below will fail if
    // a non-visible field appears in the custom ordering). However in that case it's safe to
    // just ignore the extraneous field.
    Set<AddressField> nonVisibleCustomFields = EnumSet.copyOf(customFieldOrder);
    nonVisibleCustomFields.removeAll(fieldOrder);
    if (nonVisibleCustomFields.size() > 0) {
      // Local mutable copy to remove non visible fields - this shouldn't happen often.
      customFieldOrder = new ArrayList<AddressField>(customFieldOrder);
      customFieldOrder.removeAll(nonVisibleCustomFields);
    }
    // It is vital for this loop to work correctly that every element in customFieldOrder
    // appears in fieldOrder exactly once.
    for (int fieldIdx = 0, customIdx = 0; fieldIdx < fieldOrder.size(); fieldIdx++) {
      if (customFieldOrder.contains(fieldOrder.get(fieldIdx))) {
        fieldOrder.set(fieldIdx, customFieldOrder.get(customIdx++));
      }
    }
  }

  /**
   * Returns the fields that are required to be filled in for this country. This is based upon the
   * "required" field in RegionDataConstants for {@code regionCode}, and handles falling back to
   * the default data if necessary.
   */
  static Set<AddressField> getRequiredFields(String regionCode) {
    Util.checkNotNull(regionCode);
    String requireString = getRequiredString(regionCode);
    return getRequiredFields(requireString, regionCode);
  }

  static Set<AddressField> getRequiredFields(String requireString, String regionCode) {
    EnumSet<AddressField> required = EnumSet.of(AddressField.COUNTRY);
    for (char c : requireString.toCharArray()) {
      required.add(AddressField.of(c));
    }
    return required;
  }

  private static String getRequiredString(String regionCode) {
    String required = getJsonValue(regionCode, AddressDataKey.REQUIRE);
    if (required == null) {
      required = getJsonValue("ZZ", AddressDataKey.REQUIRE);
    }
    return required;
  }

  /**
   * Returns the field width override for the specified country, or null if there's none. This is
   * based upon the "width_overrides" field in RegionDataConstants for {@code regionCode}.
   */
  static WidthType getWidthOverride(AddressField field, String regionCode) {
    return getWidthOverride(field, regionCode, RegionDataConstants.getCountryFormatMap());
  }

  /**
   * Visible for Testing - same as {@link #getWidthOverride(AddressField, String)} but testable with
   * fake data.
   */
  static WidthType getWidthOverride(
      AddressField field, String regionCode, Map<String, String> regionDataMap) {
    Util.checkNotNull(regionCode);
    String overridesString =
        getJsonValue(regionCode, AddressDataKey.WIDTH_OVERRIDES, regionDataMap);
    if (overridesString == null || overridesString.isEmpty()) {
      return null;
    }

    // The field width overrides string starts with a %, so we skip the first one.
    // Example string: "%C:L%S:S" which is a repeated string of
    // '<%> field_character <:> width_character'.
    for (int pos = 0; pos != -1;) {
      int keyStartIndex = pos + 1;
      int valueStartIndex = overridesString.indexOf(':', keyStartIndex + 1) + 1;
      if (valueStartIndex == 0 || valueStartIndex == overridesString.length()) {
        // Malformed string -- % not followed by ':' or trailing ':'
        return null;
      }
      // Prepare for next iteration.
      pos = overridesString.indexOf('%', valueStartIndex + 1);
      if (valueStartIndex != keyStartIndex + 2 ||
          overridesString.charAt(keyStartIndex) != field.getChar()) {
        // Key is not a high level field (unhandled by this code) or does not match.
        // Also catches malformed string where key is of zero length (skip, not error).
        continue;
      }
      int valueLength = (pos != -1 ? pos : overridesString.length()) - valueStartIndex;
      if (valueLength != 1) {
        // Malformed string -- value has length other than 1
        return null;
      }
      return WidthType.of(overridesString.charAt(valueStartIndex));
    }

    return null;
  }

  /**
   * Gets formatted address. For example,
   *
   * <p> John Doe</br>
   * Dnar Corp</br>
   * 5th St</br>
   * Santa Monica CA 90123 </p>
   *
   * This method does not validate addresses. Also, it will "normalize" the result strings by
   * removing redundant spaces and empty lines.
   */
  public List<String> getEnvelopeAddress(AddressData address) {
    Util.checkNotNull(address, "null input address not allowed");
    String regionCode = address.getPostalCountry();

    String lc = address.getLanguageCode();
    ScriptType scriptType = ScriptType.LOCAL;
    if (lc != null) {
      scriptType = Util.isExplicitLatinScript(lc) ? ScriptType.LATIN : ScriptType.LOCAL;
    }

    List<String> prunedFormat = new ArrayList<String>();
    String formatString = getFormatString(scriptType, regionCode);
    List<String> formatSubstrings = getFormatSubstrings(formatString);
    for (int i = 0; i < formatSubstrings.size(); i++) {
      String formatSubstring = formatSubstrings.get(i);
      // Always keep the newlines.
      if (formatSubstring.equals(NEW_LINE)) {
        prunedFormat.add(NEW_LINE);
      } else if (formatSubstringRepresentsField(formatSubstring)) {
        // Always keep the non-empty address fields.
        if (addressHasValueForField(address, getFieldForFormatSubstring(formatSubstring))) {
          prunedFormat.add(formatSubstring);
        }
      } else if (
          // Only keep literals that satisfy these 2 conditions:
          // (1) Not preceding an empty field.
          (i == formatSubstrings.size() - 1 || formatSubstrings.get(i + 1).equals(NEW_LINE)
           || addressHasValueForField(address, getFieldForFormatSubstring(
               formatSubstrings.get(i + 1))))
          // (2) Not following a removed field.
          && (i == 0 || !formatSubstringRepresentsField(formatSubstrings.get(i - 1))
              || (!prunedFormat.isEmpty()
                  && formatSubstringRepresentsField(prunedFormat.get(prunedFormat.size() - 1))))) {
        prunedFormat.add(formatSubstring);
      }
    }

    List<String> lines = new ArrayList<>();
    StringBuilder currentLine = new StringBuilder();
    for (String formatSubstring : prunedFormat) {
      if (formatSubstring.equals(NEW_LINE)) {
        if (currentLine.length() > 0) {
          lines.add(currentLine.toString());
          currentLine.setLength(0);
        }
      } else if (formatSubstringRepresentsField(formatSubstring)) {
        switch (getFieldForFormatSubstring(formatSubstring)) {
          case STREET_ADDRESS:
            // The field "street address" represents the street address lines of an address, so
            // there can be multiple values.
            List<String> addressLines = address.getAddressLines();
            if (addressLines.size() > 0) {
              currentLine.append(addressLines.get(0));
              if (addressLines.size() > 1) {
                lines.add(currentLine.toString());
                currentLine.setLength(0);
                lines.addAll(addressLines.subList(1, addressLines.size()));
              }
            }
            break;
          case COUNTRY:
            // Country name is treated separately.
            break;
          case ADMIN_AREA:
            currentLine.append(address.getAdministrativeArea());
            break;
          case LOCALITY:
            currentLine.append(address.getLocality());
            break;
          case DEPENDENT_LOCALITY:
            currentLine.append(address.getDependentLocality());
            break;
          case RECIPIENT:
            currentLine.append(address.getRecipient());
            break;
          case ORGANIZATION:
            currentLine.append(address.getOrganization());
            break;
          case POSTAL_CODE:
            currentLine.append(address.getPostalCode());
            break;
          case SORTING_CODE:
            currentLine.append(address.getSortingCode());
            break;
          default:
            break;
        }
      } else {
        // Not a symbol we recognise, so must be a literal. We append it unchanged.
        currentLine.append(formatSubstring);
      }
    }
    if (currentLine.length() > 0) {
      lines.add(currentLine.toString());
    }
    return lines;
  }

  /**
   * Tokenizes the format string and returns the token string list. "%" is treated as an escape
   * character. For example, "%n%a%nxyz" will be split into "%n", "%a", "%n", "xyz".
   * Escaped tokens correspond to either new line or address fields. The output of this method
   * may contain duplicates.
   */
  // TODO: Create a common method which does field parsing in one place (there are about 4 other
  // places in this library where format strings are parsed).
  private List<String> getFormatSubstrings(String formatString) {
    List<String> parts = new ArrayList<String>();

    boolean escaped = false;
    StringBuilder currentLiteral = new StringBuilder();
    for (char c : formatString.toCharArray()) {
      if (escaped) {
        escaped = false;
        parts.add("%" + c);
      } else if (c == '%') {
        if (currentLiteral.length() > 0) {
          parts.add(currentLiteral.toString());
          currentLiteral.setLength(0);
        }
        escaped = true;
      } else {
        currentLiteral.append(c);
      }
    }
    if (currentLiteral.length() > 0) {
      parts.add(currentLiteral.toString());
    }
    return parts;
  }

  private static String getFormatString(ScriptType scriptType, String regionCode) {
    String format = (scriptType == ScriptType.LOCAL)
        ? getJsonValue(regionCode, AddressDataKey.FMT)
        : getJsonValue(regionCode, AddressDataKey.LFMT);
    if (format == null) {
      format = getJsonValue("ZZ", AddressDataKey.FMT);
    }
    return format;
  }

  private static String getJsonValue(String regionCode, AddressDataKey key) {
    return getJsonValue(regionCode, key, RegionDataConstants.getCountryFormatMap());
  }

  /**
   * Visible for testing only.
   */
  static String getJsonValue(
      String regionCode, AddressDataKey key, Map<String, String> regionDataMap) {
    Util.checkNotNull(regionCode);
    String jsonString = regionDataMap.get(regionCode);
    Util.checkNotNull(jsonString, "no json data for region code " + regionCode);

    try {
      JSONObject jsonObj = new JSONObject(new JSONTokener(jsonString));
      if (!jsonObj.has(Util.toLowerCaseLocaleIndependent(key.name()))) {
        // Key not found. Return null.
        return null;
      }
      // Gets the string for this key.
      String parsedJsonString = jsonObj.getString(Util.toLowerCaseLocaleIndependent(key.name()));
      return parsedJsonString;
    } catch (JSONException e) {
      throw new RuntimeException("Invalid json for region code " + regionCode + ": " + jsonString);
    }
  }
}
