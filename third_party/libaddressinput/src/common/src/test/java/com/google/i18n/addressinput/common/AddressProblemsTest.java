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
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public class AddressProblemsTest {
  @Test public void testAddError() {
    AddressProblems ap = new AddressProblems();
    ap.add(AddressField.POSTAL_CODE, AddressProblemType.MISSING_REQUIRED_FIELD);
    assertEquals(AddressProblemType.MISSING_REQUIRED_FIELD,
        ap.getProblem(AddressField.POSTAL_CODE));
  }

  @Test public void testEmptyErrorList() {
    AddressProblems ap = new AddressProblems();
    assertNull(ap.getProblem(AddressField.POSTAL_CODE));
    assertTrue(ap.isEmpty());
  }
}
