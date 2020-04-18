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
import static java.util.Arrays.asList;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;

import com.google.i18n.addressinput.testing.AddressDataMapLoader;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import java.util.Arrays;
import java.util.Collections;
import java.util.EnumMap;
import java.util.EnumSet;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;

/**
 * Spot check the standard dataset for various cases of interest.  This is not an exhaustive test.
 */
@RunWith(JUnit4.class)
public class FieldVerifierTest {
  private static final StandardAddressVerifier VERIFIER = new StandardAddressVerifier(
      new FieldVerifier(new AddressVerificationData(AddressDataMapLoader.TEST_COUNTRY_DATA)));

  @Test public void testUnitedStatesOk() {
    AddressProblems problems = new AddressProblems();
    AddressData addr = AddressData.builder()
        .setCountry("US")
        .setAdminArea("CA")
        .setLocality("Mountain View")
        .setAddress("1234 Somewhere")
        .setPostalCode("94025")
        .build();
    VERIFIER.verify(addr, problems);
    assertTrue(problems.isEmpty());  // No mismatch.
  }

  /**
   * Testing that UNEXPECTED_FIELD is returned for the US if a sorting code is used.
   */
  @Test public void testUnitedStatesWithIllegalField() {
    AddressProblems problems = new AddressProblems();
    AddressData addr = AddressData.builder()
        .setCountry("US")
        .setAdminArea("CA")
        .setLocality("Mountain View")
        .setAddress("1234 Somewhere")
        .setPostalCode("94025")
        .setSortingCode("ABC")
        .build();
    VERIFIER.verify(addr, problems);
    assertEquals(AddressProblemType.UNEXPECTED_FIELD,
        problems.getProblem(AddressField.SORTING_CODE));
  }

  @Test public void testFranceInvalidPostcode() {
    AddressProblems problems = new AddressProblems();
    // Post-code should be 5 digits long.
    AddressData addr = AddressData.builder()
        .setCountry("FR")
        .setLocality("Paris")
        .setAddress("1234 Rue Arc-en-ciel")
        .setPostalCode("9425")
        .build();
    VERIFIER.verify(addr, problems);
    assertFalse(problems.isEmpty());
    assertEquals(AddressProblemType.INVALID_FORMAT, problems.getProblem(AddressField.POSTAL_CODE));
  }

  @Test public void testUnitedStatesZipMismatch() {
    AddressProblems problems = new AddressProblems();
    AddressData addr = AddressData.builder()
        .setCountry("US")
        .setAdminArea("CA")
        .setLocality("Mountain View")
        .setPostalCode("12345")
        .setAddress("Some Street")
        .build();
    VERIFIER.verify(addr, problems);

    assertFalse(problems.isEmpty());
    assertEquals(AddressProblemType.MISMATCHING_VALUE,
        problems.getProblem(AddressField.POSTAL_CODE));
  }

  @Test public void testUnitedStatesNotOk() {
    AddressProblems problems = new AddressProblems();
    AddressData addr = AddressData.builder()
        .setCountry("US")
        .setAdminArea("CA")
        .setLocality(null)
        .setDependentLocality("Foo Bar")
        .setPostalCode("12345")
        .build();
    VERIFIER.verify(addr, problems);

    assertFalse(problems.isEmpty());
    assertEquals(AddressProblemType.MISMATCHING_VALUE,
        problems.getProblem(AddressField.POSTAL_CODE));
    assertEquals(AddressProblemType.MISSING_REQUIRED_FIELD,
        problems.getProblem(AddressField.LOCALITY));
    assertEquals(AddressProblemType.MISSING_REQUIRED_FIELD,
        problems.getProblem(AddressField.STREET_ADDRESS));
  }

