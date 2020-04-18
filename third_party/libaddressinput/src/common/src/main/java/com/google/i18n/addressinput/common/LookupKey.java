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
import java.util.EnumMap;
import java.util.Map;

/**
 * A builder for creating keys that are used to lookup data in the local cache and fetch data from
 * the server. There are two key types: {@link KeyType#DATA} or {@link KeyType#EXAMPLES}.
 * <p>
 * The {@link KeyType#DATA} key is built based on a universal Address hierarchy, which is:<br>
 * {@link AddressField#COUNTRY} -> {@link AddressField#ADMIN_AREA} -> {@link AddressField#LOCALITY}
 * -> {@link AddressField#DEPENDENT_LOCALITY}
 * <p>
 * The {@link KeyType#EXAMPLES} key is built with the following format:<br>
 * {@link AddressField#COUNTRY} -> {@link ScriptType} -> language. </p>
 */
public final class LookupKey {
  /**
   * Key types. Address Widget organizes address info based on key types. For example, if you want
   * to know how to verify or format an US address, you need to use {@link KeyType#DATA} to get
   * that info; if you want to get an example address, you use {@link KeyType#EXAMPLES} instead.
   */
  public enum KeyType {
    /**
     * Key type for getting address data.
     */
    DATA,
    /**
     * Key type for getting examples.
     */
    EXAMPLES
  }

  /**
   * Script types. This is used for countries that do not use Latin script, but accept it for
   * transcribing their addresses. For example, you can write a Japanese address in Latin script
   * instead of Japanese:
   * <pre>7-2, Marunouchi 2-Chome, Chiyoda-ku, Tokyo 100-8799 </pre>
   * <p>
   * Notice that {@link ScriptType} is based on country/region, not language.
   */
  public enum ScriptType {
    /**
     * The script that uses Roman characters like ABC (as opposed to scripts like Cyrillic or
     * Arabic).
     */
    LATIN,

    /**
     * Local scripts. For Japan, it's Japanese (including Hiragana, Katagana, and Kanji); For
     * Saudi Arabia, it's Arabic. Notice that for US, the local script is actually Latin script
     * (The same goes for other countries that use Latin script). For these countries, we do not
     * provide two set of data (Latin and local) since they use only Latin script. You have to
     * specify the {@link ScriptType} as local instead Latin.
     */
    LOCAL
  }

  /**
   * The universal address hierarchy. Notice that sub-administrative area is neglected here since
   * it is not required to fill out address forms.
   */
  private static final AddressField[] HIERARCHY = {
    AddressField.COUNTRY,
    AddressField.ADMIN_AREA,
    AddressField.LOCALITY,
    AddressField.DEPENDENT_LOCALITY};

  private static final String SLASH_DELIM = "/";

  private static final String DASH_DELIM = "--";

  private static final String DEFAULT_LANGUAGE = "_default";

  private final KeyType keyType;

  private final ScriptType scriptType;

  // Values for each address field in the hierarchy.
  private final Map<AddressField, String> nodes;

  private final String keyString;

  private final String languageCode;

  private LookupKey(Builder builder) {
    this.keyType = builder.keyType;
    this.scriptType = builder.script;
    this.nodes = builder.nodes;
    this.languageCode = builder.languageCode;
    this.keyString = createKeyString();
  }

  /**
   * Gets a lookup key built from the values of nodes in the hierarchy up to and including the input
   * address field. This method does not allow keys with a key type of {@link KeyType#EXAMPLES}.
   *
   * @param field a field in the address hierarchy.
   * @return key of the specified address field. If address field is not in the hierarchy, or is
   *         more granular than the data present in the current key, returns null. For example,
   *         if your current key is "data/US" (down to COUNTRY level), and you want to get the key
   *         for LOCALITY (more granular than COUNTRY), it will return null.
   */
  public LookupKey getKeyForUpperLevelField(AddressField field) {
    if (keyType != KeyType.DATA) {
      // We only support getting the parent key for the data key type.
      throw new RuntimeException("Only support getting parent keys for the data key type.");
    }
    Builder newKeyBuilder = new Builder(this);

    boolean removeNode = false;
    boolean fieldInHierarchy = false;
    for (AddressField hierarchyField : HIERARCHY) {
      if (removeNode) {
        if (newKeyBuilder.nodes.containsKey(hierarchyField)) {
          newKeyBuilder.nodes.remove(hierarchyField);
        }
      }
      if (hierarchyField == field) {
        if (!newKeyBuilder.nodes.containsKey(hierarchyField)) {
          return null;
        }
        removeNode = true;
        fieldInHierarchy = true;
      }
    }

    if (!fieldInHierarchy) {
      return null;
    }

    newKeyBuilder.languageCode = languageCode;
    newKeyBuilder.script = scriptType;

    return newKeyBuilder.build();
  }

