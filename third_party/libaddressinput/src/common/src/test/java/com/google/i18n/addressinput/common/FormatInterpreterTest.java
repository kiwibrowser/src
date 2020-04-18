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

import static com.google.common.truth.Truth.assertThat;
import com.google.i18n.addressinput.common.AddressField.WidthType;
import static java.util.Arrays.asList;

import com.google.i18n.addressinput.common.LookupKey.ScriptType;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Map;

@RunWith(JUnit4.class)
public class FormatInterpreterTest {

  @SuppressWarnings("deprecation")
  @Test public void testIterateUsAddressFields() {
    FormatInterpreter formatInterpreter = new FormatInterpreter(new FormOptions().createSnapshot());
    assertThat(formatInterpreter.getAddressFieldOrder(ScriptType.LOCAL, "US")).containsExactly(
        AddressField.RECIPIENT,
        AddressField.ORGANIZATION,
        AddressField.ADDRESS_LINE_1,
        AddressField.ADDRESS_LINE_2,
        AddressField.LOCALITY,
        AddressField.ADMIN_AREA,
        AddressField.POSTAL_CODE)
        .inOrder();
  }

  /**
   * Tests that this works for the case of Vanuatu, since we have no country-specific formatting
   * data for that country. Should fall back to the default region.
   */
  @SuppressWarnings("deprecation")
  @Test public void testIterateVuAddressFields() {
    FormatInterpreter formatInterpreter = new FormatInterpreter(new FormOptions().createSnapshot());
    assertThat(formatInterpreter.getAddressFieldOrder(ScriptType.LOCAL, "VU")).containsExactly(
        AddressField.RECIPIENT,
        AddressField.ORGANIZATION,
        AddressField.ADDRESS_LINE_1,
        AddressField.ADDRESS_LINE_2,
        AddressField.LOCALITY)
        .inOrder();
  }

  @SuppressWarnings("deprecation")
  @Test public void testOverrideFieldOrder() {
    // Sorting code (CEDEX) is not in US address format and should be
    // neglected even if it is specified in customizeFieldOrder().
    FormatInterpreter myInterpreter = new FormatInterpreter(
        new FormOptions().setCustomFieldOrder("US",
            AddressField.ADMIN_AREA, AddressField.RECIPIENT,
            AddressField.SORTING_CODE, AddressField.POSTAL_CODE)
        .createSnapshot());

    assertThat(myInterpreter.getAddressFieldOrder(ScriptType.LOCAL, "US")).containsExactly(
        AddressField.ADMIN_AREA,
        AddressField.ORGANIZATION,
        AddressField.ADDRESS_LINE_1,
        AddressField.ADDRESS_LINE_2,
        AddressField.LOCALITY,
        AddressField.RECIPIENT,
        AddressField.POSTAL_CODE)
        .inOrder();
  }

  @SuppressWarnings("deprecation")
  @Test public void testIterateTwLatinAddressFields() {
    FormatInterpreter formatInterpreter = new FormatInterpreter(new FormOptions().createSnapshot());
    assertThat(formatInterpreter.getAddressFieldOrder(ScriptType.LATIN, "TW")).containsExactly(
        AddressField.RECIPIENT,
        AddressField.ORGANIZATION,
        AddressField.ADDRESS_LINE_1,
        AddressField.ADDRESS_LINE_2,
        AddressField.LOCALITY,
        AddressField.ADMIN_AREA,
        AddressField.POSTAL_CODE)
        .inOrder();
  }

  @Test public void testGetEnvelopeAddress_MissingFields_LiteralsBetweenFields() {
    FormatInterpreter formatInterpreter = new FormatInterpreter(new FormOptions().createSnapshot());
    AddressData.Builder addressBuilder = AddressData.builder()
        .setCountry("US");

    assertThat(formatInterpreter.getEnvelopeAddress(addressBuilder.build())).isEmpty();

    addressBuilder.setAdminArea("CA");
    assertThat(formatInterpreter.getEnvelopeAddress(addressBuilder.build()))
        .containsExactly("CA");

    addressBuilder.setLocality("Los Angeles");
    assertThat(formatInterpreter.getEnvelopeAddress(addressBuilder.build()))
        .containsExactly("Los Angeles, CA");

    addressBuilder.setPostalCode("90291");
    assertThat(formatInterpreter.getEnvelopeAddress(addressBuilder.build()))
        .containsExactly("Los Angeles, CA 90291");

    addressBuilder.setAdminArea("");
    assertThat(formatInterpreter.getEnvelopeAddress(addressBuilder.build()))
        .containsExactly("Los Angeles 90291");

    addressBuilder.setLocality("");
    addressBuilder.setAdminArea("CA");
    assertThat(formatInterpreter.getEnvelopeAddress(addressBuilder.build()))
        .containsExactly("CA 90291");
  }