  @Test public void testChinaOk() {
    AddressProblems problems = new AddressProblems();
    AddressData addr = AddressData.builder()
        .setCountry("CN")
        .setAdminArea("Beijing Shi")
        .setLocality("Xicheng Qu")
        .setAddress("Yitiao Lu")
        .setPostalCode("123456")
        .build();
    VERIFIER.verify(addr, problems);
    assertTrue(problems.isEmpty());
  }

  @Test public void testHongKongInvalidDistrictInEnglish() {
    AddressProblems problems = new AddressProblems();
    AddressData addr = AddressData.builder()
        .setCountry("HK")
        .setAdminArea("New Territories")
        .setLocality("Cheung Sha Lantau")
        .setLanguageCode("en")
        .build();
    VERIFIER.verify(addr, problems);
    assertFalse(problems.isEmpty());

    assertFalse(problems.isEmpty());
    // The administrative area matches a known value, so we shouldn't get any problem.
    assertNull(problems.getProblem(AddressField.ADMIN_AREA));
    assertEquals(AddressProblemType.MISSING_REQUIRED_FIELD,
        problems.getProblem(AddressField.STREET_ADDRESS));
    // However, the locality does not.
    assertEquals(AddressProblemType.UNKNOWN_VALUE, problems.getProblem(AddressField.LOCALITY));
  }

  @Test public void testHongKongNoStreetInEnglish() {
    AddressProblems problems = new AddressProblems();
    AddressData addr = AddressData.builder()
        .setCountry("HK")
        .setAdminArea("New Territories")
        .setLocality("Cheung Sha Lantau Island")
        .setLanguageCode("en")
        .build();
    VERIFIER.verify(addr, problems);
    assertFalse(problems.isEmpty());

    assertEquals(AddressProblemType.MISSING_REQUIRED_FIELD,
        problems.getProblem(AddressField.STREET_ADDRESS));
    assertNull(problems.getProblem(AddressField.ADMIN_AREA));
    assertNull(problems.getProblem(AddressField.LOCALITY));
  }

  @Test public void testGermanAddress() {
    AddressProblems problems = new AddressProblems();
    AddressData addr = AddressData.builder()
        .setCountry("DE")
        .setLocality("Berlin")
        .setAddress("Huttenstr. 50")
        .setPostalCode("10553")
        .setOrganization("BMW AG Niederkassung Berlin")
        .setRecipient("Herr Diefendorf")
        .build();

    VERIFIER.verify(addr, problems);
    assertTrue(problems.isEmpty());

    // Clone the address and clear the city.
    addr = AddressData.builder().set(addr).setLocality(null).build();

    VERIFIER.verify(addr, problems);
    assertEquals(AddressProblemType.MISSING_REQUIRED_FIELD,
        problems.getProblem(AddressField.LOCALITY));
  }

  @Test public void testIrishAddress() {
    AddressProblems problems = new AddressProblems();
    AddressData addr = AddressData.builder()
        .setCountry("IE")
        .setLocality("Dublin")
        .setAdminArea("Co. Dublin")
        .setAddress("7424 118 Avenue NW")
        .setRecipient("Conan O'Brien")
        .build();

    VERIFIER.verify(addr, problems);
    assertTrue(problems.isEmpty());

    // Clones address but clears the county. This address should be valid since county is not
    // required.
    addr = AddressData.builder().set(addr).setAdminArea(null).build();

    VERIFIER.verify(addr, problems);
    assertTrue(problems.isEmpty());
  }

  @Test public void testChinaPostalCodeMismatchingFormat() {
    AddressProblems problems = new AddressProblems();
    // This post-code is 5 digits - which is only valid in Taiwan, not Beijing.
    AddressData addr = AddressData.builder()
        .setCountry("CN")
        .setAdminArea("Beijing Shi")
        .setLocality("Xicheng Qu")
        .setPostalCode("12345")
        .setAddress("Street")
        .build();
    VERIFIER.verify(addr, problems);
    assertFalse(problems.isEmpty());

    assertEquals(AddressProblemType.INVALID_FORMAT, problems.getProblem(AddressField.POSTAL_CODE));
  }

