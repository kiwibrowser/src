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

import org.json.JSONArray;
import org.json.JSONException;

import java.util.EnumMap;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;
import java.util.logging.Logger;

/**
 * Access point for the cached address verification data. The data contained here will mainly be
 * used to build {@link FieldVerifier}'s.
 */
public final class ClientData implements DataSource {
  private static final Logger logger = Logger.getLogger(ClientData.class.getName());

  /**
   * Data to bootstrap the process. The data are all regional (country level) data. Keys are like
   * "data/US/CA"
   */
  private final Map<String, JsoMap> bootstrapMap = new HashMap<String, JsoMap>();

  private CacheData cacheData;

  public ClientData(CacheData cacheData) {
    this.cacheData = cacheData;
    buildRegionalData();
  }

  @Override
  public AddressVerificationNodeData get(String key) {
    JsoMap jso = cacheData.getObj(key);
    if (jso == null) {  // Not cached.
      fetchDataIfNotAvailable(key);
      jso = cacheData.getObj(key);
    }
    if (jso != null && isValidDataKey(key)) {
      return createNodeData(jso);
    }
    return null;
  }

  @Override
  public AddressVerificationNodeData getDefaultData(String key) {
    // root data
    if (key.split("/").length == 1) {
      JsoMap jso = bootstrapMap.get(key);
      if (jso == null || !isValidDataKey(key)) {
        throw new RuntimeException("key " + key + " does not have bootstrap data");
      }
      return createNodeData(jso);
    }

    key = getCountryKey(key);
    JsoMap jso = bootstrapMap.get(key);
    if (jso == null || !isValidDataKey(key)) {
      throw new RuntimeException("key " + key + " does not have bootstrap data");
    }
    return createNodeData(jso);
  }

  private String getCountryKey(String hierarchyKey) {
    if (hierarchyKey.split("/").length <= 1) {
      throw new RuntimeException("Cannot get country key with key '" + hierarchyKey + "'");
    }
    if (isCountryKey(hierarchyKey)) {
      return hierarchyKey;
    }

    String[] parts = hierarchyKey.split("/");
    return parts[0] + "/" + parts[1];
  }

  private boolean isCountryKey(String hierarchyKey) {
    Util.checkNotNull(hierarchyKey, "Cannot use null as a key");
    return hierarchyKey.split("/").length == 2;
  }

  /**
   * Returns the contents of the JSON-format string as a map.
   */
  protected AddressVerificationNodeData createNodeData(JsoMap jso) {
    Map<AddressDataKey, String> map = new EnumMap<AddressDataKey, String>(AddressDataKey.class);

    JSONArray arr = jso.getKeys();
    for (int i = 0; i < arr.length(); i++) {
      try {
        AddressDataKey key = AddressDataKey.get(arr.getString(i));

        if (key == null) {
          // Not all keys are supported by Android, so we continue if we encounter one
          // that is not used.
          continue;
        }

        String value = jso.get(Util.toLowerCaseLocaleIndependent(key.toString()));
        map.put(key, value);
      } catch (JSONException e) {
        // This should not happen - we should not be fetching a key from outside the bounds
        // of the array.
      }
    }

    return new AddressVerificationNodeData(map);
  }

  /**
   * We can be initialized with the full set of address information, but validation only uses info
   * prefixed with "data" (in particular, no info prefixed with "examples").
   */
  private boolean isValidDataKey(String key) {
    return key.startsWith("data");
  }

  /**
   * Initializes regionalData structure based on property file.
   */
  private void buildRegionalData() {
    StringBuilder countries = new StringBuilder();

    for (String countryCode : RegionDataConstants.getCountryFormatMap().keySet()) {
      countries.append(countryCode + "~");
      String json = RegionDataConstants.getCountryFormatMap().get(countryCode);
      JsoMap jso = null;
      try {
        jso = JsoMap.buildJsoMap(json);
      } catch (JSONException e) {
        // Ignore.
      }

      AddressData data = new AddressData.Builder().setCountry(countryCode).build();
      LookupKey key = new LookupKey.Builder(KeyType.DATA).setAddressData(data).build();
      bootstrapMap.put(key.toString(), jso);
    }
    countries.setLength(countries.length() - 1);

    // TODO: this is messy. do we have better ways to do it?
    // Creates verification data for key="data". This will be used for the root FieldVerifier.
    String str = "{\"id\":\"data\",\"countries\": \"" + countries.toString() + "\"}";
    JsoMap jsoData = null;
    try {
      jsoData = JsoMap.buildJsoMap(str);
    } catch (JSONException e) {
      // Ignore.
    }
    bootstrapMap.put("data", jsoData);
  }

