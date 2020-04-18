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

import com.google.i18n.addressinput.common.LookupKey.KeyType;
import com.google.i18n.addressinput.common.LookupKey.ScriptType;

import java.util.ArrayList;
import java.util.LinkedList;
import java.util.List;
import java.util.Queue;

/**
 * Responsible for looking up data for address fields. This fetches possible
 * values for the next level down in the address hierarchy, if these are known.
 */
public final class FormController {
  // For address hierarchy in lookup key.
  private static final String SLASH_DELIM = "/";
  // For joined values.
  private static final String TILDE_DELIM = "~";
  // For language code info in lookup key (E.g., data/CA--fr).
  private static final String DASH_DELIM = "--";
  private static final LookupKey ROOT_KEY = FormController.getDataKeyForRoot();
  private static final String DEFAULT_REGION_CODE = "ZZ";
  private static final AddressField[] ADDRESS_HIERARCHY = {
      AddressField.COUNTRY,
      AddressField.ADMIN_AREA,
      AddressField.LOCALITY,
      AddressField.DEPENDENT_LOCALITY};

  // Current user language.
  private String languageCode;
  private final ClientData integratedData;
  private String currentCountry;

  /**
   * Constructor that populates this with data. languageCode should be a BCP language code (such
   * as "en" or "zh-Hant") and currentCountry should be an ISO 2-letter region code (such as "GB"
   * or "US").
   */
  public FormController(ClientData integratedData, String languageCode, String currentCountry) {
    Util.checkNotNull(integratedData, "null data not allowed");
    this.languageCode = languageCode;
    this.currentCountry = currentCountry;

    AddressData address = new AddressData.Builder().setCountry(DEFAULT_REGION_CODE).build();
    LookupKey defaultCountryKey = getDataKeyFor(address);

    AddressVerificationNodeData defaultCountryData =
        integratedData.getDefaultData(defaultCountryKey.toString());
    Util.checkNotNull(defaultCountryData,
        "require data for default country key: " + defaultCountryKey);
    this.integratedData = integratedData;
  }

  public void setLanguageCode(String languageCode) {
    this.languageCode = languageCode;
  }

  public void setCurrentCountry(String currentCountry) {
    this.currentCountry = currentCountry;
  }

  private ScriptType getScriptType() {
    if (languageCode != null && Util.isExplicitLatinScript(languageCode)) {
      return ScriptType.LATIN;
    }
    return ScriptType.LOCAL;
  }

  private static LookupKey getDataKeyForRoot() {
    AddressData address = new AddressData.Builder().build();
    return new LookupKey.Builder(KeyType.DATA).setAddressData(address).build();
  }

  public LookupKey getDataKeyFor(AddressData address) {
    return new LookupKey.Builder(KeyType.DATA).setAddressData(address).build();
  }

  /**
   * Requests data for the input address. This method chains multiple requests together. For
   * example, an address for Mt View, California needs data from "data/US", "data/US/CA", and
   * "data/US/CA/Mt View" to support it. This method will request them one by one (from top level
   * key down to the most granular) and evokes {@link DataLoadListener#dataLoadingEnd} method when
   * all data is collected. If the address is invalid, it will request the first valid child key
   * instead. For example, a request for "data/US/Foo" will end up requesting data for "data/US",
   * "data/US/AL".
   *
   * @param address  the current address.
   * @param listener triggered when requested data for the address is returned.
   */
  public void requestDataForAddress(AddressData address, DataLoadListener listener) {
    Util.checkNotNull(address.getPostalCountry(), "null country not allowed");

    // Gets the key for deepest available node.
    Queue<String> subkeys = new LinkedList<String>();

    for (AddressField field : ADDRESS_HIERARCHY) {
      String value = address.getFieldValue(field);
      if (value == null) {
        break;
      }
      subkeys.add(value);
    }
    if (subkeys.size() == 0) {
      throw new RuntimeException("Need at least country level info");
    }

    if (listener != null) {
      listener.dataLoadingBegin();
    }
    requestDataRecursively(ROOT_KEY, subkeys, listener);
  }

  private void requestDataRecursively(
      final LookupKey key, final Queue<String> subkeys, final DataLoadListener listener) {
    Util.checkNotNull(key, "Null key not allowed");
    Util.checkNotNull(subkeys, "Null subkeys not allowed");

    integratedData.requestData(key, new DataLoadListener() {
      @Override
      public void dataLoadingBegin() {
      }

      @Override
      public void dataLoadingEnd() {
        List<RegionData> subregions = getRegionData(key);
        if (subregions.isEmpty()) {
          if (listener != null) {
            listener.dataLoadingEnd();
          }
          // TODO: Should update the selectors here.
          return;
        } else if (subkeys.size() > 0) {
          String subkey = subkeys.remove();
          for (RegionData subregion : subregions) {
            if (subregion.isValidName(subkey)) {
              LookupKey nextKey = buildDataLookupKey(key, subregion.getKey());
              requestDataRecursively(nextKey, subkeys, listener);
              return;
            }
          }
        }

        // Current value in the field is not valid, use the first valid subkey
        // to request more data instead.
        String firstSubkey = subregions.get(0).getKey();
        LookupKey nextKey = buildDataLookupKey(key, firstSubkey);
        Queue<String> emptyList = new LinkedList<String>();
        requestDataRecursively(nextKey, emptyList, listener);
      }
    });
  }