  @Test public void testChinaPostalCodeBadFormat() {
    AddressProblems problems = new AddressProblems();
    AddressData addr = AddressData.builder()
        .setCountry("CN")
        .setAdminArea("Beijing Shi")
        .setLocality("Xicheng Qu")
        .setPostalCode("12345")
        .setAddress("Street")
        .build();
    VERIFIER.verify(addr, problems);

    assertFalse(problems.isEmpty());
    assertEquals(AddressProblemType.INVALID_FORMAT, problems.getProblem(AddressField.POSTAL_CODE));
  }

  /**
   * If there is a postal code pattern for a certain country, and the input postal code is empty, it
   * should not be reported as an invalid postal code format. Whether or not an empty postal code is
   * okay should be determined by checks for required fields.
   */
  @Test public void testEmptyPostalCodeReportedAsGoodFormat() {
    AddressProblems problems = new AddressProblems();
    // Chilean address has a postal code format pattern, but does not require postal code. The
    // following address is valid.
    AddressData addr = AddressData.builder()
        .setCountry("CL")
        .setAddress("GUSTAVO LE PAIGE ST #159")
        .setAdminArea("Atacama")
        .setLocality("Huasco")
        .setPostalCode("")
        .build();
    VERIFIER.verify(addr, problems);
    assertTrue(problems.isEmpty());

    // Now check for US addresses, which require a postal code. The following address's postal code
    // is wrong because it is missing a required field, not because it doesn't match the expected
    // postal code pattern.
    addr = AddressData.builder().setCountry("US").setPostalCode("").build();
    VERIFIER.verify(addr, problems);

    assertFalse(problems.isEmpty());
    assertEquals(AddressProblemType.MISSING_REQUIRED_FIELD,
        problems.getProblem(AddressField.POSTAL_CODE));
  }

  @Test public void testChinaTaiwanOk() {
    AddressProblems problems = new AddressProblems();
    AddressData addr = AddressData.builder()
        .setCountry("CN")
        .setAdminArea("Taiwan")
        .setLocality("Taichung City")
        .setDependentLocality("Xitun District")
        .setAddress("12345 Yitiao Lu")
        .setPostalCode("407")
        .build();
    VERIFIER.verify(addr, problems);
    assertTrue(problems.isEmpty());
  }

  @Test public void testChinaTaiwanUnknownDistrict() {
    AddressProblems problems = new AddressProblems();
    AddressData addr = AddressData.builder()
        .setCountry("CN")
        .setAdminArea("Taiwan")
        .setLocality("Taichung City")
        .setDependentLocality("Foo Bar")
        .setAddress("12345 Yitiao Lu")
        .setPostalCode("400")
        .build();
    VERIFIER.verify(addr, problems);

    assertFalse(problems.isEmpty());
    assertEquals(AddressProblemType.UNKNOWN_VALUE,
        problems.getProblem(AddressField.DEPENDENT_LOCALITY));
  }

  @Test public void testStreetVerification() {
    AddressProblems problems = new AddressProblems();
    // Missing street address.
    AddressData address = AddressData.builder()
        .setCountry("US")
        .setAdminArea("CA")
        .setLocality("Mountain View")
        .setPostalCode("94025")
        .build();
    assertThat(address.getAddressLines()).isEmpty();

    VERIFIER.verify(address, problems);

    assertEquals(AddressProblemType.MISSING_REQUIRED_FIELD,
        problems.getProblem(AddressField.STREET_ADDRESS));
  }

  // Tests The Bahamas' address
  @Test public void testBahamas() {
    AddressProblems problems = new AddressProblems();
    AddressData address = AddressData.builder()
        .setAddress("Abaco Beach Resort & Boat Habour")
        .setLocality("Treasure Cay")
        .setAdminArea("Abaco")
        .setCountry("BS")
        .build();
    VERIFIER.verify(address, problems);
    assertTrue(problems.isEmpty());
  }

