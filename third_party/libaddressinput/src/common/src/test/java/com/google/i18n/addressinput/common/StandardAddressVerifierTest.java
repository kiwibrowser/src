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
import static com.google.i18n.addressinput.common.AddressField.ADMIN_AREA;
import static com.google.i18n.addressinput.common.AddressField.COUNTRY;
import static com.google.i18n.addressinput.common.AddressField.DEPENDENT_LOCALITY;
import static com.google.i18n.addressinput.common.AddressField.LOCALITY;
import static com.google.i18n.addressinput.common.AddressField.POSTAL_CODE;
import static com.google.i18n.addressinput.common.AddressField.STREET_ADDRESS;
import static com.google.i18n.addressinput.common.AddressProblemType.INVALID_FORMAT;
import static com.google.i18n.addressinput.common.AddressProblemType.MISMATCHING_VALUE;
import static com.google.i18n.addressinput.common.AddressProblemType.MISSING_REQUIRED_FIELD;
import static com.google.i18n.addressinput.common.AddressProblemType.UNKNOWN_VALUE;
import static com.google.i18n.addressinput.testing.AddressDataMapLoader.TEST_COUNTRY_DATA;
import static org.junit.Assert.assertEquals;

import com.google.common.collect.ImmutableList;
import com.google.common.collect.ImmutableMap;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import java.util.EnumSet;
import java.util.List;
import java.util.Map;

@RunWith(JUnit4.class)
public class StandardAddressVerifierTest {

  private static StandardAddressVerifier verifierFor(
      Map<AddressField, List<AddressProblemType>> problemMap) {
    FieldVerifier fieldVerifier = new FieldVerifier(new AddressVerificationData(TEST_COUNTRY_DATA));
    return new StandardAddressVerifier(fieldVerifier, problemMap);
  }

  private static AddressProblems verify(AddressData address) {
    AddressProblems problems = new AddressProblems();
    verifierFor(StandardChecks.PROBLEM_MAP).verify(address, problems);
    return problems;
  }

  // A minimally valid US address (contains no extraneous fields).
  private static final AddressData VALID_US_ADDRESS = AddressData.builder()
      .setCountry("US")
      .setAddress("1234 Somewhere")
      .setLocality("Mountain View")
      .setAdminArea("CA")
      .setPostalCode("94025")
      .build();

  @Test public void testUnitedStates_Valid() {
    assertThat(verify(VALID_US_ADDRESS).getProblems()).isEmpty();
  }

  @Test public void testUnitedStates_PostalCodeMismatch() {
    AddressData address = AddressData.builder(VALID_US_ADDRESS)
        .setPostalCode("12345")
        .build();
    assertEquals(ImmutableMap.of(
        POSTAL_CODE, MISMATCHING_VALUE),
        verify(address).getProblems());
  }

  @Test public void testUnitedStates_MissingStreetAddress() {
    // Missing street address
    AddressData address = AddressData.builder(VALID_US_ADDRESS)
        .setAddress("")
        .build();
    assertEquals(ImmutableMap.of(
        STREET_ADDRESS, MISSING_REQUIRED_FIELD),
        verify(address).getProblems());
  }

  @Test public void testUnitedStates_MultipleProblems() {
    AddressData address = AddressData.builder(VALID_US_ADDRESS)
        .setPostalCode("12345")
        .setLocality(null)
        .build();
    assertEquals(ImmutableMap.of(
        POSTAL_CODE, MISMATCHING_VALUE,
        LOCALITY, MISSING_REQUIRED_FIELD),
        verify(address).getProblems());
  }

  @Test public void testEmptyProblemMap() {
    // Create an empty verifier that's not looking for any problems.
    StandardAddressVerifier emptyVerifier =
        verifierFor(ImmutableMap.<AddressField, List<AddressProblemType>>of());

    // Check that standard verifier reports at least one problem for the broken address.
    AddressData brokenAddress = AddressData.builder()
        .setCountry("US")
        .build();
    assertThat(verify(brokenAddress).getProblems()).isNotEmpty();

    // Check that the empty verify ignores all the problems.
    AddressProblems problems = new AddressProblems();
    emptyVerifier.verify(brokenAddress, problems);
    // We aren't looking for any problems, so shouldn't find any.
    assertThat(problems.getProblems()).isEmpty();
  }

