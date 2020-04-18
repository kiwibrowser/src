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

import com.google.common.collect.ImmutableMap;
import com.google.i18n.addressinput.common.LookupKey.KeyType;
import com.google.i18n.addressinput.testing.InMemoryAsyncRequest;

import junit.framework.TestCase;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import java.util.List;
import java.util.Map;

@RunWith(JUnit4.class)
public class FormControllerTest extends TestCase {
  private static final ImmutableMap<String, String> US_TEST_JSON_MAP = ImmutableMap.of(
      "data", "{\"id\":\"data\",\"countries\":\"CA~US\"}",
      "data/US", "{"
          + "\"id\":\"data/US\","
          + "\"key\":\"US\","
          + "\"name\":\"UNITED STATES\","
          + "\"sub_keys\":\"AL~CA~WY\","
          + "\"sub_names\":\"Alabama~California~Wyoming\"}\n",
      "data/US/AL", "{"
          + "\"id\":\"data/US/AL\","
          + "\"key\":\"AL\","
          + "\"name\":\"Alabama\"}",
      "data/US/CA", "{"
          + "\"id\":\"data/US/CA\","
          + "\"key\":\"CA\","
          + "\"name\":\"California\"}",
      "data/US/WY", "{"
          + "\"id\":\"data/US/WY\","
          + "\"key\":\"WY\","
          + "\"name\":\"Wyoming\"}");

  private static ClientData createTestClientData(Map<String, String> keyToJsonMap) {
    InMemoryAsyncRequest inMemoryAsyncRequest =
        new InMemoryAsyncRequest("http://someurl", keyToJsonMap);
    CacheData cacheData = new CacheData(inMemoryAsyncRequest);
    cacheData.setUrl("http://someurl");
    return new ClientData(cacheData);
  }

  private static final AddressData US_ADDRESS = AddressData.builder().setCountry("US").build();

  private static final AddressData US_CA_ADDRESS = AddressData.builder()
      .setCountry("US")
      .setAdminArea("CA")
      .setLocality("Mt View")
      .setAddress("1098 Alta Ave")
      .setPostalCode("94043")
      .build();

  @Test public void testRequestDataForAddress() {
    LookupKey usCaMtvKey =
        new LookupKey.Builder(KeyType.DATA).setAddressData(US_CA_ADDRESS).build();
    LookupKey usKey = usCaMtvKey.getKeyForUpperLevelField(AddressField.COUNTRY);
    LookupKey usCaKey = usCaMtvKey.getKeyForUpperLevelField(AddressField.ADMIN_AREA);
    ClientData clientData = createTestClientData(US_TEST_JSON_MAP);
    FormController controller = new FormController(clientData, "en", "US");

    controller.requestDataForAddress(US_CA_ADDRESS, null);

    assertNotNull(clientData.get(usKey.toString()));
    assertNotNull(clientData.get(usCaKey.toString()));
    assertNull(clientData.get(usCaMtvKey.toString()));
  }

  @Test public void testRequestDataForBadAddress() {
    AddressData address = AddressData.builder(US_CA_ADDRESS)
        .setAdminArea("FOOBAR")
        .setLocality("KarKar")
        .build();

    LookupKey badKey = new LookupKey.Builder(KeyType.DATA).setAddressData(address).build();
    LookupKey usKey = badKey.getKeyForUpperLevelField(AddressField.COUNTRY);
    LookupKey usAlabamaKey = new LookupKey.Builder(usKey.toString() + "/AL").build();

    ClientData clientData = createTestClientData(US_TEST_JSON_MAP);
    FormController controller = new FormController(clientData, "en", "US");

    controller.requestDataForAddress(address, null);

    assertNotNull(clientData.get(usKey.toString()));
    assertNotNull(clientData.get(usAlabamaKey.toString()));
    assertNull(clientData.get(badKey.toString()));
    List<RegionData> rdata = controller.getRegionData(usKey);
    assertTrue(rdata.size() > 0);
    // Alabama is the first US state (even in our fake data).
    assertEquals("AL", rdata.get(0).getKey());
  }

  @Test public void testRequestDataForCountry() {
    LookupKey usKey = new LookupKey.Builder(KeyType.DATA).setAddressData(US_ADDRESS).build();
    LookupKey usAlabamaKey = new LookupKey.Builder(usKey.toString() + "/AL").build();

    ClientData clientData = createTestClientData(US_TEST_JSON_MAP);
    FormController controller = new FormController(clientData, "en", "US");

    controller.requestDataForAddress(US_ADDRESS, null);

    assertNotNull(clientData.get(usKey.toString()));
    assertNotNull(clientData.get(usAlabamaKey.toString()));
    List<RegionData> rdata = controller.getRegionData(usKey);
    assertTrue(rdata.size() > 0);
    // Alabama is the first US state (even in our fake data).
    assertEquals("AL", rdata.get(0).getKey());
  }
}