  @Test public void testJapan() {
    AddressProblems problems = new AddressProblems();
    AddressData address = AddressData.builder()
        .setRecipient("\u5BAE\u672C \u8302")  // SHIGERU_MIYAMOTO
        .setAddress("\u4E0A\u9CE5\u7FBD\u927E\u7ACB\u753A11\u756A\u5730")
        .setAdminArea("\u4eac\u90fd\u5e9c")  // Kyoto prefecture
        .setLocality("\u4EAC\u90FD\u5E02")  // Kyoto city
        .setCountry("JP")
        .setPostalCode("601-8501")
        .build();
    VERIFIER.verify(address, problems);
    assertTrue(problems.toString(), problems.isEmpty());
  }

  @Test public void testJapanLatin() {
    AddressProblems problems = new AddressProblems();
    AddressData address = AddressData.builder()
        .setRecipient("Shigeru Miyamoto")
        .setAddress("11-1 Kamitoba-hokotate-cho")
        .setAdminArea("KYOTO")
        .setLocality("Kyoto")
        .setLanguageCode("ja_Latn")
        .setCountry("JP")
        .setPostalCode("601-8501")
        .build();
    VERIFIER.verify(address, problems);
    assertTrue(problems.isEmpty());
  }

  @Test public void testJapanLatinInvalidAdmin() {
    AddressProblems problems = new AddressProblems();
    AddressData address = AddressData.builder()
        .setRecipient("Shigeru Miyamoto")  // SHIGERU_MIYAMOTO_ENGLISH
        .setAddress("11-1 Kamitoba-hokotate-cho")
        .setAdminArea("Fake Admin")
        .setLocality("Kyoto")
        .setLanguageCode("ja_Latn")
        .setCountry("JP")
        .setPostalCode("601-8501")
        .build();
    VERIFIER.verify(address, problems);
    assertFalse(problems.isEmpty());
    assertEquals(AddressProblemType.UNKNOWN_VALUE, problems.getProblem(AddressField.ADMIN_AREA));
  }

  @Test public void testUnrecognizedFormatCheckWithNoState_US() {
    AddressProblems problems = new AddressProblems();
    Map<AddressField, List<AddressProblemType>> customChecks =
        new HashMap<AddressField, List<AddressProblemType>>();
    AddressProblemType countryProblems[] = {
      AddressProblemType.UNEXPECTED_FIELD,
      AddressProblemType.MISSING_REQUIRED_FIELD,
      AddressProblemType.UNKNOWN_VALUE };
    customChecks.put(AddressField.COUNTRY,
                     Collections.unmodifiableList(Arrays.asList(countryProblems)));
    AddressProblemType postalCodeProblems[] = {
      AddressProblemType.UNEXPECTED_FIELD,
      AddressProblemType.MISSING_REQUIRED_FIELD,
      AddressProblemType.INVALID_FORMAT,
      AddressProblemType.MISMATCHING_VALUE };
    customChecks.put(AddressField.POSTAL_CODE,
                     Collections.unmodifiableList(Arrays.asList(postalCodeProblems)));
    StandardAddressVerifier customVerifier = new StandardAddressVerifier(
      new FieldVerifier(new AddressVerificationData(AddressDataMapLoader.TEST_COUNTRY_DATA)),
      customChecks);

    AddressData addr = AddressData.builder().setCountry("US").setPostalCode("0000").build();
    customVerifier.verify(addr, problems);
    assertFalse(problems.isEmpty());
    // There should be a problem, complaining that the postal-code was invalid, since it doesn't
    // match the pattern for any state.
    assertEquals(AddressProblemType.INVALID_FORMAT,
        problems.getProblem(AddressField.POSTAL_CODE));

    AddressData goodAddr =
        AddressData.builder().setCountry("US").setPostalCode("65213").build();

    problems.clear();
    customVerifier.verify(goodAddr, problems);
    assertTrue(problems.toString(), problems.isEmpty());
  }