  @Test public void testGetEnvelopeAddress_MissingFields_LiteralsOnSeparateLine() {
    FormatInterpreter formatInterpreter = new FormatInterpreter(new FormOptions().createSnapshot());
    AddressData.Builder addressBuilder = AddressData.builder()
        .setCountry("AX");

    assertThat(formatInterpreter.getEnvelopeAddress(addressBuilder.build()))
        .containsExactly("ÅLAND");

    addressBuilder.setLocality("City");
    assertThat(formatInterpreter.getEnvelopeAddress(addressBuilder.build()))
        .containsExactly("City", "ÅLAND").inOrder();

    addressBuilder.setPostalCode("123");
    assertThat(formatInterpreter.getEnvelopeAddress(addressBuilder.build()))
        .containsExactly("AX-123 City", "ÅLAND").inOrder();
  }

  @Test public void testGetEnvelopeAddress_MissingFields_LiteralBeforeField() {
    FormatInterpreter formatInterpreter = new FormatInterpreter(new FormOptions().createSnapshot());
    AddressData.Builder addressBuilder = AddressData.builder()
        .setCountry("JP")
        .setLanguageCode("ja");

    assertThat(formatInterpreter.getEnvelopeAddress(addressBuilder.build())).isEmpty();

    addressBuilder.setPostalCode("123");
    assertThat(formatInterpreter.getEnvelopeAddress(addressBuilder.build()))
        .containsExactly("〒123");

    addressBuilder.setAdminArea("Prefecture");
    assertThat(formatInterpreter.getEnvelopeAddress(addressBuilder.build()))
        .containsExactly("〒123", "Prefecture").inOrder();

    addressBuilder.setPostalCode("");
    assertThat(formatInterpreter.getEnvelopeAddress(addressBuilder.build()))
        .containsExactly("Prefecture");
  }

  @Test public void testGetEnvelopeAddress_MissingFields_DuplicateField() {
    FormatInterpreter formatInterpreter = new FormatInterpreter(new FormOptions().createSnapshot());
    AddressData.Builder addressBuilder = AddressData.builder()
        .setCountry("CI");

    addressBuilder.setSortingCode("123");
    assertThat(formatInterpreter.getEnvelopeAddress(addressBuilder.build()))
        .containsExactly("123 123");

    addressBuilder.setAddressLine1("456 Main St");
    assertThat(formatInterpreter.getEnvelopeAddress(addressBuilder.build()))
        .containsExactly("123 456 Main St 123");

    addressBuilder.setLocality("Yamoussoukro");
    assertThat(formatInterpreter.getEnvelopeAddress(addressBuilder.build()))
        .containsExactly("123 456 Main St Yamoussoukro 123");

    addressBuilder.setSortingCode("");
    assertThat(formatInterpreter.getEnvelopeAddress(addressBuilder.build()))
        .containsExactly("456 Main St Yamoussoukro");

    addressBuilder.setAddressLines(new ArrayList<String>());
    assertThat(formatInterpreter.getEnvelopeAddress(addressBuilder.build()))
        .containsExactly("Yamoussoukro");
  }

  @Test public void testUsEnvelopeAddress() {
    FormatInterpreter formatInterpreter = new FormatInterpreter(new FormOptions().createSnapshot());
    AddressData address = AddressData.builder()
        .setCountry("US")
        .setAdminArea("CA")
        .setLocality("Mt View")
        .setAddress("1098 Alta Ave")
        .setPostalCode("94043")
        .build();
    assertThat(formatInterpreter.getEnvelopeAddress(address))
        .containsExactly("1098 Alta Ave", "Mt View, CA 94043").inOrder();
  }

  @Test public void testTwEnvelopeAddress() {
    FormatInterpreter formatInterpreter = new FormatInterpreter(new FormOptions().createSnapshot());
    // To be in this order, the whole address should really be in Traditional Chinese - for
    // readability, only the neighbourhood and city are.
    AddressData address = AddressData.builder()
        .setCountry("TW")
        .setAdminArea("\u53F0\u5317\u5E02")  // Taipei city
        .setLocality("\u5927\u5B89\u5340")  // Da-an district
        .setAddress("Sec. 3 Hsin-yi Rd.")
        .setPostalCode("106")
        .setOrganization("Giant Bike Store")
        .setRecipient("Mr. Liu")
        .build();
    assertThat(formatInterpreter.getEnvelopeAddress(address)).containsExactly(
        "106",
        "\u53F0\u5317\u5E02\u5927\u5B89\u5340",  // Taipei city, Da-an district
        "Sec. 3 Hsin-yi Rd.",
        "Giant Bike Store",
        "Mr. Liu")
        .inOrder();
  }