  private LookupKey buildDataLookupKey(LookupKey lookupKey, String subKey) {
    String[] subKeys = lookupKey.toString().split(SLASH_DELIM);
    String languageCodeSubTag =
        (languageCode == null) ? null : Util.getLanguageSubtag(languageCode);
    String key = lookupKey.toString() + SLASH_DELIM + subKey;

    // Country level key
    if (subKeys.length == 1 && languageCodeSubTag != null
        && !isDefaultLanguage(languageCodeSubTag)) {
      key += DASH_DELIM + languageCodeSubTag.toString();
    }
    return new LookupKey.Builder(key).build();
  }

  /**
   * Compares the language subtags of input {@code languageCode} and default language code. For
   * example, "zh-Hant" and "zh" are viewed as identical.
   */
  public boolean isDefaultLanguage(String languageCode) {
    if (languageCode == null) {
      return true;
    }
    AddressData addr = new AddressData.Builder().setCountry(currentCountry).build();
    LookupKey lookupKey = getDataKeyFor(addr);
    AddressVerificationNodeData data = integratedData.getDefaultData(lookupKey.toString());
    String defaultLanguage = data.get(AddressDataKey.LANG);

    // Current language is not the default language for the country.
    if (Util.trimToNull(defaultLanguage) != null
        && !Util.getLanguageSubtag(languageCode).equals(Util.getLanguageSubtag(languageCode))) {
      return false;
    }
    return true;
  }

  /**
   * Gets a list of {@link RegionData} for sub-regions for a given key. For example, sub regions
   * for "data/US" are AL/Alabama, AK/Alaska, etc.
   *
   * <p> TODO: Rename/simplify RegionData to avoid confusion with RegionDataConstants elsewhere
   * since it does not contain anything more than key/value pairs now.
   *
   * @return A list of sub-regions, each sub-region represented by a {@link RegionData}.
   */
  public List<RegionData> getRegionData(LookupKey key) {
    if (key.getKeyType() == KeyType.EXAMPLES) {
      throw new RuntimeException("example key not allowed for getting region data");
    }
    Util.checkNotNull(key, "null regionKey not allowed");
    LookupKey normalizedKey = normalizeLookupKey(key);
    List<RegionData> results = new ArrayList<RegionData>();

    // Root key.
    if (normalizedKey.equals(ROOT_KEY)) {
      AddressVerificationNodeData data = integratedData.getDefaultData(normalizedKey.toString());
      String[] countries = splitData(data.get(AddressDataKey.COUNTRIES));
      for (int i = 0; i < countries.length; i++) {
        RegionData rd = new RegionData.Builder().setKey(countries[i]).setName(countries[i]).build();
        results.add(rd);
      }
      return results;
    }

    AddressVerificationNodeData data = integratedData.get(normalizedKey.toString());
    if (data != null) {
      String[] keys = splitData(data.get(AddressDataKey.SUB_KEYS));
      String[] names = (getScriptType() == ScriptType.LOCAL)
          ? splitData(data.get(AddressDataKey.SUB_NAMES))
          : splitData(data.get(AddressDataKey.SUB_LNAMES));

      for (int i = 0; i < keys.length; i++) {
        RegionData rd = new RegionData.Builder()
            .setKey(keys[i])
            .setName((i < names.length) ? names[i] : keys[i])
            .build();
        results.add(rd);
      }
    }
    return results;
  }

  /**
   * Split a '~' delimited string into an array of strings. This method is null tolerant and
   * considers an empty string to contain no elements.
   *
   * @param raw The data to split
   * @return an array of strings
   */
  private String[] splitData(String raw) {
    if (raw == null || raw.isEmpty()) {
      return new String[]{};
    }
    return raw.split(TILDE_DELIM);
  }

  private String getSubKey(LookupKey parentKey, String name) {
    for (RegionData subRegion : getRegionData(parentKey)) {
      if (subRegion.isValidName(name)) {
        return subRegion.getKey();
      }
    }
    return null;
  }

  /**
   * Normalizes {@code key} by replacing field values with sub-keys. For example, California is
   * replaced with CA. The normalization goes from top (country) to bottom (dependent locality)
   * and if any field value is empty, unknown, or invalid, it will stop and return whatever it
   * gets. For example, a key "data/US/California/foobar/kar" will be normalized into
   * "data/US/CA/foobar/kar" since "foobar" is unknown. This method supports only keys of
   * {@link KeyType#DATA} type.
   *
   * @return normalized {@link LookupKey}.
   */
  private LookupKey normalizeLookupKey(LookupKey key) {
    Util.checkNotNull(key);
    if (key.getKeyType() != KeyType.DATA) {
      throw new RuntimeException("Only DATA keyType is supported");
    }

    String subStr[] = key.toString().split(SLASH_DELIM);

    // Root key does not need to be normalized.
    if (subStr.length < 2) {
      return key;
    }

    StringBuilder sb = new StringBuilder(subStr[0]);
    for (int i = 1; i < subStr.length; ++i) {
      // Strips the language code if there was one.
      String languageCode = null;
      if (i == 1 && subStr[i].contains(DASH_DELIM)) {
        String[] s = subStr[i].split(DASH_DELIM);
        subStr[i] = s[0];
        languageCode = s[1];
      }

      String normalizedSubKey = getSubKey(new LookupKey.Builder(sb.toString()).build(), subStr[i]);

      // Can't find normalized sub-key; assembles the lookup key with the
      // remaining sub-keys and returns it.
      if (normalizedSubKey == null) {
        for (; i < subStr.length; ++i) {
          sb.append(SLASH_DELIM).append(subStr[i]);
        }
        break;
      }
      sb.append(SLASH_DELIM).append(normalizedSubKey);
      if (languageCode != null) {
        sb.append(DASH_DELIM).append(languageCode);
      }
    }
    return new LookupKey.Builder(sb.toString()).build();
  }
}