  /**
   * Fetches data from remote server if it is not cached yet.
   *
   * @param key The key for data that being requested. Key can be either a data key (starts with
   *     "data") or example key (starts with "examples")
   */
  private void fetchDataIfNotAvailable(String key) {
    JsoMap jso = cacheData.getObj(key);
    if (jso == null) {
      // If there is bootstrap data for the key, pass the data to fetchDynamicData
      JsoMap regionalData = bootstrapMap.get(key);
      NotifyingListener listener = new NotifyingListener();
      // If the key was invalid, we don't want to attempt to fetch it.
      if (LookupKey.hasValidKeyPrefix(key)) {
        LookupKey lookupKey = new LookupKey.Builder(key).build();
        cacheData.fetchDynamicData(lookupKey, regionalData, listener);
        try {
          listener.waitLoadingEnd();
          // Check to see if there is data for this key now.
          if (cacheData.getObj(key) == null && isCountryKey(key)) {
            // If not, see if there is data in RegionDataConstants.
            logger.info("Server failure: looking up key in region data constants.");
            cacheData.getFromRegionDataConstants(lookupKey);
          }
        } catch (InterruptedException e) {
          throw new RuntimeException(e);
        }
      }
    }
  }

  public void requestData(LookupKey key, DataLoadListener listener) {
    Util.checkNotNull(key, "Null lookup key not allowed");
    JsoMap regionalData = bootstrapMap.get(key.toString());
    cacheData.fetchDynamicData(key, regionalData, listener);
  }

  /**
   * Fetches all data for the specified country from the remote server.
   */
  public void prefetchCountry(String country, DataLoadListener listener) {
    String key = "data/" + country;
    Set<RecursiveLoader> loaders = new HashSet<RecursiveLoader>();
    listener.dataLoadingBegin();
    cacheData.fetchDynamicData(
        new LookupKey.Builder(key).build(), null, new RecursiveLoader(key, loaders, listener));
  }

  /**
   * A helper class to recursively load all sub keys using fetchDynamicData().
   */
  private class RecursiveLoader implements DataLoadListener {
    private final String key;

    private final Set<RecursiveLoader> loaders;

    private final DataLoadListener listener;

    public RecursiveLoader(String key, Set<RecursiveLoader> loaders, DataLoadListener listener) {
      this.key = key;
      this.loaders = loaders;
      this.listener = listener;

      synchronized (loaders) {
        loaders.add(this);
      }
    }

    @Override
    public void dataLoadingBegin() {
    }

    @Override
    public void dataLoadingEnd() {
      final String subKeys = Util.toLowerCaseLocaleIndependent(AddressDataKey.SUB_KEYS.name());
      final String subMores = Util.toLowerCaseLocaleIndependent(AddressDataKey.SUB_MORES.name());

      JsoMap map = cacheData.getObj(key);
      // It is entirely possible that data loading failed and that the map is null.
      if (map != null && map.containsKey(subMores)) {
        // This key could have sub keys.
        String[] mores = map.get(subMores).split("~");
        String[] keys = {};

        if (map.containsKey(subKeys)) {
          keys = map.get(subKeys).split("~");
        }

        if (mores.length != keys.length) {  // This should never happen.
          throw new IndexOutOfBoundsException("mores.length != keys.length");
        }

        for (int i = 0; i < mores.length; i++) {
          if (mores[i].equalsIgnoreCase("true")) {
            // This key should have sub keys.
            String subKey = key + "/" + keys[i];
            cacheData.fetchDynamicData(new LookupKey.Builder(subKey).build(), null,
                new RecursiveLoader(subKey, loaders, listener));
          }
        }
      }

      // TODO: Rethink how notification is handled to avoid error-prone synchronization.
      boolean wasLastLoader = false;
      synchronized (loaders) {
        wasLastLoader = loaders.remove(this) && loaders.isEmpty();
      }
      if (wasLastLoader) {
        listener.dataLoadingEnd();
      }
    }
  }
}