  @Test public void testGetEnvelopeAddressIncompleteAddress() {
    FormatInterpreter formatInterpreter = new FormatInterpreter(new FormOptions().createSnapshot());
    AddressData address = AddressData.builder()
        .setCountry("US")
        .setAdminArea("CA")
        // Locality is missing
        .setAddress("1098 Alta Ave")
        .setPostalCode("94043")
        .build();
    assertThat(formatInterpreter.getEnvelopeAddress(address))
        .containsExactly("1098 Alta Ave", "CA 94043").inOrder();
  }

  @Test public void testGetEnvelopeAddressEmptyAddress() {
    FormatInterpreter formatInterpreter = new FormatInterpreter(new FormOptions().createSnapshot());
    AddressData address = AddressData.builder().setCountry("US").build();
    assertThat(formatInterpreter.getEnvelopeAddress(address)).isEmpty();
  }

  @Test public void testGetEnvelopeAddressLeadingPostPrefix() {
    FormatInterpreter formatInterpreter = new FormatInterpreter(new FormOptions().createSnapshot());
    AddressData address = AddressData.builder()
        .setCountry("CH")
        .setPostalCode("8047")
        .setLocality("Herrliberg")
        .build();
    assertThat(formatInterpreter.getEnvelopeAddress(address))
        .containsExactly("CH-8047 Herrliberg").inOrder();
  }

  @Test public void testSvAddress() {
    FormatInterpreter formatInterpreter = new FormatInterpreter(new FormOptions().createSnapshot());
    AddressData svAddress = AddressData.builder()
        .setCountry("SV")
        .setAdminArea("Ahuachapán")
        .setLocality("Ahuachapán")
        .setAddressLines(asList("Some Street 12"))
        .build();
    assertThat(formatInterpreter.getEnvelopeAddress(svAddress))
        .containsExactly("Some Street 12", "Ahuachapán", "Ahuachapán").inOrder();

    AddressData svAddressWithPostCode =
        AddressData.builder(svAddress).setPostalCode("CP 2101").build();
    assertThat(formatInterpreter.getEnvelopeAddress(svAddressWithPostCode))
        .containsExactly("Some Street 12", "CP 2101-Ahuachapán", "Ahuachapán").inOrder();
  }

  private Map<String, String> createWidthOverrideRegionData(String overridesString) {
    Map<String, String> map = new HashMap<String, String>(1);
    map.put("US", "{\"width_overrides\":\"" + overridesString + "\"}");
    return map;
  }

  @Test public void testGetWidthOverride_goodData() {
    Map<String, String> fakeData = createWidthOverrideRegionData("%S:L%C:S");
    assertThat(FormatInterpreter.getWidthOverride(AddressField.LOCALITY, "US", fakeData))
        .isEqualTo(WidthType.SHORT);
    assertThat(FormatInterpreter.getWidthOverride(AddressField.ADMIN_AREA, "US", fakeData))
        .isEqualTo(WidthType.LONG);
    assertThat(FormatInterpreter.getWidthOverride(AddressField.POSTAL_CODE, "US", fakeData))
        .isNull();
  }

  @Test public void testGetWidthOverride_singleOverride() {
    Map<String, String> fakeData = createWidthOverrideRegionData("%S:S");
    assertThat(FormatInterpreter.getWidthOverride(AddressField.ADMIN_AREA, "US", fakeData))
        .isEqualTo(WidthType.SHORT);
    assertThat(FormatInterpreter.getWidthOverride(AddressField.LOCALITY, "US", fakeData)).isNull();
  }

  @Test public void testGetWidthOverride_badData() {
    // Doesn't test that the parsing code actually just skips bad keys/values. That's nice, but
    // not essential.
    for (String overridesString : new String[]{
        "", "%", ":", "%%", "%:", "%CX", "%CX:", "C:S", "%C:", "%C:SS", "%C:SS%Q:L"}) {
      Map<String, String> fakeData = createWidthOverrideRegionData(overridesString);
      for (AddressField field : AddressField.values()) {
        assertThat(FormatInterpreter.getWidthOverride(field, "US", fakeData))
            .named("With field " + field + " and overrides string '" + overridesString + "'")
            .isNull();
      }
    }
  }

  @Test public void testGetWidthOverride_skipTooLongKeys() {
    for (String overridesString : new String[]{
        "%NH:L%C:S", "%Z:L%BG:S%C:S", "%C:S%NH:S", "%NH:QL%C:S", "%NH:L%C:S%BG:L"}) {
      Map<String, String> fakeData = createWidthOverrideRegionData(overridesString);
        assertThat(FormatInterpreter.getWidthOverride(AddressField.LOCALITY, "US", fakeData))
            .named("For LOCALITY (C) and overrides string '" + overridesString + "'")
            .isEqualTo(WidthType.SHORT);
        assertThat(FormatInterpreter.getWidthOverride(AddressField.ADMIN_AREA, "US", fakeData))
            .named("For ADMIN_AREA (S) and overrides string '" + overridesString + "'")
            .isNull();
    }
  }
}
