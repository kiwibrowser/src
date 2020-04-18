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

import junit.framework.TestCase;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import java.util.Arrays;
import java.util.List;

@RunWith(JUnit4.class)
public class FormOptionsTest extends TestCase {

  @Test public void testReadonlyField() {
    FormOptions options = new FormOptions();

    options.setReadonly(AddressField.COUNTRY);
    FormOptions.Snapshot snapshot1 = options.createSnapshot();
    assertTrue(snapshot1.isReadonly(AddressField.COUNTRY));
    assertFalse(snapshot1.isReadonly(AddressField.LOCALITY));

    options.setReadonly(AddressField.LOCALITY);
    FormOptions.Snapshot snapshot2 = options.createSnapshot();
    assertTrue(snapshot2.isReadonly(AddressField.COUNTRY));
    assertTrue(snapshot2.isReadonly(AddressField.LOCALITY));

    assertFalse(snapshot1.isReadonly(AddressField.LOCALITY));
  }

  @Test public void testHiddenField() {
    FormOptions options = new FormOptions();

    options.setHidden(AddressField.COUNTRY);
    FormOptions.Snapshot snapshot = options.createSnapshot();
    assertTrue(snapshot.isHidden(AddressField.COUNTRY));
    assertFalse(snapshot.isHidden(AddressField.LOCALITY));

    options.setHidden(AddressField.LOCALITY);
    snapshot = options.createSnapshot();
    assertTrue(snapshot.isHidden(AddressField.COUNTRY));
    assertTrue(snapshot.isHidden(AddressField.LOCALITY));
  }

  @Test public void testCustomFieldOrder() {
    FormOptions options = new FormOptions();

    options.setCustomFieldOrder("US", AddressField.ORGANIZATION, AddressField.RECIPIENT);
    FormOptions.Snapshot snapshot = options.createSnapshot();
    List<AddressField> customOrder = snapshot.getCustomFieldOrder("US");
    assertEquals(Arrays.asList(AddressField.ORGANIZATION, AddressField.RECIPIENT), customOrder);
    try {
      customOrder.remove(0);
      fail("list should not be modifiable");
    } catch (UnsupportedOperationException e) {
      // pass - list is unmodifiable.
    }
    assertNull(snapshot.getCustomFieldOrder("GB"));
  }

  @Test public void testCustomFieldOrderEmptyList() {
    FormOptions options = new FormOptions();
    options.setCustomFieldOrder("US", AddressField.ORGANIZATION, AddressField.RECIPIENT);
    assertNotNull(options.createSnapshot().getCustomFieldOrder("US"));
    options.setCustomFieldOrder("US");
    assertNull(options.createSnapshot().getCustomFieldOrder("US"));
  }

  @Test public void testCustomFieldOrderDuplicateFields() {
    FormOptions options = new FormOptions();
    try {
      options.setCustomFieldOrder("US", AddressField.RECIPIENT, AddressField.RECIPIENT);
      fail("Expected failure for duplicate fields.");
    } catch (IllegalArgumentException e) {
      // Expected - duplicates cause failure.
    }
  }

  @Test public void testBlacklistedRegions() {
    FormOptions options = new FormOptions();

    // Lowercase
    options.blacklistRegion("ch");
    FormOptions.Snapshot snapshot = options.createSnapshot();
    // Uppercase
    assertTrue(snapshot.isBlacklistedRegion("CH"));
    assertFalse(snapshot.isBlacklistedRegion("DE"));

    // Same region, but upper case (shouldn't matter).
    options.blacklistRegion("CH");
    options.blacklistRegion("DE");
    snapshot = options.createSnapshot();
    assertTrue(snapshot.isBlacklistedRegion("CH"));
    assertTrue(snapshot.isBlacklistedRegion("DE"));
  }
}