  /**
   * Returns the string value of a field in a key for a particular
   * AddressField. For example, for the key "data/US/CA" and the address
   * field AddressField.COUNTRY, "US" would be returned. Returns an empty
   * string if the key does not have this field in it.
   */
  String getValueForUpperLevelField(AddressField field) {
    if (!this.nodes.containsKey(field)) {
      return "";
    }

    return this.nodes.get(field);
  }

  /**
   * Gets parent key for data key. For example, parent key for "data/US/CA" is "data/US". This
   * method does not allow key with key type of {@link KeyType#EXAMPLES}.
   */
  LookupKey getParentKey() {
    if (keyType != KeyType.DATA) {
      throw new RuntimeException("Only support getting parent keys for the data key type.");
    }
    // Root key's parent should be null.
    if (!nodes.containsKey(AddressField.COUNTRY)) {
      return null;
    }

    Builder parentKeyBuilder = new Builder(this);
    AddressField mostGranularField = AddressField.COUNTRY;

    for (AddressField hierarchyField : HIERARCHY) {
      if (!nodes.containsKey(hierarchyField)) {
        break;
      }
      mostGranularField = hierarchyField;
    }
    parentKeyBuilder.nodes.remove(mostGranularField);
    return parentKeyBuilder.build();
  }

  public KeyType getKeyType() {
    return keyType;
  }

  /**
   * Creates the string format of the given key. E.g., "data/US/CA".
   */
  private String createKeyString() {
    StringBuilder keyBuilder = new StringBuilder(Util.toLowerCaseLocaleIndependent(keyType.name()));

    if (keyType == KeyType.DATA) {
      for (AddressField field : HIERARCHY) {
        if (!nodes.containsKey(field)) {
          break;
        }
        keyBuilder.append(SLASH_DELIM).append(nodes.get(field));
      }
      // Only append the language if this is not the root key and there was a language.
      if (languageCode != null && nodes.size() > 0) {
        keyBuilder.append(DASH_DELIM).append(languageCode);
      }
    } else {
      if (nodes.containsKey(AddressField.COUNTRY)) {
        // Example key. E.g., "examples/TW/local/_default".
        keyBuilder.append(SLASH_DELIM)
            .append(nodes.get(AddressField.COUNTRY))
            .append(SLASH_DELIM)
            .append(Util.toLowerCaseLocaleIndependent(scriptType.name()))
            .append(SLASH_DELIM)
            .append(DEFAULT_LANGUAGE);
      }
    }

    return keyBuilder.toString();
  }

  /**
   * Gets a lookup key as a plain text string., e.g., "data/US/CA".
   */
  @Override
  public String toString() {
    return keyString;
  }

  @Override
  public boolean equals(Object obj) {
    if (this == obj) {
      return true;
    }
    if ((obj == null) || (obj.getClass() != this.getClass())) {
      return false;
    }

    return ((LookupKey) obj).toString().equals(keyString);
  }

  @Override
  public int hashCode() {
    return keyString.hashCode();
  }

  static boolean hasValidKeyPrefix(String key) {
    for (KeyType type : KeyType.values()) {
      if (key.startsWith(Util.toLowerCaseLocaleIndependent(type.name()))) {
        return true;
      }
    }
    return false;
  }

  /**
   * Builds lookup keys.
   */
  // TODO: This is used in AddressWidget in a small number of places and it should be possible
  // to hide this type within this package quite easily.
  public static class Builder {
    private KeyType keyType;

    // Default to LOCAL script.
    private ScriptType script = ScriptType.LOCAL;

    private final Map<AddressField, String> nodes =
        new EnumMap<AddressField, String>(AddressField.class);

    private String languageCode;

    /**
     * Creates a new builder for the specified key type. keyType cannot be null.
     */
    public Builder(KeyType keyType) {
      this.keyType = keyType;
    }

    /**
     * Creates a new builder for the specified key. oldKey cannot be null.
     */
    Builder(LookupKey oldKey) {
      this.keyType = oldKey.keyType;
      this.script = oldKey.scriptType;
      this.languageCode = oldKey.languageCode;
      for (AddressField field : HIERARCHY) {
        if (!oldKey.nodes.containsKey(field)) {
          break;
        }
        this.nodes.put(field, oldKey.nodes.get(field));
      }
    }