  /**
   * Testing that verifying behaviour for one country doesn't cause the validator to have strange
   * behaviour when reused for another country later.
   */
  @Test public void testNoSideEffects() {
    AddressProblems problems = new AddressProblems();
    AddressData addressDE = AddressData.builder()
        .setCountry("DE")
        .setAdminArea("BY")
        .build();

    VERIFIER.verify(addressDE, problems);
    assertFalse(problems.isEmpty());
    assertEquals(AddressProblemType.MISSING_REQUIRED_FIELD,
        problems.getProblem(AddressField.STREET_ADDRESS));
    assertEquals(AddressProblemType.UNEXPECTED_FIELD,
        problems.getProblem(AddressField.ADMIN_AREA));
    assertEquals(AddressProblemType.MISSING_REQUIRED_FIELD,
        problems.getProblem(AddressField.POSTAL_CODE));

    AddressData addressMX = AddressData.builder().setCountry("MX").build();

    problems.clear();
    VERIFIER.verify(addressMX, problems);
    assertFalse(problems.isEmpty());
    assertEquals(AddressProblemType.MISSING_REQUIRED_FIELD,
        problems.getProblem(AddressField.STREET_ADDRESS));
    assertEquals(AddressProblemType.MISSING_REQUIRED_FIELD,
        problems.getProblem(AddressField.LOCALITY));
    assertEquals(AddressProblemType.MISSING_REQUIRED_FIELD,
        problems.getProblem(AddressField.POSTAL_CODE));

    problems.clear();
    VERIFIER.verify(addressDE, problems);
    assertFalse(problems.isEmpty());
    assertEquals(AddressProblemType.MISSING_REQUIRED_FIELD,
        problems.getProblem(AddressField.STREET_ADDRESS));
    assertEquals(AddressProblemType.UNEXPECTED_FIELD,
        problems.getProblem(AddressField.ADMIN_AREA));
    assertEquals(AddressProblemType.MISSING_REQUIRED_FIELD,
        problems.getProblem(AddressField.POSTAL_CODE));
    assertNull(problems.getProblem(AddressField.LOCALITY));
  }

  @Test public void testElSalvador() {
    AddressProblems problems = new AddressProblems();
    // The address should be valid both with and without a post-code.
    AddressData addressNoPostcode = AddressData.builder()
        .setAddress("Some Street 12")
        // We have to have an admin area set here that exactly matches the *key* of the sub-region
        // data in order for any more detailed verification data to be loaded.
        .setAdminArea("AHUACHAPAN")
        .setLocality("Ahuachapán")
        .setCountry("SV")
        .build();
    VERIFIER.verify(addressNoPostcode, problems);
    assertTrue(problems.isEmpty());

    AddressData validAddress = AddressData.builder(addressNoPostcode)
        .setPostalCode("CP 2101")
        .build();
    VERIFIER.verify(validAddress, problems);
    assertTrue(problems.isEmpty());

    AddressData addressInvalidNoPostCodePrefix = AddressData.builder(validAddress)
        .setPostalCode("2101")
        .build();
    problems.clear();
    VERIFIER.verify(addressInvalidNoPostCodePrefix, problems);
    assertFalse(problems.isEmpty());
    assertEquals(AddressProblemType.INVALID_FORMAT,
        problems.getProblem(AddressField.POSTAL_CODE));

    AddressData addressInvalidPostCodeDigits = AddressData.builder(validAddress)
        .setPostalCode("CP 9876")
        .build();
    problems.clear();
    VERIFIER.verify(addressInvalidPostCodeDigits, problems);
    assertFalse(problems.isEmpty());
    assertEquals(AddressProblemType.INVALID_FORMAT,
        problems.getProblem(AddressField.POSTAL_CODE));

    // Correct post-code, but not for this province.
    AddressData addressInvalidPostCode = AddressData.builder(validAddress)
        .setPostalCode("CP 2201")  // Santa Ana, not Ahuachapán.
        .build();
    problems.clear();
    VERIFIER.verify(addressInvalidPostCode, problems);
    assertFalse(problems.isEmpty());
    assertEquals(AddressProblemType.MISMATCHING_VALUE,
        problems.getProblem(AddressField.POSTAL_CODE));
    assertNull(problems.getProblem(AddressField.ADMIN_AREA));
  }