  @Test public void testCustomProblemMap() {
    // Create a map that looks only for postal code problems.
    StandardAddressVerifier postalCodeVerifier =
        verifierFor(ImmutableMap.<AddressField, List<AddressProblemType>>of(
        POSTAL_CODE, ImmutableList.of(INVALID_FORMAT, MISSING_REQUIRED_FIELD)));

    // Check that standard verifier reports more than one problem for the broken address.
    AddressData brokenAddress = AddressData.builder()
        .setCountry("US")
        .setAdminArea("Fake")
        .setPostalCode("000")
        .build();
    assertThat(verify(brokenAddress).getProblems().size()).isAtLeast(2);

    // Check the postal verifier only reports a postal code problem.
    AddressProblems problems = new AddressProblems();
    postalCodeVerifier.verify(brokenAddress, problems);
    assertEquals(ImmutableMap.of(
        POSTAL_CODE, INVALID_FORMAT),
        problems.getProblems());
  }

  @Test public void testChinaAddress() {
    AddressData address = AddressData.builder()
        .setCountry("CN")
        .setAdminArea("Beijing Shi")
        .setLocality("Xicheng Qu")
        .setAddress("Yitiao Lu")
        .setPostalCode("123456")
        .build();
    assertThat(verify(address).getProblems()).isEmpty();

    // Clones the address but invalidates the postcode.
    address = AddressData.builder().set(address).setPostalCode("12345").build();
    assertEquals(ImmutableMap.of(
        POSTAL_CODE, INVALID_FORMAT),
        verify(address).getProblems());
  }

  @Test public void testChinaTaiwanAddress() {
    AddressData address = AddressData.builder()
        .setCountry("CN")
        .setAdminArea("Taiwan")
        .setLocality("Taichung City")
        .setDependentLocality("Xitun District")
        .setAddress("12345 Yitiao Lu")
        .setPostalCode("407")
        .build();
    assertThat(verify(address).getProblems()).isEmpty();

    address = AddressData.builder(address)
        .setDependentLocality("Foo Bar")
        .build();
    assertEquals(UNKNOWN_VALUE,
        verify(address).getProblem(DEPENDENT_LOCALITY));
  }

  @Test public void testGermanAddress() {
    AddressData address = AddressData.builder()
        .setCountry("DE")
        .setLocality("Berlin")
        .setAddress("Huttenstr. 50")
        .setPostalCode("10553")
        .setOrganization("BMW AG Niederkassung Berlin")
        .setRecipient("Herr Diefendorf")
        .build();
    assertThat(verify(address).getProblems()).isEmpty();

    // Clones address but clears the city.
    address = AddressData.builder().set(address).setLocality(null).build();
    assertEquals(ImmutableMap.of(
        LOCALITY, MISSING_REQUIRED_FIELD),
        verify(address).getProblems());
  }

  @Test public void testIrishAddress() {
    AddressData address = AddressData.builder()
        .setCountry("IE")
        .setLocality("Dublin")
        .setAdminArea("Co. Dublin")
        .setAddress("7424 118 Avenue NW")
        .setRecipient("Conan O'Brien")
        .build();
    assertThat(verify(address).getProblems()).isEmpty();

    // Clones address but clears county.
    // This address should also be valid since county is not required.
    address = AddressData.builder().set(address).setAdminArea(null).build();
    assertThat(verify(address).getProblems()).isEmpty();
  }

  @Test public void testJapanAddress() {
    AddressData address = AddressData.builder()
        .setRecipient("\u5BAE\u672C \u8302")  // SHIGERU_MIYAMOTO
        .setAddress("\u4E0A\u9CE5\u7FBD\u927E\u7ACB\u753A11\u756A\u5730")
        .setAdminArea("\u4eac\u90fd\u5e9c")  // Kyoto prefecture
        .setLocality("\u4EAC\u90FD\u5E02")  // Kyoto city
        .setCountry("JP")
        .setPostalCode("601-8501")
        .build();
    assertThat(verify(address).getProblems()).isEmpty();
  }