    /**
     * Builds the {@link LookupKey} with the input key string. Input string has to represent
     * either a {@link KeyType#DATA} key or a {@link KeyType#EXAMPLES} key. Also, key hierarchy
     * deeper than {@link AddressField#DEPENDENT_LOCALITY} is not allowed. Notice that if any
     * node in the hierarchy is empty, all the descendant nodes' values will be neglected. For
     * example, input string "data/US//Mt View" will become "data/US".
     *
     * @param keyString e.g., "data/US/CA"
     */
    public Builder(String keyString) {
      String[] parts = keyString.split(SLASH_DELIM);

      if (!parts[0].equals(Util.toLowerCaseLocaleIndependent(KeyType.DATA.name()))
          && !parts[0].equals(Util.toLowerCaseLocaleIndependent(KeyType.EXAMPLES.name()))) {
        throw new RuntimeException("Wrong key type: " + parts[0]);
      }
      if (parts.length > HIERARCHY.length + 1) {
        // Assume that any extra elements found in the key belong in the 'dependent locality' field.
        // This means that a key string of /EXAMPLES/C/A/L/D/E would result in a dependent locality
        // value of 'D/E'. This also means that if it's the actual locality name has a slash in it
        // (for example 'L/D'), the locality field which we break down will be incorrect
        // (for example: 'L'). Regardless, the actual breakdown of the key doesn't impact the server
        // lookup, so there will be no problems.
        String[] extraParts = Arrays.copyOfRange(parts, HIERARCHY.length + 1, parts.length + 1);

        // Update the original array to only contain the number of elements which we expect.
        parts = Arrays.copyOfRange(parts, 0, HIERARCHY.length + 1);

        // Append the extra parts to the last element (dependent locality).
        for (String element : extraParts) {
          if (element != null) {
            parts[4] += SLASH_DELIM + element;
          }
        }
      }

      if (parts[0].equals("data")) {
        keyType = KeyType.DATA;

        // Process all parts of the key, starting from the country.
        for (int i = 1; i < parts.length; i++) {
          // TODO: We shouldn't need the trimToNull here.
          String substr = Util.trimToNull(parts[i]);
          if (substr == null) {
            break;
          }
          // If a language code specification was present, extract this. This should only be there
          // (if it ever is) on the last node.
          if (substr.contains(DASH_DELIM)) {
            String[] s = substr.split(DASH_DELIM);
            if (s.length != 2) {
              throw new RuntimeException(
                  "Wrong format: Substring should be <last node value>--<language code>");
            }
            substr = s[0];
            languageCode = s[1];
          }

          this.nodes.put(HIERARCHY[i - 1], substr);
        }
      } else if (parts[0].equals("examples")) {
        keyType = KeyType.EXAMPLES;

        // Parses country info.
        if (parts.length > 1) {
          this.nodes.put(AddressField.COUNTRY, parts[1]);
        }

        // Parses script types.
        if (parts.length > 2) {
          String scriptStr = parts[2];
          if (scriptStr.equals("local")) {
            this.script = ScriptType.LOCAL;
          } else if (scriptStr.equals("latin")) {
            this.script = ScriptType.LATIN;
          } else {
            throw new RuntimeException("Script type has to be either latin or local.");
          }
        }

        // Parses language code. Example: "zh_Hant" in
        // "examples/TW/local/zH_Hant".
        if (parts.length > 3 && !parts[3].equals(DEFAULT_LANGUAGE)) {
          languageCode = parts[3];
        }
      }
    }

    Builder setLanguageCode(String languageCode) {
      this.languageCode = languageCode;
      return this;
    }

    /**
     * Sets key using {@link AddressData}. Notice that if any node in the hierarchy is empty,
     * all the descendant nodes' values will be neglected. For example, the following address
     * misses {@link AddressField#ADMIN_AREA}, thus its data key will be "data/US".
     *
     * <p> country: US<br> administrative area: null<br> locality: Mt. View </p>
     */
    public Builder setAddressData(AddressData data) {
      languageCode = data.getLanguageCode();
      if (languageCode != null) {
        if (Util.isExplicitLatinScript(languageCode)) {
          script = ScriptType.LATIN;
        }
      }

      if (data.getPostalCountry() == null) {
        return this;
      }
      this.nodes.put(AddressField.COUNTRY, data.getPostalCountry());

      if (data.getAdministrativeArea() == null) {
        return this;
      }
      this.nodes.put(AddressField.ADMIN_AREA, data.getAdministrativeArea());

      if (data.getLocality() == null) {
        return this;
      }
      this.nodes.put(AddressField.LOCALITY, data.getLocality());

      if (data.getDependentLocality() == null) {
        return this;
      }
      this.nodes.put(AddressField.DEPENDENT_LOCALITY, data.getDependentLocality());
      return this;
    }

    public LookupKey build() {
      return new LookupKey(this);
    }
  }
}