  @Test public void testCanadaMixedCasePostcode() {
    AddressProblems problems = new AddressProblems();
    AddressData address = AddressData.builder()
        .setRecipient("Joe Bloggs")
        .setAddress("11 East St")
        .setLocality("Montreal")
        .setAdminArea("Quebec")
        .setCountry("CA")
        .setPostalCode("H2b 2y5")
        .build();
    VERIFIER.verify(address, problems);
    assertTrue(problems.isEmpty());
  }

  @Test public void testNoCountryProvided() {
    AddressProblems problems = new AddressProblems();
    AddressData address = AddressData.builder()
        .setLanguageCode("en")
        .build();
    VERIFIER.verify(address, problems);
    assertFalse(problems.toString(), problems.isEmpty());
    assertEquals(AddressProblemType.MISSING_REQUIRED_FIELD,
        problems.getProblem(AddressField.COUNTRY));
  }

  @Test public void testMultipleAddressLines() {
    AddressProblems problems = new AddressProblems();
    AddressData address = AddressData.builder()
        .setCountry("US")
        .setAdminArea("CA")
        .setLocality("Mountain View")
        .setAddressLines(asList("Somewhere", "1234"))
        .setPostalCode("94025").build();
    VERIFIER.verify(address, problems);
    assertTrue(problems.isEmpty());
  }

  @Test public void testFieldVerifierUsesRegionDataConstantsForFmtAndRequire() {
    Map<AddressDataKey, String> map = new EnumMap<AddressDataKey, String>(AddressDataKey.class);
    // Values for format and require are deliberately different from RegionDataConstants so that we
    // can test that the RDC's version is preferred.
    map.put(AddressDataKey.FMT, "%N%n%O");
    map.put(AddressDataKey.REQUIRE, "A");
    map.put(AddressDataKey.ZIP_NAME_TYPE, "postal");
    map.put(AddressDataKey.STATE_NAME_TYPE, "region");
    map.put(AddressDataKey.SUBLOCALITY_NAME_TYPE, "neighbourhood");
    map.put(AddressDataKey.SUB_KEYS, "Test");
    map.put(AddressDataKey.ID, "data/FM");
    // In real life this represents the nodedata obtained from e.g. our metadata server.
    AddressVerificationNodeData testNode = new AddressVerificationNodeData(map);
    FieldVerifier fieldVerifier = new FieldVerifier(VERIFIER.rootVerifier, testNode);

    // Used and required obtained from RegionDataConstants for FM.
    Set<AddressField> expectedPossibleFields = EnumSet.of(AddressField.RECIPIENT,
        AddressField.ORGANIZATION, AddressField.STREET_ADDRESS, AddressField.LOCALITY,
        AddressField.ADMIN_AREA, AddressField.POSTAL_CODE, AddressField.COUNTRY);
    Set<AddressField> expectedRequiredFields = EnumSet.of(AddressField.STREET_ADDRESS,
        AddressField.LOCALITY, AddressField.ADMIN_AREA, AddressField.POSTAL_CODE,
        AddressField.COUNTRY);
    assertEquals(expectedPossibleFields, fieldVerifier.possiblyUsedFields);
    assertEquals(expectedRequiredFields, fieldVerifier.required);
    assertEquals("data/FM", fieldVerifier.id);
    // Keys should be populated from the test node.
    assertEquals("[Test]", Arrays.toString(fieldVerifier.keys));
  }
}
