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

import java.util.HashMap;
import java.util.Map;
import java.util.Map.Entry;

/**
 * This structure keeps track of any errors found when validating the AddressData.
 */
// This is an external class and part of the widget's public API.
// TODO: Review public API for external classes and tidy JavaDoc.
public final class AddressProblems {
  private Map<AddressField, AddressProblemType> problems =
      new HashMap<AddressField, AddressProblemType>();

  /**
   * Adds a problem of the given type for the given address field. Only one address problem is
   * saved per address field.
   */
  void add(AddressField addressField, AddressProblemType problem) {
    problems.put(addressField, problem);
  }

  /**
   * Returns true if no problems have been added.
   */
  public boolean isEmpty() {
    return problems.isEmpty();
  }

  @Override
  public String toString() {
    return problems.toString();
  }

  public void clear() {
    problems.clear();
  }

  /**
   * Returns null if no problems exists.
   */
  public AddressProblemType getProblem(AddressField addressField) {
    return problems.get(addressField);
  }

  /**
   * This will return an empty map if there are no problems.
   */
  public Map<AddressField, AddressProblemType> getProblems() {
    return problems;
  }

  /**
   * Adds all problems this object contains to the given {@link AddressProblems} object.
   */
  public void copyInto(AddressProblems other) {
    for (Entry<AddressField, AddressProblemType> problem : problems.entrySet()) {
      other.add(problem.getKey(), problem.getValue());
    }
  }
}
