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

import static org.junit.Assert.assertEquals;

import com.google.i18n.addressinput.common.AddressField.WidthType;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public class AddressFieldTest {
  @Test public void testOf() throws Exception {
    assertEquals(AddressField.COUNTRY, AddressField.of('R'));
  }

  @Test public void testGetChar() throws Exception {
    assertEquals('R', AddressField.COUNTRY.getChar());
  }

  @Test public void testGetWidthTypeForPostalCode() throws Exception {
    // Postal (& sorting) code always have SHORT width.
    assertEquals(WidthType.SHORT, AddressField.POSTAL_CODE.getWidthTypeForRegion("US"));
    assertEquals(WidthType.SHORT, AddressField.SORTING_CODE.getWidthTypeForRegion("DE"));
  }

  @Test public void testGetWidthTypeForCountry() throws Exception {
    // No overrides for country, so we use the default, LONG.
    assertEquals(WidthType.LONG, AddressField.COUNTRY.getWidthTypeForRegion("US"));
    assertEquals(WidthType.LONG, AddressField.COUNTRY.getWidthTypeForRegion("CH"));
  }

  @Test public void testGetWidthTypeWithOverride() throws Exception {
    // With an override.
    assertEquals(WidthType.SHORT, AddressField.LOCALITY.getWidthTypeForRegion("CN"));
    // Without an override.
    assertEquals(WidthType.LONG, AddressField.LOCALITY.getWidthTypeForRegion("US"));
  }
}