  @Test public void testJapanAddress_Latin() {
    AddressData address = AddressData.builder()
        .setRecipient("Shigeru Miyamoto")
        .setAddress("11-1 Kamitoba-hokotate-cho")
        .setAdminArea("KYOTO")  // Kyoto prefecture
        .setLocality("Kyoto")  // Kyoto city
        .setLanguageCode("ja_Latn")
        .setCountry("JP")
        .setPostalCode("601-8501")
        .build();
    assertThat(verify(address).getProblems()).isEmpty();
  }

  /**
   * If there is a postal code pattern for a certain country, and the input postal code is empty,
   * it should not be reported as bad postal code format. Whether empty postal code is ok should
   * be determined by checks for required fields.
   */
  @Test public void testEmptyPostalCode_Allowed() {
    // Chilean address has a postal code format pattern, but does not require postal code. The
    // following address is valid.
    AddressData address = AddressData.builder()
        .setCountry("CL")
        .setAddress("GUSTAVO LE PAIGE ST #159")
        .setAdminArea("Atacama")
        .setLocality("Alto del Carmen")
        .setPostalCode("")
        .build();
    assertThat(verify(address).getProblems()).isEmpty();
  }

  @Test public void testEmptyPostalCode_Prohibited() {
    // Check for US addresses, which requires a postal code. The following address's postal code is
    // wrong because it is missing a required field, not because the postal code doesn't match the
    // administrative area.
    AddressData address = AddressData.builder(VALID_US_ADDRESS)
        .setPostalCode("")
        .build();
    assertEquals(ImmutableMap.of(
        POSTAL_CODE, MISSING_REQUIRED_FIELD),
        verify(address).getProblems());
  }

  @Test public void testVerifyCountryOnly_Valid() {
    AddressData address = AddressData.builder()
        .setCountry("US")
        .setAdminArea("Invalid admin area") // Non-selected field should be ignored
        .build();
    AddressProblems problems = new AddressProblems();
    verifierFor(StandardChecks.PROBLEM_MAP)
        .verifyFields(address, problems, EnumSet.of(COUNTRY));
    assertThat(problems.getProblems()).isEmpty();
  }

  @Test public void testVerifyCountryOnly_InvalidCountry() {
    AddressData address = AddressData.builder()
        .setCountry("USA")
        .setAdminArea("Invalid admin area") // Non-selected field should be ignored
        .build();
    AddressProblems problems = new AddressProblems();
    verifierFor(StandardChecks.PROBLEM_MAP)
        .verifyFields(address, problems, EnumSet.of(COUNTRY));
    assertThat(problems.getProblem(COUNTRY)).isEqualTo(UNKNOWN_VALUE);
    assertThat(problems.getProblem(ADMIN_AREA)).isNull();
  }

  @Test public void testVerifyCountryAndPostalCodeOnly_Valid() {
    AddressData address = AddressData.builder()
        .setCountry("US")
        .setPostalCode("94043")
        .setAdminArea("Invalid admin area") // Non-selected field should be ignored
        .build();
    AddressProblems problems = new AddressProblems();
    verifierFor(StandardChecks.PROBLEM_MAP)
        .verifyFields(address, problems, EnumSet.of(COUNTRY, POSTAL_CODE));
    assertThat(problems.getProblems()).isEmpty();
  }

  @Test public void testVerifyCountryAndPostalCodeOnly_InvalidPostalCode() {
    AddressData address = AddressData.builder()
        .setCountry("US")
        .setPostalCode("094043")
        .setAdminArea("Invalid admin area") // Non-selected field should be ignored
        .build();
    AddressProblems problems = new AddressProblems();
    verifierFor(StandardChecks.PROBLEM_MAP)
        .verifyFields(address, problems, EnumSet.of(COUNTRY, POSTAL_CODE));
    assertThat(problems.getProblem(POSTAL_CODE)).isEqualTo(INVALID_FORMAT);
    assertThat(problems.getProblem(ADMIN_AREA)).isNull();
  }
}
